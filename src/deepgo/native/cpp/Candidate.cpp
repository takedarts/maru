#include "Candidate.h"

#include <iomanip>
#include <sstream>

namespace deepgo {

/**
 * 候補手データを作成する。
 * @param x x座標
 * @param y y座標
 * @param color 石の色
 * @param visits 訪問回数
 * @param playouts プレイアウト数
 * @param policy 予想着手確率
 * @param value 評価値
 * @param variations 予想進行
 * @param territories 予測領域確率
 */
Candidate::Candidate(
    Move move, int32_t visits, int32_t playouts,
    float policy, float value, const std::vector<Move> variations,
    const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories)
    : _move(move),
      _visits(visits),
      _playouts(playouts),
      _policy(policy),
      _value(value),
      _variations(variations),
      _territories(territories) {
}

/**
 * ノードオブジェクトから候補手データを作成する。
 * @param node ノードオブジェクト
 */
Candidate::Candidate(MctsNode* node)
    : _move(node->getMove()),
      _visits(node->getVisits()),
      _playouts(node->getPlayouts()),
      _policy(node->getProbability()),
      _value(node->getMctsValue()),
      _variations(node->getVariations()),
      _territories(node->getTerritories()) {
}

/**
 * 候補手オブジェクトをコピーする。
 * @param other コピー元の候補手オブジェクト
 */
Candidate::Candidate(const Candidate& other)
    : _move(other._move),
      _visits(other._visits),
      _playouts(other._playouts),
      _policy(other._policy),
      _value(other._value),
      _variations(other._variations),
      _territories(other._territories) {
}

/**
 * 候補手オブジェクトを作成する。
 * 不正な候補手を表すオブジェクトを作成する。
 */
Candidate::Candidate()
    : _move(MOVE_INVALID),
      _visits(0),
      _playouts(0),
      _policy(0.0f),
      _value(0.0f),
      _variations(),
      _territories() {
  std::fill(std::begin(_territories), std::end(_territories), 0.0f);
}

/**
 * 候補手の文字列表現を取得する。
 * @return 候補手の文字列表現
 */
std::string Candidate::toString() const {
  std::stringstream ss;

  ss << "Move: (" << _move << ")";
  ss << ", Visits: " << _visits;
  ss << ", Playouts: " << _playouts;
  ss << ", Policy: " << std::fixed << std::setprecision(4) << _policy;
  ss << ", Value: " << std::fixed << std::setprecision(4) << _value;
  ss << ", Variations: [";

  for (size_t i = 0; i < _variations.size(); ++i) {
    const auto& variation = _variations[i];
    ss << "(" << variation << ")";

    if (i < _variations.size() - 1) {
      ss << ", ";
    }
  }

  ss << "]";

  return ss.str();
}

}  // namespace deepgo
