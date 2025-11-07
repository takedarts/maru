#include "ExecutorJob.h"

namespace deepgo {

/**
 * Creates a computation object.
 */
ExecutorJob::ExecutorJob(float* inputs, float* outputs, int32_t size)
    : _mutex(),
      _condition(),
      _inputs(inputs),
      _outputs(outputs),
      _size(size),
      _terminated(false) {
}

/**
 * Waits until computation is complete.
 */
void ExecutorJob::wait() {
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this] { return _terminated; });
  }
}

/**
 * Notifies that computation is complete.
 */
void ExecutorJob::notify() {
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }
}

/**
 * Returns input data.
 */
float* ExecutorJob::getInputs() const {
  return _inputs;
}

/**
 * Returns output data.
 */
float* ExecutorJob::getOutputs() const {
  return _outputs;
}

/**
 * Returns the number of data.
 */
int32_t ExecutorJob::getSize() const {
  return _size;
}

}  // namespace deepgo
