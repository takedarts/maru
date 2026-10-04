#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "Board.h"
#include "InferenceCache.h"
#include "InferenceExecutor.h"
#include "InferenceResult.h"

namespace deepgo {

/**
 * A class that manages inference execution.
 */
class InferenceProcessor {
 private:
  // Make InferenceExecutor a friend class to access the inference waiting queue
  friend class InferenceExecutor;

 public:
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
  InferenceProcessor(
      std::string model, std::vector<int32_t> gpus, bool fp16, bool deterministic,
      int32_t batchSize, int32_t threadsPerGpu, int32_t cacheSize);

  /**
   * Destroys the inference processor object.
   */
  virtual ~InferenceProcessor();

  /**
   * Submits an inference execution request.
   * @param node Node to perform inference on
   * @param callback Callback invoked when inference completes
   */
  void submit(MctsNode* node, std::function<void(MctsNode*)> callback);

  /**
   * Gets the evaluation value for the specified board.
   * @param board Board state
   * @param color Color of the stone to play next
   * @param komi Komi
   * @param rule Rule
   * @param superko true to use the super ko rule
   * @return Evaluation value
   */
  float predict(Board* board, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * Run inference synchronously.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of data samples to evaluate
   */
  void execute(int32_t* inputs, float* outputs, int32_t size);

  /**
   * Gets the number of inference threads.
   * @return Number of inference threads
   */
  inline int32_t getThreadSize() const {
    return _threadSize;
  }

  /**
   * Gets the batch size.
   * @return Batch size
   */
  inline int32_t getBatchSize() const {
    return _batchSize;
  }

  /**
   *  Get the ratio of inference requests included in the batch.
   * @return Batch fill rate
   */
  inline float getBatchFillRate() const {
    float total_fill_rate = 0.0f;

    for (const auto& executor : _executors) {
      total_fill_rate += executor->getBatchFillRate();
    }

    return total_fill_rate / static_cast<float>(_executors.size());
  }

  /**
   * Get the cache hit rate of inference.
   * @return Cache hit rate for inference
   */
  inline float getCacheHitRate() const {
    return _cache.getHitRate();
  }

 private:
  /**
   * Mutex for synchronizing the queue.
   */
  std::mutex _queueMutex;

  /**
   * Condition variable for waiting on the queue.
   */
  std::condition_variable _queueCondition;

  /**
   * Inference waiting queue.
   */
  std::queue<std::pair<MctsNode*, InferenceExecutorCallback>> _queue;

  /**
   * Inference result cache.
   */
  InferenceCache _cache;

  /**
   * Inference executor objects.
   */
  std::vector<std::unique_ptr<InferenceExecutor>> _executors;

  /**
   * True if the processor is terminated.
   */
  bool _terminated;

  /**
   * Number of inference threads.
   */
  int32_t _threadSize;

  /**
   * Batch size.
   */
  int32_t _batchSize;

};

}  // namespace deepgo
