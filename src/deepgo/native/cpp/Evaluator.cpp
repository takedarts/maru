#include "Evaluator.h"

namespace deepgo {

/**
 * 評価結果オブジェクトを作成する。
 * @param processor 推論を実行するオブジェクト
 * @param komi コミの目数
 * @param rule 勝敗判定ルール
 * @param superko スーパーコウルールを適用するならtrue
 */
Evaluator::Evaluator(
  Processor* processor, float komi, int32_t rule, bool superko)
    : _processor(processor),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _policies(),
      _value(0.0),
      _evaluated(false) {
}

/**
 * モデルによる評価結果をクリアする。
 */
void Evaluator::clear() {
  _policies.clear();
  _value = 0.0;
  _evaluated = false;
}

/**
 * モデルによる評価を実行する。
 * @param board 評価対象の盤面
 * @param color 評価対象の石の色
 */
void Evaluator::evaluate(Board* board, int32_t color) {
  // 評価済みなら何もしない。
  if (_evaluated) {
    return;
  }

  // 盤面の幅と高さを取得する。
  int32_t width = board->getWidth();
  int32_t height = board->getHeight();

  // 現在の盤面の評価を実行する。
  float inputs[MODEL_INPUT_SIZE];
  float outputs[MODEL_OUTPUT_SIZE];

  board->getInputs(inputs, color, _komi, _rule, _superko);
  _processor->execute(inputs, outputs, 1);

  // 候補手の一覧を作成する
  std::unique_ptr<int32_t[]> enableds(new int32_t[width * height]);
  std::unique_ptr<int32_t[]> territories(new int32_t[width * height]);
  int32_t offset_x = (MODEL_SIZE - width) / 2;
  int32_t offset_y = (MODEL_SIZE - height) / 2;

  board->getEnableds(enableds.get(), color, true);
  board->getTerritories(territories.get(), color);

  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      int32_t board_index = y * width + x;
      int32_t model_index = (offset_y + y) * MODEL_SIZE + (offset_x + x);

      if (enableds[board_index] == 1 && territories[board_index] == EMPTY) {
        _policies.emplace_back(x, y, outputs[model_index], 0);
      }
    }
  }

  // 評価値を取得する。
  _value = outputs[MODEL_PREDICTIONS * MODEL_SIZE * MODEL_SIZE + 0] * 2 - 1;

  // 白番の場合は黒白の評価値を反転する。
  if (color == WHITE) {
    _value = -_value;
  }

  // 評価済みのフラグを立てる。
  _evaluated = true;
}

/**
 * モデルによる評価結果が設定されていればtrueを返す。
 * @return モデルによる評価結果が設定されていればtrue
 */
bool Evaluator::isEvaluated() {
  return _evaluated;
}

/**
 * モデルによる推論結果の予測候補手の一覧を取得する。
 * @return 予測候補手の一覧
 */
std::vector<Policy> Evaluator::getPolicies() {
  return _policies;
}

/**
 * モデルによる推論結果の評価値を取得する。
 * @return モデルによる推論結果の評価値
 */
float Evaluator::getValue() {
  return _value;
}

}  // namespace deepgo
