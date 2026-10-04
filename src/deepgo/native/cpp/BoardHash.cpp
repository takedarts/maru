#include "BoardHash.h"

namespace deepgo {

/**
 * Creates an object that manages the hash value of a board object.
 * @param board Board object
 */
BoardHash::BoardHash(const Board* board) {
  _hash = board->_hash;
  _size = board->_width << 16 | board->_height;

  // Copy the board's bitboards for each color
  _blackBitBoard = board->_blackBitBoard;
  _whiteBitBoard = board->_whiteBitBoard;
}

}  // namespace deepgo
