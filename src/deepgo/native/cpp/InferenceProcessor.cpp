#include "InferenceProcessor.h"

#include "MctsManager.h"
#include "MctsNode.h"
#include "MctsParameter.h"

namespace deepgo {

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
InferenceProcessor::InferenceProcessor(
    std::string model, std::vector<int32_t> gpus, bool fp16, bool deterministic,
    int32_t batchSize, int32_t threadsPerGpu, int32_t cacheSize)
    : _mutex(),
      _executors(),
      _threadSize(static_cast<int32_t>(gpus.size()) * threadsPerGpu),
      _cacheSize(cacheSize),
      _cacheKeys(),
      _cacheResults(),
      _batchSize(batchSize) {
  for (int32_t gpu : gpus) {
    _executors.emplace_back(std::make_unique<InferenceExecutor>(
        model, gpu, fp16, deterministic, batchSize, threadsPerGpu));
  }
}

/**
 * 推論実行を予約する。
 * @param node 推論対象ノード
 * @param callback 推論完了時のコールバック
 */
void InferenceProcessor::submit(MctsNode* node, std::function<void(MctsNode*)> callback) {
  // キャッシュされている推論結果を保存する変数
  InferenceResult cached_result;
  // キャッシュされた推論結果が見つかったかどうかを示すフラグ
  bool cached_result_found = false;
  // 実行する推論実行オブジェクトのインデックス
  int32_t executor_index = 0;

  // 同期処理を行うためのロックを取得する
  {
    std::lock_guard<std::mutex> lock(_mutex);

    // キャッシュされた推論結果があるかを確認する
    // キャッシュされた推論結果がある場合は、その推論結果を保存してフラグを立てる
    // ノードへの適用とコールバック関数の呼び出しはロックの外で行う
    BoardHash hash(&node->getBoard(), node->getNextColor());
    auto it = _cacheResults.find(hash);

    if (it != _cacheResults.end()) {
      cached_result = it->second;
      cached_result_found = true;
    }

    // 最も使用されていない推論実行オブジェクトを選択する
    size_t min_queue_size = _executors[0]->getQueueSize();

    for (size_t i = 1; i < _executors.size(); i++) {
      size_t queue_size = _executors[i]->getQueueSize();

      if (queue_size < min_queue_size) {
        executor_index = static_cast<int32_t>(i);
        min_queue_size = queue_size;
      }
    }
  }

  // キャッシュされた推論結果が見つかった場合
  // ノードに適用してコールバック関数を呼び出す
  if (cached_result_found) {
    node->applyInferenceResult(cached_result);
    callback(node);
    return;
  }

  // 推論終了時のコールバック関数を定義する
  auto exec_callback = [this, node, callback](MctsNode*, const InferenceResult& result) {
    {
      std::lock_guard<std::mutex> lock(_mutex);

      // キャッシュに結果があるかを確認する
      // 結果がない場合は、キャッシュに保存する
      BoardHash hash(&node->getBoard(), node->getNextColor());
      auto it = _cacheResults.find(hash);

      if (it == _cacheResults.end() && _cacheSize > 0) {
        _cacheResults.insert({hash, result});
        _cacheKeys.push(hash);
      }

      // キャッシュサイズを超えた分は古いものから削除する
      while (_cacheSize > 0 && _cacheKeys.size() >= static_cast<size_t>(_cacheSize)) {
        _cacheResults.erase(_cacheKeys.front());
        _cacheKeys.pop();
      }
    }

    // ノードに推論結果を適用する
    node->applyInferenceResult(result);

    // コールバック関数を呼び出す
    callback(node);
  };

  // 推論実行オブジェクトに推論実行を予約する
  _executors[executor_index]->submit(node, exec_callback);
}

/**
 * 指定された盤面の評価値を取得する。
 * @param board 盤面
 * @param color 次に打つ石の色
 * @param komi コミ
 * @param rule ルール
 * @param superko スーパーコウルールを使うならtrue
 * @return 評価値
 */
float InferenceProcessor::predict(
    Board* board, int32_t color, float komi, int32_t rule, bool superko) {
  // 同期評価用に一時的なMCTSノードを作り、通常の推論経路を再利用する
  MctsParameter parameter(
      board->getWidth(), board->getHeight(), komi, rule, superko, 1.0f, 18200.0f);
  MctsManager manager(parameter);
  MctsNode* node = manager.createNode();

  // ノードオブジェクトに盤面を設定する
  node->initialize(board, -1, -1, OPPOSITE(color), 0);

  // 推論処理の終了を待つための同期オブジェクトと条件変数を生成する
  std::mutex mutex;
  std::condition_variable cv;

  // 推論処理を実行してノードの評価値が更新されるのを待つ
  {
    std::unique_lock<std::mutex> lock(mutex);

    submit(node, [&cv](MctsNode*) { cv.notify_one(); });
    cv.wait(lock, [node] { return node->isEvaluated(); });
  }

  return node->getNodeValue();
}

/**
 * 推論を同期実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 評価データの数
 */
void InferenceProcessor::execute(int32_t* inputs, float* outputs, int32_t size) {
  _executors[0]->execute(inputs, outputs, size);
}

}  // namespace deepgo
