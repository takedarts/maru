#pragma once

#include <cstdint>

namespace deepgo {

/**
 * Struct to calculate predicted move probability.
 */
struct Policy {
  /**
   * Creates an object for predicted move probability.
   * @param x X coordinate
   * @param y Y coordinate
   * @param policy Predicted move probability
   * @param visits Number of searches
   */
  Policy(int32_t x, int32_t y, float policy, int32_t visits);

  /**
   * X coordinate of the move.
   */
  int32_t x;

  /**
   * Y coordinate of the move.
   */
  int32_t y;

  /**
   * Predicted move probability.
   */
  float policy;

  /**
   * Number of searches.
   */
  int32_t visits;
};

}  // namespace deepgo
