#include "History.h"

namespace deepgo {

/**
 * Create an object to hold move history.
 */
History::History()
    : _index(0),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = -1;
  }
}

/**
 * Create an object to hold copied move history.
 * @param history Source move history to copy from
 */
History::History(const History& history)
    : _index(history._index),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = history._moves[i];
  }
}

/**
 * Initialize the history.
 */
void History::clear() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = -1;
  }
}

/**
 * Add a move coordinate.
 * @param move Move coordinate
 */
void History::add(int32_t move) {
  _moves[_index] = move;
  _index = (_index + 1) % 3;
}

/**
 * Get the move history.
 * @return Move history
 */
std::vector<int32_t> History::get() {
  std::vector<int32_t> moves;

  for (int32_t i = 0; i < 3; i++) {
    moves.push_back(_moves[(_index + i) % 3]);
  }

  return moves;
}

}  // namespace deepgo
