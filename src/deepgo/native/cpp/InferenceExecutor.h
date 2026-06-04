#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "InferenceModel.h"
#include "InferenceResult.h"

namespace deepgo {

class MctsNode;
class InferenceProcessor;

using InferenceExecutorCallback =
    std::function<void(MctsNode*, const InferenceResult&)>;

/**
 * A class that asynchronously processes inference execution.
 */
class InferenceExecutor {
 public:
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
  InferenceExecutor(
      InferenceProcessor* processor, std::string file, int32_t gpu, bool fp16,
      bool deterministic, int32_t batchSize, int32_t threads);

  /**
   * Destroys the inference executor object.
   */
  virtual ~InferenceExecutor();

  /**
   * Executes inference synchronously.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of data samples to evaluate
   */
  void execute(int32_t* inputs, float* outputs, int32_t size);

  /**
   * Gets the batch fill rate.
   * @return The batch fill rate
   */
  inline float getBatchFillRate() const {
    float total_fill_rate = 0.0f;

    for (const auto& fill_rate : _batchFillRates) {
      total_fill_rate += fill_rate.load(std::memory_order_relaxed);
    }

    return total_fill_rate / static_cast<float>(_batchFillRates.size());
  }

 private:
  /**
   * Mutex for model synchronization.
   */
  std::mutex _mutex;

  /**
   * Inference management object.
   */
  InferenceProcessor* _processor;

  /**
   * Inference model.
   */
  InferenceModel* _model;

  /**
   * Model file.
   */
  std::string _file;

  /**
   * GPU number.
   */
  int32_t _gpu;

  /**
   * true to use half precision.
   */
  bool _fp16;

  /**
   * true to run deterministically.
   */
  bool _deterministic;

  /**
   * Batch size.
   */
  int32_t _batchSize;

  /**
   * Inference threads.
   */
  std::vector<std::thread> _threads;

  /**
   * Variable storing the ratio of inference requests included in the batch.
   */
  std::vector<std::atomic<float>> _batchFillRates;

  /**
   * Processing executed in the inference thread.
   * @param threadIndex Thread index
   */
  void _run(int32_t threadIndex);
};

}  // namespace deepgo
