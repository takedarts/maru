#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "Move.h"

namespace deepgo {

/**
 * Class managing the result of a move.
 */
class MoveResult {
 public:
  /**
   * Create an empty move result.
   */
  MoveResult();

  /**
   * Create a move result.
   * @param move Move information
   * @param captured Number of captured stones
   * @param capturedDirections Captured directions (up, right, down, left)
   * @param previousKo Ko coordinate before the move
   */
  MoveResult(
      const Move& move,
      int32_t captured,
      const std::array<bool, 4>& capturedDirections,
      const std::pair<int32_t, int32_t>& previousKo);

  /**
   * Create a move result.
   * @param move Move information
   * @param captured Number of captured stones
   * @param capturedUp True if stones were captured above
   * @param capturedRight True if stones were captured to the right
   * @param capturedDown True if stones were captured below
   * @param capturedLeft True if stones were captured to the left
   * @param previousKo Ko coordinate before the move
   */
  MoveResult(
      const Move& move,
      int32_t captured,
      bool capturedUp,
      bool capturedRight,
      bool capturedDown,
      bool capturedLeft,
      const std::pair<int32_t, int32_t>& previousKo);

  /**
   * Get the played X coordinate.
   * @return X coordinate
   */
  int32_t getX() const;

  /**
   * Get the played Y coordinate.
   * @return Y coordinate
   */
  int32_t getY() const;

  /**
   * Get the color of the played stone.
   * @return Stone color
   */
  int32_t getColor() const;

  /**
   * Get the number of captured stones.
   * @return Number of captured stones
   */
  int32_t getCaptured() const;

  /**
   * Check whether stones were captured in the specified direction.
   * @param direction Direction index (up, right, down, left)
   * @return True if stones were captured in this direction
   */
  bool getCapturedDirection(int32_t direction) const;

  /**
   * Get the ko coordinate before the move.
   * @return Ko coordinate before the move
   */
  std::pair<int32_t, int32_t> getPreviousKo() const;

 private:
  /** Move information. */
  Move _move;

  /** Number of captured stones. */
  int32_t _captured;

  /** Captured directions (up, right, down, left). */
  std::array<bool, 4> _capturedDirections;

  /** Ko coordinate before the move. */
  std::pair<int32_t, int32_t> _previousKo;
};

}  // namespace deepgo
