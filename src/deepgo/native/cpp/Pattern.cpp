#include "Pattern.h"

#include <cstring>

#include "Config.h"

namespace deepgo {

/**
 * Create a board pattern object.
 * @param width Board width
 * @param height Board height
 */
Pattern::Pattern(int width, int height)
    : _width(width),
      _height(height),
      _length((width * height - 1) / 16 + 1),
      _values(new int32_t[_length]) {
  memset(_values.get(), 0, sizeof(int32_t) * _length);
}

/**
 * Create a copied board pattern object.
 * @param pattern Source board pattern object to copy from
 */
Pattern::Pattern(const Pattern& pattern)
    : _width(pattern._width),
      _height(pattern._height),
      _length(pattern._length),
      _values(new int32_t[_length]) {
  memcpy(_values.get(), pattern._values.get(), sizeof(int32_t) * _length);
}

/**
 * Initialize the representation value of the stone arrangement.
 */
void Pattern::clear() {
  memset(_values.get(), 0, sizeof(int32_t) * _length);
}

/**
 * Update the state to place a stone at the specified coordinates.
 * @param x X-coordinate
 * @param y Y-coordinate
 * @param color Stone color
 */
void Pattern::put(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] |= 1 << shift;
}

/**
 * Update the state to remove a stone at the specified coordinates.
 * @param x X-coordinate
 * @param y Y-coordinate
 * @param color Stone color
 */
void Pattern::remove(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] &= ~(1 << shift);
}

/**
 * Get the value representing the pattern.
 * @return Value representing the pattern
 */
std::vector<int32_t> Pattern::values() {
  std::vector<int32_t> values;

  for (int32_t i = 0; i < _length; i++) {
    values.push_back(_values[i]);
  }

  return values;
}

/**
 * Copy the value representing the pattern.
 * @param pattern Source pattern to copy from
 */
void Pattern::copyFrom(const Pattern& pattern) {
  memcpy(_values.get(), pattern._values.get(), sizeof(int32_t) * _length);
}

}  // namespace deepgo
