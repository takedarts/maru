#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "Config.h"

namespace deepgo {

/**
 * Class that manages move information.
 */
class Move {
 public:
  /**
   * Creates a pass move.
   * @param color color of the placed stone
   * @return pass move
   */
  inline static Move createPassMove(int8_t color) {
    return Move(-1, -1, color);
  }

  /**
   * Creates a move object.
   * @param x x coordinate of the placed stone
   * @param y y coordinate of the placed stone
   * @param color color of the placed stone
   */
  Move(int8_t x, int8_t y, int8_t color);

  /**
   * Copies a move object.
   * @param other source move object to copy from
   */
  Move(const Move& other) = default;

  /**
   * Creates a move object.
   * Creates an object representing an invalid move.
   */
  Move();

  /**
   * Destructor.
   */
  virtual ~Move() = default;

  /**
   * Returns the string representation of the move object.
   * @return string representation of the move object
   */
  std::string toString() const;

  /**
   * Returns the x coordinate.
   * @return x coordinate
   */
  inline int8_t getX() const {
    return _x;
  }

  /**
   * Returns the y coordinate.
   * @return y coordinate
   */
  inline int8_t getY() const {
    return _y;
  }

  /**
   * Returns the color of the placed stone.
   * @return stone color
   */
  inline int8_t getColor() const {
    return _color;
  }

  /**
   * Returns whether the move object holds a valid value.
   * @param width board width
   * @param height board height
   * @return true if the move holds a valid value
   */
  inline bool isValid(int8_t width, int8_t height) const {
    if (_x < 0 || _x >= width || _y < 0 || _y >= height) {
      return false;
    } else if (_color != BLACK && _color != WHITE) {
      return false;
    } else {
      return true;
    }
  }

  /**
   * Returns whether the move object represents a pass.
   * @return true if the move represents a pass
   */
  inline bool isPass() const {
    return _x == -1 && _y == -1;
  }

  /**
   * Writes the string representation of the move object to an output stream.
   * @param os output stream
   * @param move move object
   * @return output stream
   */
  friend std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << move.toString();
    return os;
  }

 private:
  /**
   * X coordinate.
   */
  int8_t _x;

  /**
   * Y coordinate.
   */
  int8_t _y;

  /**
   * Color of the placed stone.
   */
  int8_t _color;
};

// Invalid move value
const Move MOVE_INVALID = Move();

}  // namespace deepgo
