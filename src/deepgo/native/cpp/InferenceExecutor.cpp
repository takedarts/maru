#include "InferenceExecutor.h"

#include <algorithm>

#include "Config.h"
#include "MctsNode.h"
#include "Move.h"

namespace deepgo {

/**
 * 指定された座標のPolicyインデックスを取得する。
 * @param board 盤面
 * @param x X座標
 * @param y Y座標
 * @return Policyインデックス
 */
static int32_t getPolicyIndex(const Board* board, int32_t x, int32_t y) {
  // 可変サイズの盤面をモデル入力の中央へ配置したときのインデックスを計算する
  int32_t offset_x = (MODEL_SIZE - board->getWidth()) / 2;
  int32_t offset_y = (MODEL_SIZE - board->getHeight()) / 2;

  return (offset_y + y) * MODEL_SIZE + (offset_x + x);
}

/**
 * 推論実行オブジェクトを作成する。
 * @param model モデルファイル
 * @param gpu GPU番号
 * @param fp16 半精度を使用するならtrue
 * @param deterministic 決定論的に実行するならtrue
 * @param batchSize バッチサイズ
 * @param threads 実行スレッド数
 */
InferenceExecutor::InferenceExecutor(
    std::string model, int32_t gpu, bool fp16, bool deterministic,
    int32_t batchSize, int32_t threads)
    : _modelMutex(),
      _threadMutex(),
      _condition(),
      _model(nullptr),
      _modelFile(model),
      _gpu(gpu),
      _fp16(fp16),
      _deterministic(deterministic),
      _batchSize(batchSize),
      _threads(),
      _terminated(false),
      _queue() {
  for (int32_t i = 0; i < threads; i++) {
    _threads.emplace_back(&InferenceExecutor::_run, this);
  }
}

/**
 * 推論実行オブジェクトを破棄する。
 */
InferenceExecutor::~InferenceExecutor() {
  // 推論処理を終了する
  {
    std::lock_guard<std::mutex> lock(_threadMutex);
    _terminated = true;
  }

  _condition.notify_all();

  // スレッドの終了を待機する
  for (auto& thread : _threads) {
    thread.join();
  }

  // 推論モデルオブジェクトを破棄する
  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    if (_model != nullptr) {
      delete _model;
      _model = nullptr;
    }
  }
}

/**
 * 推論実行を予約する。
 * @param node 推論対象ノード
 * @param callback 推論完了時のコールバック
 */
void InferenceExecutor::submit(MctsNode* node, InferenceExecutorCallback callback) {
  std::lock_guard<std::mutex> lock(_threadMutex);
  _queue.emplace_back(node, callback);
  _condition.notify_one();
}

/**
 * 推論を同期実行する。
 * @param inputs 入力データ
 * @param outputs 出力データ
 * @param size 評価データの数
 */
void InferenceExecutor::execute(int32_t* inputs, float* outputs, int32_t size) {
  // 使用するデバイスを設定する
  torch::DeviceGuard device_guard(InferenceModel::getDevice(_gpu));

  // モデルオブジェクトを取得する
  // モデルオブジェクトが作成されていない場合は作成する
  InferenceModel* model = nullptr;

  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    // モデルは初回実行時に遅延ロードし、以降の推論で共有する
    if (_model == nullptr) {
      _model = new InferenceModel(_modelFile, _gpu, _fp16, _deterministic);
    }

    model = _model;
  }

  // CPU以外で実行する場合は指定されたバッチサイズで推論を実行する
  if (!_model->isCpu()) {
    std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
    std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

    std::copy(inputs, inputs + (size * MODEL_INPUT_PACK_SIZE), input_buffer.data());
    model->forward(input_buffer.data(), output_buffer.data(), _batchSize);
    std::copy(output_buffer.data(), output_buffer.data() + (size * MODEL_OUTPUT_SIZE), outputs);
  } else {
    model->forward(inputs, outputs, size);
  }
}

/**
 * 待機中の推論数を取得する。
 * @return 待機中の推論数
 */
int32_t InferenceExecutor::getQueueSize() {
  std::lock_guard<std::mutex> lock(_threadMutex);
  return static_cast<int32_t>(_queue.size());
}

/**
 * 推論スレッドで実行される処理。
 */
void InferenceExecutor::_run() {
  // 使用するデバイスを設定する
  torch::DeviceGuard device_guard(InferenceModel::getDevice(_gpu));

  // 入力データとマスクデータと出力データのバッファを作成する
  std::vector<int32_t> input_buffer(_batchSize * MODEL_INPUT_PACK_SIZE);
  std::vector<float> output_buffer(_batchSize * MODEL_OUTPUT_SIZE);

  // モデルオブジェクトが作成されていない場合は作成する
  {
    std::lock_guard<std::mutex> lock(_modelMutex);

    if (_model == nullptr) {
      _model = new InferenceModel(_modelFile, _gpu, _fp16, _deterministic);
    }
  }

  // キューから推論実行の予約を取り出してバッチを作成し、
  // 推論を実行して結果をノードに適用する処理を繰り返す
  while (true) {
    // キューから推論実行の予約を取り出してバッチを作成する
    std::vector<std::pair<MctsNode*, InferenceExecutorCallback>> batch;

    {
      // 同期処理を行うためのロックを取得する
      std::unique_lock<std::mutex> lock(_threadMutex);

      // 推論実行の予約がない場合は待機する
      // [停止] 停止要求があり、キューが空である場合は待機を終了する
      // [推論実行] 推論実行の予約がある場合は待機を終了する
      _condition.wait(lock, [this] {
        if (_terminated && _queue.empty()) {
          return true;
        } else if (!_queue.empty()) {
          return true;
        } else {
          return false;
        }
      });

      // 推論処理を終了するならばループを抜ける
      if (_terminated && _queue.empty()) {
        break;
      }

      // キューから最大バッチサイズ分を取り出して推論バッチを作る
      while (!_queue.empty() && batch.size() < static_cast<size_t>(_batchSize)) {
        batch.push_back(_queue.back());
        _queue.pop_back();
      }
    }

    // 入力データのバッファを初期化する
    std::fill(input_buffer.begin(), input_buffer.end(), 0);

    // 各ノードの局面をモデル入力へ変換する
    for (size_t i = 0; i < batch.size(); i++) {
      MctsNode* node = batch[i].first;
      int32_t* inputs = input_buffer.data() + (i * MODEL_INPUT_PACK_SIZE);
      int32_t color = node->getNextColor();
      Board board(node->getBoard());

      board.getInputs(inputs, color, node->getKomi(), node->getRule(), node->getSuperko());
    }

    // 推論を実行する
    // CPUで実行する場合はバッチサイズを実際のバッチサイズに合わせる
    // CPU以外で実行する場合は指定されたバッチサイズで推論を実行する
    int32_t batch_size = (_model->isCpu()) ? static_cast<int32_t>(batch.size()) : _batchSize;

    _model->forward(input_buffer.data(), output_buffer.data(), batch_size);

    // 推論結果をノードに適用してコールバック関数を呼び出す
    for (size_t i = 0; i < batch.size(); i++) {
      float* outputs = output_buffer.data() + (i * MODEL_OUTPUT_SIZE);
      MctsNode* node = batch[i].first;
      InferenceExecutorCallback callback = batch[i].second;
      Board board(node->getBoard());
      int32_t color = node->getNextColor();
      int32_t width = board.getWidth();
      int32_t height = board.getHeight();

      // Valueの推論結果を作成する
      float value = outputs[MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE] * 2.0f - 1.0f;

      if (color == WHITE) {
        value = -value;
      }

      // 着手の対象となる座標を取得する
      std::vector<int32_t> enableds(width * height);
      std::vector<int32_t> territories(width * height);

      board.getEnableds(enableds.data(), color, true);
      board.getTerritories(territories.data(), color);

      // Policyの確率の合計を計算する
      float total_probability = 0.0f;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;

          if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
            total_probability += outputs[getPolicyIndex(&board, x, y)];
          }
        }
      }

      // Policyの推論結果を作成する
      std::vector<Policy> policies;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;

          if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
            float probability =
                outputs[getPolicyIndex(&board, x, y)] / (total_probability + 1e-6f);

            policies.emplace_back(Move(x, y, color), probability, 0);
          }
        }
      }

      // Territoryの推論結果を作成する
      // 手番が白の場合は0番目と2番目の推論結果を入れ替える
      std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> territory_probs;
      const int8_t black_index = (color == BLACK) ? 2 : 4;
      const int8_t seki_index = 3;
      const int8_t white_index = (color == BLACK) ? 4 : 2;

      std::copy(
          outputs + black_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (black_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 0 * MODEL_SIZE * MODEL_SIZE);

      std::copy(
          outputs + seki_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (seki_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 1 * MODEL_SIZE * MODEL_SIZE);

      std::copy(
          outputs + white_index * MODEL_SIZE * MODEL_SIZE,
          outputs + (white_index + 1) * MODEL_SIZE * MODEL_SIZE,
          territory_probs.begin() + 2 * MODEL_SIZE * MODEL_SIZE);

      // コールバック関数を呼び出す
      callback(node, InferenceResult(value, policies, territory_probs));
    }
  }
}

}  // namespace deepgo
