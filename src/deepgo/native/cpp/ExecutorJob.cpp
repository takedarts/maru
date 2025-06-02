#include "ExecutorJob.h"

namespace deepgo {

/**
 * 計算オブジェクトを生成する。
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
 * 計算が完了するまで待機する。
 */
void ExecutorJob::wait() {
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this] { return _terminated; });
  }
}

/**
 * 計算が完了したことを通知する。
 */
void ExecutorJob::notify() {
  {  // synchronize
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }
}

/**
 * 入力データを返す。
 */
float* ExecutorJob::getInputs() const {
  return _inputs;
}

/**
 * 出力データを返す。
 */
float* ExecutorJob::getOutputs() const {
  return _outputs;
}

/**
 * データの数を返す。
 */
int32_t ExecutorJob::getSize() const {
  return _size;
}

}  // namespace deepgo
