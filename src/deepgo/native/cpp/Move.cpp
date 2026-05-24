#include "Move.h"

#include <sstream>

namespace deepgo {

/**
 * Creates a move object.
 * @param x x coordinate of the placed stone
 * @param y y coordinate of the placed stone
 * @param color color of the placed stone
 */
Move::Move(int8_t x, int8_t y, int8_t color)
    : _x(x),
      _y(y),
      _color(color) {
}

/**
 * Creates a move object.
 * Creates an object representing an invalid move.
 */
Move::Move()
    : _x(-2),
      _y(-2),
      _color(EMPTY) {
}

/**
 * Returns the string representation of the move object.
 * @return string representation of the move object
 */
std::string Move::toString() const {
  std::stringstream ss;
  char color_char = (_color == BLACK)   ? 'B'
                    : (_color == WHITE) ? 'W'
                                        : 'E';

  ss << "x=" << static_cast<int32_t>(_x)
     << ", y=" << static_cast<int32_t>(_y)
     << ", color=" << color_char;

  return ss.str();
}

}  // namespace deepgo
