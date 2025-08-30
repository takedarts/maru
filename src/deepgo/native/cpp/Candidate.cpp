#include "Candidate.h"

namespace deepgo {

/**
 * Creates candidate move data.
 * @param x x coordinate
 * @param y y coordinate
 * @param color Stone color
 * @param visits Number of visits
 * @param playouts Number of playouts
 * @param policy Predicted move probability
 * @param value Evaluation value
 * @param variations Predicted sequence
 */
Candidate::Candidate(
    int32_t x, int32_t y, int32_t color,
    int32_t visits, int32_t playouts, float policy, float value,
    std::vector<std::pair<int32_t, int32_t>> variations)
    : _x(x),
      _y(y),
      _color(color),
      _visits(visits),
      _playouts(playouts),
      _policy(policy),
      _value(value),
      _variations(variations) {
}

/**
 * Gets the x coordinate.
 * @return x coordinate
 */
int32_t Candidate::getX() const {
  return _x;
}

/**
 * Gets the y coordinate.
 * @return y coordinate
 */
int32_t Candidate::getY() const {
  return _y;
}

/**
 * Gets the stone color.
 * @return Stone color
 */
int32_t Candidate::getColor() const {
  return _color;
}

/**
 * Gets the number of visits.
 * @return Number of visits
 */
int32_t Candidate::getVisits() const {
  return _visits;
}

/**
 * Gets the number of playouts.
 * @return Number of playouts
 */
int32_t Candidate::getPlayouts() const {
  return _playouts;
}

/**
 * Gets the predicted move probability.
 * @return Predicted move probability
 */
float Candidate::getPolicy() const {
  return _policy;
}

/**
 * Gets the evaluation value.
 * @return Evaluation value
 */
float Candidate::getValue() const {
  return _value;
}

/**
 * Gets the predicted sequence.
 * @return Predicted sequence
 */
std::vector<std::pair<int32_t, int32_t>> Candidate::getVariations() const {
  return _variations;
}

}  // namespace deepgo
