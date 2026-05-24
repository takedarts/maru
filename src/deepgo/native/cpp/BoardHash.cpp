#include "BoardHash.h"

namespace deepgo {

/**
 * 盤面オブジェクトのハッシュ値を管理するオブジェクトを作成する。
 * @param board 盤面オブジェクト
 * @param color 手番
 */
BoardHash::BoardHash(const Board* board, int32_t color) {
  _hash = board->_hash;

  for (int i = 0; i < BITBOARD_SIZE; i++) {
    _bitBoard[i] = board->_bitBoard[i];
  }

  _koIndex = board->_koIndex;
  _koColor = board->_koColor;
  _color = color;
}

}  // namespace deepgo
