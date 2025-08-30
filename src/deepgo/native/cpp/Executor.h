#pragma once

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "ExecutorJob.h"
#include "Model.h"

namespace deepgo {

class Executor {
 public:
  /**
   * Creates an inference execution object.
   * @param model Model file
   * @param gpu GPU number
   * @param batchSize Maximum batch size
   * @param fp16 True to compute with 16-bit precision
   * @param deterministic True to make computation results reproducible
   */
  Executor(
      std::string model, int32_t gpu, int32_t batchSize, bool fp16, bool deterministic);

  /**
   * Delete the object.
   */
  virtual ~Executor();

  /**
   * Executes inference.
   * @param inputs Input data
   * @param outputs Output data
   * @param size Number of input/output data
   */
  void execute(float* inputs, float* outputs, int32_t size);

  /**
   * Returns the number of pending computation tasks.
   * @return Number of pending computation tasks
   */
  int32_t getWaitingCount();

  /**
   * Increases the number of reserved computation tasks.
   * @param reservedCount Number of reservations
   */
  void addReservedCount(int32_t reservedCount);

 private:
  /**
   * Mutex for synchronization.
   */
  std::mutex _mutex;

  /**
   * Condition variable for synchronization.
   */
  std::condition_variable _condition;

  /**
   * Inference object.
   */
  std::unique_ptr<Model> _model;

  /**
   * Thread object.
   */
  std::unique_ptr<std::thread> _thread;

  /**
   * Termination flag.
   */
  bool _terminated;

  /**
   * Queue of computation tasks.
   */
  std::queue<ExecutorJob*> _queue;

  /**
   * Number of pending computation tasks.
   */
  int32_t _waitingCount;

  /**
   * Number of reserved computation tasks.
   */
  int32_t _reservedCount;

  /**
   * Maximum batch size.
   */
  int32_t _batchSize;

  /**
   * Method executed by the thread.
   */
  void _run();

  /**
   * Executes computation.
   * @param jobs List of computation tasks
   */
  void _forward(std::vector<ExecutorJob*>& jobs);
};

}  // namespace deepgo
