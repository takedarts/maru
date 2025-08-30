#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace deepgo {

/**
 * Class that holds information about board patterns.
 */
class Pattern {
 public:
  /**
   * Create a board pattern object.
   * @param width Board width
   * @param height Board height
   */
  Pattern(int width, int height);

  /**
   * Create a copied board pattern object.
   * @param pattern Source board pattern object to copy from
   */
  Pattern(const Pattern& pattern);

  /**
   * Destroy the board pattern object.
   */
  virtual ~Pattern() = default;

  /**
   * Initialize the representation value of the stone arrangement.
   */
  void clear();

  /**
   * Update the state to place a stone at the specified coordinates.
   * @param x X-coordinate
   * @param y Y-coordinate
   * @param color Stone color
   */
  void put(int32_t x, int32_t y, int32_t color);

  /**
   * Update the state to remove a stone at the specified coordinates.
   * @param x X-coordinate
   * @param y Y-coordinate
   * @param color Stone color
   */
  void remove(int32_t x, int32_t y, int32_t color);

  /**
   * Get the value representing the pattern.
   * @return Value representing the pattern
   */
  std::vector<int32_t> values();

  /**
   * Copy the value representing the pattern.
   * @param pattern Source pattern to copy from
   */
  void copyFrom(const Pattern& pattern);

 private:
  /**
   * Board width.
   */
  int32_t _width;

  /**
   * Board height.
   */
  int32_t _height;

  /**
   * Data length.
   */
  int32_t _length;

  /**
   * Board data.
   */
  std::unique_ptr<int32_t[]> _values;
};

}  // namespace deepgo
