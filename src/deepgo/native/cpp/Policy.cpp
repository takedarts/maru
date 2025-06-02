#include "Policy.h"

namespace deepgo {

/**
 * 予測着手確率のオブジェクトを作成する。
 * @param x X座標
 * @param y Y座標
 * @param policy 予測着手確率
 * @param visits 探索回数
 */
Policy::Policy(int32_t x, int32_t y, float policy, int32_t visits)
    : x(x),
      y(y),
      policy(policy),
      visits(visits) {
}

/**
 * 優先度を取得する。
 * @return 優先度
 */
float Policy::getPriority() const {
  return policy / (visits + 1);
}

/**
 * 予測着手確率の比較を行う。
 * @param other 比較対象
 * @return 比較結果
 */
bool Policy::operator<(const Policy& other) const {
  return getPriority() < other.getPriority();
}

}  // namespace deepgo
