#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "Board.h"
#include "BoardHash.h"
#include "InferenceExecutor.h"
#include "InferenceResult.h"

namespace deepgo {

/**
 * 推論実行を管理するクラス。
 */
class InferenceProcessor {
 public:
  /**
   * 推論管理オブジェクトを作成する。
   * @param model モデルファイル
   * @param gpus GPU番号の一覧
   * @param fp16 半精度を使用するならtrue
   * @param deterministic 決定論的に実行するならtrue
   * @param batchSize バッチサイズ
   * @param threadsPerGpu GPUごとのスレッド数
   * @param cacheSize 推論結果のキャッシュサイズ
   */
  InferenceProcessor(
      std::string model, std::vector<int32_t> gpus, bool fp16, bool deterministic,
      int32_t batchSize, int32_t threadsPerGpu, int32_t cacheSize);

  /**
   * 推論実行を予約する。
   * @param node 推論対象ノード
   * @param callback 推論完了時のコールバック
   */
  void submit(MctsNode* node, std::function<void(MctsNode*)> callback);

  /**
   * 指定された盤面の評価値を取得する。
   * @param board 盤面
   * @param color 次に打つ石の色
   * @param komi コミ
   * @param rule ルール
   * @param superko スーパーコウルールを使うならtrue
   * @return 評価値
   */
  float predict(Board* board, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * 推論を同期実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 評価データの数
   */
  void execute(int32_t* inputs, float* outputs, int32_t size);

  /**
   * 推論スレッド数を取得する。
   * @return 推論スレッド数
   */
  inline int32_t getThreadSize() const {
    return _threadSize;
  }

  /**
   * バッチサイズを取得する。
   * @return バッチサイズ
   */
  inline int32_t getBatchSize() const {
    return _batchSize;
  }

 private:
  /**
   * 同期用ミューテックス。
   */
  std::mutex _mutex;

  /**
   * 推論実行オブジェクト。
   */
  std::vector<std::unique_ptr<InferenceExecutor>> _executors;

  /**
   * 推論スレッド数。
   */
  int32_t _threadSize;

  /**
   * キャッシュサイズ。
   */
  int32_t _cacheSize;

  /**
   * キャッシュキーのキュー。
   */
  std::queue<BoardHash> _cacheKeys;

  /**
   * 推論結果キャッシュ。
   */
  std::map<BoardHash, InferenceResult> _cacheResults;

  /**
   * バッチサイズ。
   */
  int32_t _batchSize;
};

}  // namespace deepgo
