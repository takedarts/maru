#pragma once

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

using InferenceExecutorCallback =
    std::function<void(MctsNode*, const InferenceResult&)>;

/**
 * A class that asynchronously processes inference execution.
 */
class InferenceExecutor {
 public:
  /**
   * Creates an inference executor object.
   * @param model Model file
   * @param gpu GPU number
   * @param fp16 true to use half precision
   * @param deterministic true to run deterministically
   * @param batchSize Batch size
   * @param threads Number of execution threads
   */
  InferenceExecutor(
      std::string model, int32_t gpu, bool fp16, bool deterministic,
      int32_t batchSize, int32_t threads);

  /**
   * Destroys the inference executor object.
   */
  virtual ~InferenceExecutor();

  /**
   * Submits an inference execution request.
   * @param node Node to perform inference on
   * @param callback Callback invoked when inference completes
   */
  void submit(MctsNode* node, InferenceExecutorCallback callback);

  /**
   * Executes inference synchronously.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of data samples to evaluate
   */
  void execute(int32_t* inputs, float* outputs, int32_t size);

  /**
   * Gets the number of pending inference requests.
   * @return Number of pending inference requests
   */
  int32_t getQueueSize();

 private:
  /**
   * Mutex for model synchronization.
   */
  std::mutex _modelMutex;

  /**
   * Mutex for thread synchronization.
   */
  std::mutex _threadMutex;

  /**
   * Condition variable.
   */
  std::condition_variable _condition;

  /**
   * Inference model.
   */
  InferenceModel* _model;

  /**
   * Model file.
   */
  std::string _modelFile;

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
   * true if terminating.
   */
  bool _terminated;

  /**
   * Queue of pending inference requests.
   */
  std::vector<std::pair<MctsNode*, InferenceExecutorCallback>> _queue;

  /**
   * Processing executed in the inference thread.
   */
  void _run();
};

}  // namespace deepgo
