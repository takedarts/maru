#pragma once

#include <cstdint>
#include <set>

namespace deepgo {

/**
 * 連の情報を保持する構造体。
 */
struct BoardRen {
  /**
   * 連のオブジェクトを生成する。
   */
  BoardRen();

  /**
   * コピーした連のオブジェクトを生成する。
   */
  BoardRen(const BoardRen& ren) = default;

  /**
   * 連のオブジェクトを破棄する。
   */
  virtual ~BoardRen() = default;

  /**
   * 石の色。
   */
  int32_t color;

  /**
   * 石の座標一覧。
   */
  std::set<int32_t> positions;

  /**
   * ダメの座標一覧。
   */
  std::set<int32_t> spaces;

  /**
   * 隣接している空き領域の一覧。
   */
  std::set<int32_t> areas;

  /**
   * シチョウであればtrue。
   */
  bool shicho;

  /**
   * 生きが確定していればtrue。
   * この連に隣接する2つ以上の領域が地となっていれば生きと確定する。
   */
  bool fixed;
};

}  // namespace deepgo
