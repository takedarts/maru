#pragma once

#include <cstdint>

#include "BoardHash.h"

namespace deepgo {

/**
 * Class holding an inference cache key.
 */
class InferenceHash {
 public:
  /**
   * Create an inference cache key.
   * @param board Board object
   * @param color Color of the stone to play next
   * @param komi Komi value
   * @param rule Win/loss determination rule
   * @param superko True if the superko rule is applied
   */
  InferenceHash(
      const Board* board, int32_t color, float komi, int32_t rule, bool superko);

  /**
   * Compare cache keys.
   * @param other Cache key to compare against
   * @return True if this key is less than other
   */
  bool operator<(const InferenceHash& other) const;

 private:
  /** Stone arrangement on the board. */
  BoardHash _boardHash;

  /** Color of the next stone to play. */
  int32_t _color;

  /** Ko position active for the player to move. */
  int32_t _koIndex;

  /** Komi converted to a fixed-point value. */
  int32_t _komi;

  /** Win/loss determination rule. */
  int32_t _rule;

  /** True if the superko rule is applied. */
  bool _superko;
};

}  // namespace deepgo
