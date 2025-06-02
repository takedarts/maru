#include "Processor.h"

#include <algorithm>
#include <iostream>

namespace deepgo {

/**
 * 計算管理オブジェクトを生成する。
 * @param model モデルファイル
 * @param gpus GPU番号のリスト
 * @param batchSize バッチサイズの最大値
 * @param fp16 16bit精度で計算するならTrue
 * @param deterministic 計算結果を再現可能にするならTrue
 * @param threadsParGpu GPUごとのスレッド数
 */
Processor::Processor(
    std::string model, std::vector<int32_t> gpus, int32_t batchSize,
    bool fp16, bool deterministic, int32_t threadsParGpu)
    : _mutex(),
      _executors() {
  for (auto gpu : gpus) {
    for (int32_t i = 0; i < threadsParGpu; i++) {
      _executors.push_back(
          std::make_unique<Executor>(model, gpu, batchSize, fp16, deterministic));
    }
  }
}

/**
 * 推論を実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 入出力データの数
 */
void Processor::execute(float* inputs, float* outputs, int32_t size) {
  // 次に計算を実行する計算実行オブジェクトを選択する
  int32_t min_index = 0;

  {  // synchronize
    // 最も待機数が少ない計算実行オブジェクトを探す
    std::unique_lock<std::mutex> lock(_mutex);
    int32_t min_count = _executors[0]->getWaitingCount();

    for (int32_t i = 1; i < _executors.size(); i++) {
      int32_t count = _executors[i]->getWaitingCount();

      if (count < min_count) {
        min_index = i;
        min_count = count;
      }
    }

    // 予約数を増やす
    _executors[min_index]->addReservedCount(size);
  }

  // 計算を実行する。
  _executors[min_index]->execute(inputs, outputs, size);
}

}  // namespace deepgo
