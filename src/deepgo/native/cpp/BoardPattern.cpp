#include "BoardPattern.h"

#include <cstring>

#include "Config.h"

namespace deepgo {

/**
 * Creates a board pattern object.
 * @param width Board width
 * @param height Board height
 */
BoardPattern::BoardPattern(int width, int height)
    : _width(width),
      _height(height),
      _length((width * height - 1) / 16 + 1),
      _values(_length, 0) {
}

/**
 * Creates a copy of the board pattern object.
 * @param pattern Source board pattern object
 */
BoardPattern::BoardPattern(const BoardPattern& pattern)
    : _width(pattern._width),
      _height(pattern._height),
      _length(pattern._length),
      _values(pattern._values) {
}

/**
 * Initializes the stone arrangement representation value.
 */
void BoardPattern::clear() {
  std::fill(_values.begin(), _values.end(), 0);
}

/**
 * Updates the state with a stone placed at the specified position.
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Stone color
 */
void BoardPattern::put(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] |= 1 << shift;
}

/**
 * Updates the state with a stone removed from the specified position.
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Stone color
 */
void BoardPattern::remove(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] &= ~(1 << shift);
}

/**
 * Returns the values representing the pattern.
 * @return Values representing the pattern
 */
std::vector<int32_t> BoardPattern::values() {
  std::vector<int32_t> values;

  for (int32_t i = 0; i < _length; i++) {
    values.push_back(_values[i]);
  }

  return values;
}

/**
 * Copies the values representing the pattern.
 * @param pattern Source pattern to copy from
 */
void BoardPattern::copyFrom(const BoardPattern& pattern) {
  std::copy(pattern._values.begin(), pattern._values.end(), _values.begin());
}

}  // namespace deepgo
