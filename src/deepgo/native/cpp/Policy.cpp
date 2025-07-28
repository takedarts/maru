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

}  // namespace deepgo
