#pragma once

#include <cstdint>
#include <vector>

namespace deepgo {

/**
 * Candidate move class.
 */
class Candidate {
 public:
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
  Candidate(
      int32_t x, int32_t y, int32_t color,
      int32_t visits, int32_t playouts, float policy, float value,
      std::vector<std::pair<int32_t, int32_t>> variations);

  /**
   * Destroys the instance.
   */
  virtual ~Candidate() = default;

  /**
   * Gets the x coordinate.
   * @return x coordinate
   */
  int32_t getX() const;

  /**
   * Gets the y coordinate.
   * @return y coordinate
   */
  int32_t getY() const;

  /**
   * Gets the stone color.
   * @return Stone color
   */
  int32_t getColor() const;

  /**
   * Gets the number of visits.
   * @return Number of visits
   */
  int32_t getVisits() const;

  /**
   * Gets the number of playouts.
   * @return Number of playouts
   */
  int32_t getPlayouts() const;

  /**
   * Gets the predicted move probability.
   * @return Predicted move probability
   */
  float getPolicy() const;

  /**
   * Gets the evaluation value.
   * @return Evaluation value
   */
  float getValue() const;

  /**
   * Gets the predicted sequence.
   * @return Predicted sequence
   */
  std::vector<std::pair<int32_t, int32_t>> getVariations() const;

 private:
  /**
   * x coordinate.
   */
  int32_t _x;
  /**
   * y coordinate.
   */
  int32_t _y;

  /**
   * Stone color.
   */
  int32_t _color;

  /**
   * Number of visits.
   */
  int32_t _visits;

  /**
   * Number of playouts.
   */
  int32_t _playouts;

  /**
   * Predicted move probability.
   */
  float _policy;

  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Predicted sequence.
   */
  std::vector<std::pair<int32_t, int32_t>> _variations;
};

}  // namespace deepgo
