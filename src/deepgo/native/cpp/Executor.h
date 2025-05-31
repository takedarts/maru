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
   * 推論実行オブジェクトを生成する。
   * @param model モデルファイル
   * @param gpu GPU番号
   * @param batchSize バッチサイズの最大値
   * @param fp16 16bit精度で計算するならTrue
   * @param deterministic 計算結果を再現可能にするならTrue
   */
  Executor(
      std::string model, int32_t gpu, int32_t batchSize, bool fp16, bool deterministic);

  /**
   * デストラクタ。
   */
  virtual ~Executor();

  /**
   * 推論を実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 入出力データの数
   */
  void execute(float* inputs, float* outputs, int32_t size);

  /**
   * 待機中の計算処理の数を返す。
   * @return 待機中の計算処理の数
   */
  int32_t getWaitingCount();

  /**
   * 計算処理の予約数を増やす。
   * @param reservedCount 予約数
   */
  void addReservedCount(int32_t reservedCount);

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
   * 推論オブジェクト。
   */
  std::unique_ptr<Model> _model;

  /**
   * スレッドオブジェクト。
   */
  std::unique_ptr<std::thread> _thread;

  /**
   * 終了フラグ。
   */
  bool _terminated;

  /**
   * 計算処理のキュー。
   */
  std::queue<ExecutorJob*> _queue;

  /**
   * 待機中の計算処理の数。
   */
  int32_t _waitingCount;

  /**
   * 予約された計算処理の数。
   */
  int32_t _reservedCount;

  /**
   * バッチサイズの最大値。
   */
  int32_t _batchSize;

  /**
   * スレッドで実行されるメソッド。
   */
  void _run();

  /**
   * 計算を実行する。
   * @param jobs 計算タスクのリスト
   */
  void _forward(std::vector<ExecutorJob*>& jobs);
};

}  // namespace deepgo
