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
   * Gets the inference efficiency.
   * @return The inference efficiency
   */
  inline float getEfficiency() const {
    float total_efficiency = 0.0f;

    for (const auto& efficiency : _efficiencies) {
      total_efficiency += efficiency.load(std::memory_order_relaxed);
    }

    return total_efficiency / static_cast<float>(_efficiencies.size());
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
   * Inference efficiencies.
   */
  std::vector<std::atomic<float>> _efficiencies;

  /**
   * Processing executed in the inference thread.
   * @param threadIndex Thread index
   */
  void _run(int32_t threadIndex);
};

}  // namespace deepgo
