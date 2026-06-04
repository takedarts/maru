#include "MoveHistory.h"

namespace deepgo {

/**
 * Creates an object that holds the move history.
 */
MoveHistory::MoveHistory()
    : _index(0),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = MOVE_INVALID;
  }
}

/**
 * Creates an object that holds a copy of the move history.
 * @param history source move history to copy from
 */
MoveHistory::MoveHistory(const MoveHistory& history)
    : _index(history._index),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = history._moves[i];
  }
}

/**
 * Clears the history.
 */
void MoveHistory::clearMoves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = MOVE_INVALID;
  }
}

/**
 * Adds a move.
 * @param move move to add
 */
void MoveHistory::addMove(Move move) {
  _moves[_index] = move;
  _index = (_index + 1) % 3;
}

/**
 * Returns the move history.
 * @return move history
 */
std::vector<Move> MoveHistory::getMoves() const {
  std::vector<Move> moves;

  for (int32_t i = 0; i < 3; i++) {
    moves.push_back(_moves[(_index + i) % 3]);
  }

  return moves;
}

}  // namespace deepgo
