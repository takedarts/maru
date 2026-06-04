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
  BoardHash(const Board* board, int32_t color = EMPTY);

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

    for (int i = 0; i < BITBOARD_SIZE; i++) {
      if (_bitBoard[i] != other._bitBoard[i]) {
        return _bitBoard[i] < other._bitBoard[i];
      }
    }

    if (_koIndex != other._koIndex) {
      return _koIndex < other._koIndex;
    }

    if (_koColor != other._koColor) {
      return _koColor < other._koColor;
    }

    return _color < other._color;
  }

 private:
  /**
   * Hash value of the board.
   */
  uint64_t _hash;

  /**
   * Bitboard representing where stones are placed.
   */
  uint64_t _bitBoard[BITBOARD_SIZE];

  /**
   * Position where ko is in effect.
   */
  int32_t _koIndex;

  /**
   * Color subject to the ko restriction.
   */
  int32_t _koColor;

  /**
   * Current turn.
   */
  int32_t _color;
};

}  // namespace deepgo
