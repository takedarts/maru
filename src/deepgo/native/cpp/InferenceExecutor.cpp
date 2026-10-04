#include "InferenceExecutor.h"

#include <algorithm>
#include <utility>

#include "Config.h"
#include "InferenceProcessor.h"
#include "MctsNode.h"
#include "Move.h"

namespace deepgo {

/**
 * Gets the policy index for the specified coordinates.
 * @param board Board state
 * @param x X coordinate
 * @param y Y coordinate
 * @return Policy index
 */
static int32_t getPolicyIndex(const Board* board, int32_t x, int32_t y) {
  // Calculates the index when placing a variable-size board at the center of the model input
  const int32_t offset_x = (MODEL_BOARD_SIZE - board->getWidth()) / 2;
  const int32_t offset_y = (MODEL_BOARD_SIZE - board->getHeight()) / 2;

  return (offset_y + y) * MODEL_BOARD_SIZE + (offset_x + x);
}

/**
 * Fill unused entries of a fixed-size batch with the last valid input.
 * @param inputBuffer Model input buffer
 * @param size Number of valid inputs
 * @param batchSize Fixed batch size
 */
static void padInputBuffer(
    std::vector<int32_t>& inputBuffer, int32_t size, int32_t batchSize) {
  // Return if there are no unused entries
  if (size <= 0 || size >= batchSize) {
    return;
  }

  // Copy the last input into unused entries
  const int32_t* last_input =
      inputBuffer.data() + (size - 1) * MODEL_INPUT_PACK_SIZE;

  for (int32_t i = size; i < batchSize; i++) {
    int32_t* padded_input =
        inputBuffer.data() + i * MODEL_INPUT_PACK_SIZE;

    std::copy(
        last_input, last_input + MODEL_INPUT_PACK_SIZE, padded_input);
  }
}

/**
 * Creates an inference executor object.
 * @param processor Inference management object
 * @param file Model file
 * @param gpu GPU number
 * @param fp16 true to use half precision
 * @param deterministic true to run deterministically
 * @param batchSize Batch size
 * @param threads Number of execution threads
 */
InferenceExecutor::InferenceExecutor(
    InferenceProcessor* processor, std::string file, int32_t gpu, bool fp16,
    bool deterministic, int32_t batchSize, int32_t threads)
    : _mutex(),
      _processor(processor),
      _model(nullptr),
      _file(file),
      _gpu(gpu),
      _fp16(fp16),
      _deterministic(deterministic),
      _batchSize(batchSize),
      _threads(),
      _batchFillRates(threads) {
  for (int32_t i = 0; i < threads; i++) {
    _threads.emplace_back(&InferenceExecutor::_run, this, i);
    _batchFillRates[i].store(0.0f, std::memory_order_relaxed);
  }
}

/**
 * Destroys the inference executor object.
 */
InferenceExecutor::~InferenceExecutor() {
  // Waits for threads to finish
  for (auto& thread : _threads) {
    thread.join();
  }

  // Destroys the inference model object
  {
    std::lock_guard<std::mutex> lock(_mutex);

    if (_model != nullptr) {
      delete _model;
      _model = nullptr;
    }
  }
}

/**
 * Run inference synchronously.
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
    std::lock_guard<std::mutex> lock(_mutex);

    // The model is lazily loaded on the first execution and shared for subsequent inference
    if (_model == nullptr) {
      _model = new InferenceModel(_file, _gpu, _fp16, _deterministic);
    }

    model = _model;
  }

  // When running on non-CPU devices, executes inference with the specified batch size
  if (!_model->isCpu()) {
    std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
    std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

    for (int32_t i = 0; i < size; i += _batchSize) {
      int32_t batch_size = std::min(_batchSize, size - i);
      int32_t* inputs_begin = inputs + (i * MODEL_INPUT_PACK_SIZE);
      int32_t inputs_size = batch_size * MODEL_INPUT_PACK_SIZE;
      float* outputs_begin = outputs + (i * MODEL_OUTPUT_SIZE);
      int32_t outputs_size = batch_size * MODEL_OUTPUT_SIZE;

      std::copy(inputs_begin, inputs_begin + inputs_size, input_buffer.data());
      padInputBuffer(input_buffer, batch_size, _batchSize);
      model->forward(input_buffer.data(), output_buffer.data(), _batchSize);
      std::copy(output_buffer.data(), output_buffer.data() + outputs_size, outputs_begin);
    }
  } else {
    model->forward(inputs, outputs, size);
  }
}

/**
 * Processing executed in the inference thread.
 * @param threadIndex Thread index
 */
void InferenceExecutor::_run(int32_t threadIndex) {
  // Sets the device to use
  torch::DeviceGuard device_guard(InferenceModel::getDevice(_gpu));

  // Creates buffers for input data, mask data, and output data
  std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
  std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

  // Creates the model object if it has not been created
  {
    std::lock_guard<std::mutex> lock(_mutex);

    if (_model == nullptr) {
      _model = new InferenceModel(_file, _gpu, _fp16, _deterministic);
    }
  }

  // Repeatedly fetches inference requests from the queue, creates a batch,
  // executes inference, and applies results to nodes
  while (true) {
    // Fetches inference requests from the queue and creates a batch
    std::vector<std::pair<MctsNode*, InferenceExecutorCallback>> batch;

    {
      // Acquires a lock for synchronization
      std::unique_lock<std::mutex> lock(_processor->_queueMutex);

      // Waits if there are no pending inference requests
      // [Stop] Ends waiting if a stop request exists and the queue is empty
      // [Inference] Ends waiting if there are pending inference requests
      _processor->_queueCondition.wait(lock, [this] {
        if (_processor->_terminated && _processor->_queue.empty()) {
          return true;
        } else if (!_processor->_queue.empty()) {
          return true;
        } else {
          return false;
        }
      });

      // Breaks out of the loop if inference should be terminated
      if (_processor->_terminated && _processor->_queue.empty()) {
        break;
      }

      // Pops up to the maximum batch size from the queue to form an inference batch
      while (!_processor->_queue.empty() && batch.size() < static_cast<size_t>(_batchSize)) {
        batch.push_back(_processor->_queue.front());
        _processor->_queue.pop();
      }

      // Notifies other threads that the queue state has changed
      _processor->_queueCondition.notify_all();
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

    // On non-CPU devices, fill unused batch entries with the last input
    int32_t input_size = static_cast<int32_t>(batch.size());
    int32_t batch_size = (_model->isCpu()) ? input_size : _batchSize;

    padInputBuffer(input_buffer, input_size, batch_size);

    // Use the actual input count on CPU and a fixed batch size on other devices
    _model->forward(input_buffer.data(), output_buffer.data(), batch_size);

    // Calculates and stores the ratio of inference requests in the batch
    float fill_rate = static_cast<float>(batch.size()) / static_cast<float>(batch_size);
    float old_fill_rate = _batchFillRates[threadIndex].load(std::memory_order_relaxed);
    float new_fill_rate = old_fill_rate * 0.99f + fill_rate * 0.01f;

    _batchFillRates[threadIndex].store(new_fill_rate, std::memory_order_relaxed);

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
      float value = outputs[MODEL_VALUE_OFFSET] * 2.0f - 1.0f;

      if (color == COLOR_WHITE) {
        value = -value;
      }

      // Convert the predicted score to Black's perspective
      float score = outputs[MODEL_VALUE_OFFSET + 2] * 20.0f;

      if (color == COLOR_WHITE) {
        score = -score;
      }

      // Gets the coordinates eligible for moves
      std::vector<int32_t> enableds(width * height);
      std::vector<int32_t> territories(width * height);

      board.getEnableds(enableds.data(), color, true);
      board.getFixedTerritories(territories.data());

      // Create policy inference results
      std::vector<std::pair<Move, float>> policies;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;

          if (enableds[board_index] == 1 && territories[board_index] == COLOR_EMPTY) {
            float probability = outputs[getPolicyIndex(&board, x, y)];

            policies.emplace_back(Move(x, y, color), probability);
          }
        }
      }

      // Read the pass probability from the end of the first policy plane
      const int32_t pass_policy_index = MODEL_BOARD_SIZE * MODEL_BOARD_SIZE;
      float pass_probability = outputs[pass_policy_index];

      policies.emplace_back(Move::createPassMove(color), pass_probability);

      // Creates the Territory inference result
      // Swaps the 0th and 2nd inference results when it is White's turn
      std::array<float, MODEL_TERRITORY_SIZE> territory_probs;
      const float* territory_outputs = outputs + MODEL_TERRITORY_OFFSET;
      const int8_t black_index = (color == COLOR_BLACK) ? 0 : 2;
      const int8_t seki_index = 1;
      const int8_t white_index = (color == COLOR_BLACK) ? 2 : 0;

      std::copy(
          territory_outputs + black_index * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_outputs + (black_index + 1) * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_probs.begin() + 0 * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE);

      std::copy(
          territory_outputs + seki_index * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_outputs + (seki_index + 1) * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_probs.begin() + 1 * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE);

      std::copy(
          territory_outputs + white_index * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_outputs + (white_index + 1) * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE,
          territory_probs.begin() + 2 * MODEL_BOARD_SIZE * MODEL_BOARD_SIZE);

      // Calls the callback function
      callback(node, InferenceResult(value, score, policies, territory_probs));
    }
  }
}

}  // namespace deepgo
