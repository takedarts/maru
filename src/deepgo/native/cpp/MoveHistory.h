#pragma once

#include <cstdint>
#include <vector>

#include "Move.h"

namespace deepgo {

/**
 * 着手履歴を保持するクラス。
 * 着手履歴は、直近の3手分を保持する。
 */
class MoveHistory {
 public:
  /**
   * 着手履歴を保持するオブジェクトを生成する。
   */
  MoveHistory();

  /**
   * コピーした着手履歴を保持するオブジェクトを生成する。
   * @param history コピー元の着手履歴
   */
  MoveHistory(const MoveHistory& history);

  /**
   * オブジェクトを破棄する。
   */
  virtual ~MoveHistory() = default;

  /**
   * 履歴を初期化する。
   */
  void clearMoves();

  /**
   * 着手座標を追加する。
   * @param move 着手座標
   */
  void addMove(Move move);

  /**
   * 着手履歴を取得する。
   * @return 着手履歴
   */
  std::vector<Move> getMoves() const;

 private:
  /**
   * 値を追加する位置。
   */
  int32_t _index;

  /**
   * 着手座標の一覧
   */
  Move _moves[3];
};

}  // namespace deepgo
