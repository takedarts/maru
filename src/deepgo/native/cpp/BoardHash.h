#pragma once

#include <cstdint>

#include "Board.h"

namespace deepgo {

/**
 * Class that manages the hash value of a board object.
 */
class BoardHash {
 public:
  /**
   * Creates an object that manages the hash value of a board object.
   * @param board Board object
   */
  explicit BoardHash(const Board* board);

  /**
   * Creates a copy of the object that manages the hash value of a board object.
   * @param boardHash Source object managing the board hash value
   */
  BoardHash(const BoardHash& boardHash) = default;

  /**
   * Destroys the object that manages the hash value of a board object.
   */
  virtual ~BoardHash() = default;

  /**
   * Compares two objects that manage board hash values.
   * @param other The other board hash object to compare against
   * @return true if this object is less than other
   */
  inline bool operator<(const BoardHash& other) const {
    if (_hash != other._hash) {
      return _hash < other._hash;
    }

    if (_size != other._size) {
      return _size < other._size;
    }

    for (int32_t i = 0; i < BITBOARD_SIZE; i++) {
      if (_blackBitBoard[i] != other._blackBitBoard[i]) {
        return _blackBitBoard[i] < other._blackBitBoard[i];
      }
    }

    for (int32_t i = 0; i < BITBOARD_SIZE; i++) {
      if (_whiteBitBoard[i] != other._whiteBitBoard[i]) {
        return _whiteBitBoard[i] < other._whiteBitBoard[i];
      }
    }

    return false;
  }

 private:
  /**
   * Hash value of the board.
   */
  uint64_t _hash;

  /**
   * Board size.
   */
  uint32_t _size;

  /**
   * Bitboard representing black stone positions.
   */
  std::array<uint64_t, BITBOARD_SIZE> _blackBitBoard;

  /**
   * Bitboard representing white stone positions.
   */
  std::array<uint64_t, BITBOARD_SIZE> _whiteBitBoard;
};

}  // namespace deepgo
