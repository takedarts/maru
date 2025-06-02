#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace deepgo {

class ExecutorJob {
 public:
  /**
   * 計算オブジェクトを生成する。
   */
  ExecutorJob(float* inputs, float* outputs, int32_t size);

  /**
   * 計算が完了するまで待機する。
   */
  void wait();

  /**
   * 計算が完了したことを通知する。
   */
  void notify();

  /**
   * 入力データを返す。
   */
  float* getInputs() const;

  /**
   * 出力データを返す。
   */
  float* getOutputs() const;

  /**
   * データの数を返す。
   */
  int32_t getSize() const;

 private:
  /**
   * 同期用のミューテックス。
   */
  std::mutex _mutex;

  /**
   * 同期用の条件変数。
   */
  std::condition_variable _condition;

  /**
   * 入力データ。
   */
  float* _inputs;

  /**
   * 出力データ。
   */
  float* _outputs;

  /**
   * データの数。
   */
  int32_t _size;

  /**
   * 計算が完了していればtrue。
   */
  bool _terminated;
};

}  // namespace deepgo
