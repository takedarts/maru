#include "InferenceProcessor.h"

#include "MctsManager.h"
#include "MctsNode.h"
#include "MctsParameter.h"

namespace deepgo {

/**
 * Creates an inference processor object.
 * @param model Model file
 * @param gpus List of GPU numbers
 * @param fp16 true to use half precision
 * @param deterministic true to run deterministically
 * @param batchSize Batch size
 * @param threadsPerGpu Number of threads per GPU
 * @param cacheSize Cache size for inference results
 */
InferenceProcessor::InferenceProcessor(
    std::string model, std::vector<int32_t> gpus, bool fp16, bool deterministic,
    int32_t batchSize, int32_t threadsPerGpu, int32_t cacheSize)
    : _cacheMutex(),
      _queueMutex(),
      _queueCondition(),
      _queue(),
      _executors(),
      _cacheSize(cacheSize),
      _cacheKeys(),
      _cacheResults(),
      _terminated(false),
      _threadSize(static_cast<int32_t>(gpus.size()) * threadsPerGpu),
      _batchSize(batchSize) {
  for (int32_t gpu : gpus) {
    _executors.emplace_back(std::make_unique<InferenceExecutor>(
        this, model, gpu, fp16, deterministic, batchSize, threadsPerGpu));
  }
}

/**
 * Destroys the inference processor object.
 */
InferenceProcessor::~InferenceProcessor() {
  // Sends a termination request to the inference threads
  {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _terminated = true;
    _queueCondition.notify_all();
  }

  // Waits for the inference threads to finish
  _executors.clear();
}

/**
 * Submits an inference execution request.
 * @param node Node to perform inference on
 * @param callback Callback invoked when inference completes
 */
void InferenceProcessor::submit(MctsNode* node, std::function<void(MctsNode*)> callback) {
  // Variable to store the cached inference result
  InferenceResult cached_result;
  // Flag indicating whether a cached inference result was found
  bool cached_result_found = false;

  // Acquires a lock for synchronization
  {
    std::lock_guard<std::mutex> lock(_cacheMutex);

    // Checks whether a cached inference result exists
    // If a cached inference result exists, stores it and sets the flag
    // Applying to the node and calling the callback are done outside the lock
    BoardHash hash(&node->getBoard(), node->getNextColor());
    auto it = _cacheResults.find(hash);

    if (it != _cacheResults.end()) {
      cached_result = it->second;
      cached_result_found = true;
    }
  }

  // If a cached inference result was found,
  // applies it to the node and calls the callback function
  if (cached_result_found) {
    node->applyInferenceResult(cached_result);
    callback(node);
    return;
  }

  // Defines the callback function to invoke when inference completes
  auto exec_callback = [this, node, callback](MctsNode*, const InferenceResult& result) {
    {
      std::lock_guard<std::mutex> lock(_cacheMutex);

      // Checks whether a result exists in the cache
      // If no result exists, saves it to the cache
      BoardHash hash(&node->getBoard(), node->getNextColor());
      auto it = _cacheResults.find(hash);

      if (it == _cacheResults.end() && _cacheSize > 0) {
        _cacheResults.insert({hash, result});
        _cacheKeys.push(hash);
      }

      // Removes the oldest entries when the cache size is exceeded
      while (_cacheSize > 0 && _cacheKeys.size() >= static_cast<size_t>(_cacheSize)) {
        _cacheResults.erase(_cacheKeys.front());
        _cacheKeys.pop();
      }
    }

    // Applies the inference result to the node
    node->applyInferenceResult(result);

    // Calls the callback function
    callback(node);
  };

  // Adds an inference request to the waiting queue
  {
    std::lock_guard<std::mutex> lock(_queueMutex);
    _queue.push({node, exec_callback});
    _queueCondition.notify_one();
  }
}

/**
 * Gets the evaluation value for the specified board.
 * @param board Board
 * @param color Color of the stone to play next
 * @param komi Komi
 * @param rule Rule
 * @param superko true to use the super ko rule
 * @return Evaluation value
 */
float InferenceProcessor::predict(
    Board* board, int32_t color, float komi, int32_t rule, bool superko) {
  // Creates a temporary MCTS node for synchronous evaluation, reusing the standard inference path
  MctsParameter parameter(
      board->getWidth(), board->getHeight(), komi, rule, superko, 1.0f, 18200.0f);
  MctsManager manager(parameter);
  MctsNode* node = manager.createNode();

  // Sets the board state in the node object
  node->initialize(board, -1, -1, OPPOSITE(color), 0);

  // Creates synchronization objects and a condition variable to wait for inference completion
  std::mutex mutex;
  std::condition_variable cv;

  // Executes inference and waits for the node's evaluation value to be updated
  {
    std::unique_lock<std::mutex> lock(mutex);

    submit(node, [&cv](MctsNode*) { cv.notify_one(); });
    cv.wait(lock, [node] { return node->isEvaluated(); });
  }

  return node->getNodeValue();
}

/**
 * Executes inference synchronously.
 * @param inputs Input data
 * @param outputs Output data
 * @param size Number of data samples to evaluate
 */
void InferenceProcessor::execute(int32_t* inputs, float* outputs, int32_t size) {
  _executors[0]->execute(inputs, outputs, size);
}

}  // namespace deepgo
