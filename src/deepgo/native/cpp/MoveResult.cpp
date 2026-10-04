#include "MoveResult.h"

#include <stdexcept>

namespace deepgo {

/**
 * Create an empty move result.
 */
MoveResult::MoveResult()
    : _move(),
      _captured(0),
      _capturedDirections({false, false, false, false}),
      _previousKo({-1, -1}) {
}

/**
 * Create a move result.
 * @param move Move information
 * @param captured Number of captured stones
 * @param capturedDirections Captured directions (up, right, down, left)
 * @param previousKo Ko coordinate before the move
 */
MoveResult::MoveResult(
    const Move& move,
    int32_t captured,
    const std::array<bool, 4>& capturedDirections,
    const std::pair<int32_t, int32_t>& previousKo)
    : _move(move),
      _captured(captured),
      _capturedDirections(capturedDirections),
      _previousKo(previousKo) {
}

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
MoveResult::MoveResult(
    const Move& move,
    int32_t captured,
    bool capturedUp,
    bool capturedRight,
    bool capturedDown,
    bool capturedLeft,
    const std::pair<int32_t, int32_t>& previousKo)
    : MoveResult(
          move,
          captured,
          {capturedUp, capturedRight, capturedDown, capturedLeft},
          previousKo) {
}

/**
 * Get the played X coordinate.
 * @return X coordinate
 */
int32_t MoveResult::getX() const {
  return _move.getX();
}

/**
 * Get the played Y coordinate.
 * @return Y coordinate
 */
int32_t MoveResult::getY() const {
  return _move.getY();
}

/**
 * Get the color of the played stone.
 * @return Stone color
 */
int32_t MoveResult::getColor() const {
  return _move.getColor();
}

/**
 * Get the number of captured stones.
 * @return Number of captured stones
 */
int32_t MoveResult::getCaptured() const {
  return _captured;
}

/**
 * Check whether stones were captured in the specified direction.
 * @param direction Direction index (up, right, down, left)
 * @return True if stones were captured in this direction
 */
bool MoveResult::getCapturedDirection(int32_t direction) const {
  if (direction < 0 || direction >= static_cast<int32_t>(_capturedDirections.size())) {
    throw std::invalid_argument("Captured direction is out of range.");
  }

  return _capturedDirections[direction];
}

/**
 * Get the ko coordinate before the move.
 * @return Ko coordinate before the move
 */
std::pair<int32_t, int32_t> MoveResult::getPreviousKo() const {
  return _previousKo;
}

}  // namespace deepgo
