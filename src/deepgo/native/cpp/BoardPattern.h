#pragma once

#include <cstdint>
#include <vector>

namespace deepgo {

/**
 * Class that holds the pattern information of the board arrangement.
 */
class BoardPattern {
 public:
  /**
   * Creates a board pattern object.
   * @param width Board width
   * @param height Board height
   */
  BoardPattern(int width, int height);

  /**
   * Creates a copy of the board pattern object.
   * @param pattern Source board pattern object
   */
  BoardPattern(const BoardPattern& pattern);

  /**
   * Destroys the board pattern object.
   */
  virtual ~BoardPattern() = default;

  /**
   * Initializes the stone arrangement representation value.
   */
  void clear();

  /**
   * Updates the state with a stone placed at the specified position.
   * @param x X coordinate
   * @param y Y coordinate
   * @param color Stone color
   */
  void put(int32_t x, int32_t y, int32_t color);

  /**
   * Updates the state with a stone removed from the specified position.
   * @param x X coordinate
   * @param y Y coordinate
   * @param color Stone color
   */
  void remove(int32_t x, int32_t y, int32_t color);

  /**
   * Returns the values representing the pattern.
   * @return Values representing the pattern
   */
  std::vector<int32_t> values();

  /**
   * Copies the values representing the pattern.
   * @param pattern Source pattern to copy from
   */
  void copyFrom(const BoardPattern& pattern);

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
  std::vector<int32_t> _values;
};

}  // namespace deepgo
