#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "Move.h"

namespace deepgo {

/**
 * 予測着手確率を計算するクラス。
 */
class Policy {
 public:
  /**
   * 予測着手確率のオブジェクトを作成する。
   * @param move 着手座標
   * @param probability 予測着手確率
   * @param visits 探索回数
   */
  Policy(Move move, float probability, int32_t visits);

  /**
   * 予測着手確率をコピーしたオブジェクトを作成する。
   * @param policy コピー元の予測着手確率
   */
  Policy(const Policy& policy) = default;

  /**
   * 予測着手確率のオブジェクトを作成する。
   */
  Policy();

  /**
   * 予測着手確率オブジェクトを破棄する。
   */
  virtual ~Policy() = default;

  /**
   * 予測着手確率オブジェクトの文字列表現を取得する。
   * @return 予測着手確率オブジェクトの文字列表現
   */
  std::string toString() const;

  /**
   * 着手座標を取得する。
   * @return 着手座標
   */
  inline Move getMove() const {
    return _move;
  }

  /**
   * 予測着手確率を取得する。
   * @return 予測着手確率
   */
  inline float getProbability() const {
    return _probability;
  }

  /**
   * 探索回数を取得する。
   * @return 探索回数
   */
  inline int32_t getVisits() const {
    return _visits;
  }

  /**
   * 探索回数を1増やす。
   */
  inline void incrementVisits() {
    _visits += 1;
  }

  /**
   * 着手予測確率の文字列表現をストリームに出力する。
   * @param os 出力ストリーム
   * @param policy 予測着手確率
   * @return 出力ストリーム
   */
  friend std::ostream& operator<<(std::ostream& os, const Policy& policy) {
    os << policy.toString();
    return os;
  }

 private:
  /**
   * 着手座標。
   */
  Move _move;

  /**
   * 予測着手確率。
   */
  float _probability;

  /**
   * 探索回数。
   */
  int32_t _visits;
};

}  // namespace deepgo
