#include <sstream>

#include "Policy.h"

namespace deepgo {

/**
 * 予測着手確率のオブジェクトを作成する。
 * @param move 着手座標
 * @param probability 予測着手確率
 * @param visits 探索回数
 */
Policy::Policy(Move move, float probability, int32_t visits)
    : _move(move),
      _probability(probability),
      _visits(visits) {
}

/**
 * 予測着手確率のオブジェクトを作成する。
 */
Policy::Policy()
    : _move(Move()),
      _probability(0.0f),
      _visits(0) {
}

/**
 * 予測着手確率オブジェクトの文字列表現を取得する。
 * @return 予測着手確率オブジェクトの文字列表現
 */
std::string Policy::toString() const {
  std::stringstream ss;

  ss << "Policy(move=" << _move
     << ", probability=" << _probability
     << ", visits=" << _visits
     << ")";

  return ss.str();
}

}  // namespace deepgo
