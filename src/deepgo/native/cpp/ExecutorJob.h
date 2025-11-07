#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace deepgo {

class ExecutorJob {
 public:
  /**
   * Creates a computation object.
   */
  ExecutorJob(float* inputs, float* outputs, int32_t size);

  /**
   * Waits until computation is complete.
   */
  void wait();

  /**
   * Notifies that computation is complete.
   */
  void notify();

  /**
   * Returns input data.
   */
  float* getInputs() const;

  /**
   * Returns output data.
   */
  float* getOutputs() const;

  /**
   * Returns the number of data.
   */
  int32_t getSize() const;

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
   * Input data.
   */
  float* _inputs;

  /**
   * Output data.
   */
  float* _outputs;

  /**
   * Number of data.
   */
  int32_t _size;

  /**
   * True if computation is complete.
   */
  bool _terminated;
};

}  // namespace deepgo
