#include "MoveHistory.h"

namespace deepgo {

/**
 * 着手履歴を保持するオブジェクトを生成する。
 */
MoveHistory::MoveHistory()
    : _index(0),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = MOVE_INVALID;
  }
}

/**
 * コピーした着手履歴を保持するオブジェクトを生成する。
 * @param history コピー元の着手履歴
 */
MoveHistory::MoveHistory(const MoveHistory& history)
    : _index(history._index),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = history._moves[i];
  }
}

/**
 * 履歴を初期化する。
 */
void MoveHistory::clearMoves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = MOVE_INVALID;
  }
}

/**
 * 着手座標を追加する。
 * @param move 着手座標
 */
void MoveHistory::addMove(Move move) {
  _moves[_index] = move;
  _index = (_index + 1) % 3;
}

/**
 * 着手履歴を取得する。
 * @return 着手履歴
 */
std::vector<Move> MoveHistory::getMoves() const {
  std::vector<Move> moves;

  for (int32_t i = 0; i < 3; i++) {
    moves.push_back(_moves[(_index + i) % 3]);
  }

  return moves;
}

}  // namespace deepgo
