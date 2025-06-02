#pragma once

#include <cstdint>

namespace deepgo {

/**
 * 予測着手確率を計算する構造体。
 */
struct Policy {
  /**
   * 予測着手確率のオブジェクトを作成する。
   * @param x X座標
   * @param y Y座標
   * @param policy 予測着手確率
   * @param visits 探索回数
   */
  Policy(int32_t x, int32_t y, float policy, int32_t visits);

  /**
   * 優先度を取得する。
   * @return 優先度
   */
  float getPriority() const;

  /**
   * 予測着手確率の比較を行う。
   * @param other 比較対象
   * @return 比較結果
   */
  bool operator<(const Policy& other) const;

  /**
   * 着手座標のX座標。
   */
  int32_t x;

  /**
   * 着手座標のY座標。
   */
  int32_t y;

  /**
   * 予測着手確率。
   */
  float policy;

  /**
   * 探索回数。
   */
  int32_t visits;
};

}  // namespace deepgo
