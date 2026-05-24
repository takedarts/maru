#include "BoardHash.h"

namespace deepgo {

/**
 * Creates an object that manages the hash value of a board object.
 * @param board Board object
 * @param color Current turn
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
