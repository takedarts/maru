#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "Executor.h"
#include "Model.h"

namespace deepgo {

class Processor {
 public:
  /**
   * 計算管理オブジェクトを生成する。
   * @param model モデルファイル
   * @param gpus GPU番号のリスト
   * @param batchSize バッチサイズの最大値
   * @param fp16 16bit精度で計算するならTrue
   * @param deterministic 計算結果を再現可能にするならTrue
   * @param threadsParGpu GPUごとのスレッド数
   */
  Processor(
      std::string model, std::vector<int32_t> gpus, int32_t batchSize,
      bool fp16, bool deterministic, int32_t threadsParGpu);

  /**
   * 推論を実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 入出力データの数
   */
  void execute(float* inputs, float* outputs, int32_t size);

 private:
  /**
   * 同期用のミューテックス。
   */
  std::mutex _mutex;

  /**
   * 計算実行オブジェクト。
   */
  std::vector<std::unique_ptr<Executor>> _executors;
};

}  // namespace deepgo
