#include "InferenceExecutor.h"

#include <algorithm>

#include "Config.h"
#include "MctsNode.h"
#include "Move.h"

namespace deepgo {

/**
 * Gets the policy index for the specified coordinates.
 * @param board Board
 * @param x X coordinate
 * @param y Y coordinate
 * @return Policy index
 */
static int32_t getPolicyIndex(const Board* board, int32_t x, int32_t y) {
  // Calculates the index when placing a variable-size board at the center of the model input
  int32_t offset_x = (MODEL_SIZE - board->getWidth()) / 2;
  int32_t offset_y = (MODEL_SIZE - board->getHeight()) / 2;

  return (offset_y + y) * MODEL_SIZE + (offset_x + x);
}

/**
 * Creates an inference executor object.
 * @param model Model file
 * @param gpu GPU number
 * @param fp16 true to use half precision
 * @param deterministic true to run deterministically
 * @param batchSize Batch size
 * @param threads Number of execution threads
 */
InferenceExecutor::InferenceExecutor(
    std::string model, int32_t gpu, bool fp16, bool deterministic,
    int32_t batchSize, int32_t threads)
    : _modelMutex(),
      _threadMutex(),
      _condition(),
      _model(nullptr),
      _modelFile(model),
      _gpu(gpu),
      _fp16(fp16),
      _deterministic(deterministic),
      _batchSize(batchSize),
      _threads(),
      _terminated(false),
      _queue() {
  for (int32_t i = 0; i < threads; i++) {
    _threads.emplace_back(&InferenceExecutor::_run, this);
  }
}

/**
 * Destroys the inference executor object.
 */
InferenceExecutor::~InferenceExecutor() {
  // Terminates the inference processing
  {
    std::lock_guard<std::mutex> lock(_threadMutex);
    _terminated = true;
  }

  _condition.notify_all();

  // Waits for threads to finish
  for (auto& thread : _threads) {
    thread.join();
  }

  // Destroys the inference model object
  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    if (_model != nullptr) {
      delete _model;
      _model = nullptr;
    }
  }
}

/**
 * Submits an inference execution request.
 * @param node Node to perform inference on
 * @param callback Callback invoked when inference completes
 */
void InferenceExecutor::submit(MctsNode* node, InferenceExecutorCallback callback) {
  std::lock_guard<std::mutex> lock(_threadMutex);
  _queue.emplace_back(node, callback);
  _condition.notify_one();
}

/**
 * Executes inference synchronously.
 * @param inputs Input data
 * @param outputs Output data
 * @param size Number of data samples to evaluate
 */
void InferenceExecutor::execute(int32_t* inputs, float* outputs, int32_t size) {
  // Sets the device to use
  torch::DeviceGuard device_guard(InferenceModel::getDevice(_gpu));

  // Gets the model object
  // Creates the model object if it has not been created
  InferenceModel* model = nullptr;

  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    // The model is lazily loaded on the first execution and shared for subsequent inference
    if (_model == nullptr) {
      _model = new InferenceModel(_modelFile, _gpu, _fp16, _deterministic);
    }

    model = _model;
  }

  // When running on non-CPU devices, executes inference with the specified batch size
  if (!_model->isCpu()) {
    std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
    std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

    std::copy(inputs, inputs + (size * MODEL_INPUT_PACK_SIZE), input_buffer.data());
    model->forward(input_buffer.data(), output_buffer.data(), _batchSize);
    std::copy(output_buffer.data(), output_buffer.data() + (size * MODEL_OUTPUT_SIZE), outputs);
  } else {
    model->forward(inputs, outputs, size);
  }
}

/**
 * Gets the number of pending inference requests.
 * @return Number of pending inference requests
 */
int32_t InferenceExecutor::getQueueSize() {
  std::lock_guard<std::mutex> lock(_threadMutex);
  return static_cast<int32_t>(_queue.size());
}

/**
 * Processing executed in the inference thread.
 */
void InferenceExecutor::_run() {
  // Sets the device to use
  torch::DeviceGuard device_guard(InferenceModel::getDevice(_gpu));

  // Creates buffers for input data, mask data, and output data
  std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
  std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

  // Creates the model object if it has not been created
  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    if (_model == nullptr) {
      _model = new InferenceModel(_modelFile, _gpu, _fp16, _deterministic);
    }
  }

  // Repeatedly fetches inference requests from the queue, creates a batch,
  // executes inference, and applies results to nodes
  while (true) {
    // Fetches inference requests from the queue and creates a batch
    std::vector<std::pair<MctsNode*, InferenceExecutorCallback>> batch;

    {
      // Acquires a lock for synchronization
      std::unique_lock<std::mutex> lock(_threadMutex);

      // Waits if there are no pending inference requests
      // [Stop] Ends waiting if a stop request exists and the queue is empty
      // [Inference] Ends waiting if there are pending inference requests
      _condition.wait(lock, [this] {
        if (_terminated && _queue.empty()) {
          return true;
        } else if (!_queue.empty()) {
          return true;
        } else {
          return false;
        }
      });

      // Breaks out of the loop if inference should be terminated
      if (_terminated && _queue.empty()) {
        break;
      }

      // Pops up to the maximum batch size from the queue to form an inference batch
      while (!_queue.empty() && batch.size() < static_cast<size_t>(_batchSize)) {
        batch.push_back(_queue.back());
        _queue.pop_back();
      }
    }

    // Initializes the input data buffer
    std::fill(input_buffer.begin(), input_buffer.end(), 0);

    // Converts each node's board state to model input
    for (size_t i = 0; i < batch.size(); i++) {
      MctsNode* node = batch[i].first;
      int32_t* inputs = input_buffer.data() + (i * MODEL_INPUT_PACK_SIZE);
      int32_t color = node->getNextColor();
      Board board(node->getBoard());

      board.getInputs(inputs, color, node->getKomi(), node->getRule(), node->getSuperko());
    }

    // Executes inference
    // When running on CPU, adjusts the batch size to the actual batch size
    // When running on non-CPU devices, executes inference with the specified batch size
    int32_t batch_size = (_model->isCpu()) ? static_cast<int32_t>(batch.size()) : _batchSize;

    _model->forward(input_buffer.data(), output_buffer.data(), batch_size);

    // Applies inference results to nodes and invokes the callback functions
    for (size_t i = 0; i < batch.size(); i++) {
      float* outputs = output_buffer.data() + (i * MODEL_OUTPUT_SIZE);
      MctsNode* node = batch[i].first;
      InferenceExecutorCallback callback = batch[i].second;
      Board board(node->getBoard());
      int32_t color = node->getNextColor();
      int32_t width = board.getWidth();
      int32_t height = board.getHeight();

      // Creates the Value inference result
      float value = outputs[MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE] * 2.0f - 1.0f;

      if (color == WHITE) {
        value = -value;
      }

      // Gets the coordinates eligible for moves
      std::vector<int32_t> enableds(width * height);
      std::vector<int32_t> territories(width * height);

      board.getEnableds(enableds.data(), color, true);
      board.getTerritories(territories.data(), color);

      // Calculates the total probability of policies
      float total_probability = 0.0f;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;

          if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
            total_probability += outputs[getPolicyIndex(&board, x, y)];
          }
        }
      }

      // Creates the Policy inference result
      std::vector<Policy> policies;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;

          if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
            float probability =
                outputs[getPolicyIndex(&board, x, y)] / (total_probability + 1e-6f);

            policies.emplace_back(Move(x, y, color), probability, 0);
          }
        }
      }

      // Creates the Territory inference result
      // Swaps the 0th and 2nd inference results when it is White's turn
      std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> territory_probs;
      const int8_t black_index = (color == BLACK) ? 2 : 4;
      const int8_t seki_index = 3;
      const int8_t white_index = (color == BLACK) ? 4 : 2;

      std::copy(
          outputs + black_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (black_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 0 * MODEL_SIZE * MODEL_SIZE);

      std::copy(
          outputs + seki_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (seki_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 1 * MODEL_SIZE * MODEL_SIZE);

      std::copy(
          outputs + white_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (white_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 2 * MODEL_SIZE * MODEL_SIZE);

      // Invokes the callback function
      callback(node, InferenceResult(value, policies, territory_probs));
    }
  }
}

}  // namespace deepgo
