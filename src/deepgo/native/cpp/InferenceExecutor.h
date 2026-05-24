#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "InferenceModel.h"
#include "InferenceResult.h"

namespace deepgo {

class MctsNode;

using InferenceExecutorCallback =
    std::function<void(MctsNode*, const InferenceResult&)>;

/**
 * 推論実行を非同期に処理するクラス。
 */
class InferenceExecutor {
 public:
  /**
   * 推論実行オブジェクトを作成する。
   * @param model モデルファイル
   * @param gpu GPU番号
   * @param fp16 半精度を使用するならtrue
   * @param deterministic 決定論的に実行するならtrue
   * @param batchSize バッチサイズ
   * @param threads 実行スレッド数
   */
  InferenceExecutor(
      std::string model, int32_t gpu, bool fp16, bool deterministic,
      int32_t batchSize, int32_t threads);

  /**
   * 推論実行オブジェクトを破棄する。
   */
  virtual ~InferenceExecutor();

  /**
   * 推論実行を予約する。
   * @param node 推論対象ノード
   * @param callback 推論完了時のコールバック
   */
  void submit(MctsNode* node, InferenceExecutorCallback callback);

  /**
   * 推論を同期実行する。
   * @param inputs 入力データ
   * @param outputs 出力データ
   * @param size 評価データの数
   */
  void execute(int32_t* inputs, float* outputs, int32_t size);

  /**
   * 待機中の推論数を取得する。
   * @return 待機中の推論数
   */
  int32_t getQueueSize();

 private:
  /**
   * モデル同期用ミューテックス。
   */
  std::mutex _modelMutex;

  /**
   * スレッド同期用ミューテックス。
   */
  std::mutex _threadMutex;

  /**
   * 条件変数。
   */
  std::condition_variable _condition;

  /**
   * 推論モデル。
   */
  InferenceModel* _model;

  /**
   * モデルファイル。
   */
  std::string _modelFile;

  /**
   * GPU番号。
   */
  int32_t _gpu;

  /**
   * 半精度を使用するならtrue。
   */
  bool _fp16;

  /**
   * 決定論的に実行するならtrue。
   */
  bool _deterministic;

  /**
   * バッチサイズ。
   */
  int32_t _batchSize;

  /**
   * 推論スレッド。
   */
  std::vector<std::thread> _threads;

  /**
   * 終了するならtrue。
   */
  bool _terminated;

  /**
   * 推論待ちキュー。
   */
  std::vector<std::pair<MctsNode*, InferenceExecutorCallback>> _queue;

  /**
   * 推論スレッドで実行される処理。
   */
  void _run();
};

}  // namespace deepgo
