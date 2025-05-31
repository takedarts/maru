#include "History.h"

namespace deepgo {

/**
 * 着手履歴を保持するオブジェクトを生成する。
 */
History::History()
    : _index(0),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = -1;
  }
}

/**
 * コピーした着手履歴を保持するオブジェクトを生成する。
 * @param history コピー元の着手履歴
 */
History::History(const History& history)
    : _index(history._index),
      _moves() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = history._moves[i];
  }
}

/**
 * 履歴を初期化する。
 */
void History::clear() {
  for (int32_t i = 0; i < 3; i++) {
    _moves[i] = -1;
  }
}

/**
 * 着手座標を追加する。
 * @param move 着手座標
 */
void History::add(int32_t move) {
  _moves[_index] = move;
  _index = (_index + 1) % 3;
}

/**
 * 着手履歴を取得する。
 * @return 着手履歴
 */
std::vector<int32_t> History::get() {
  std::vector<int32_t> moves;

  for (int32_t i = 0; i < 3; i++) {
    moves.push_back(_moves[(_index + i) % 3]);
  }

  return moves;
}

}  // namespace deepgo
