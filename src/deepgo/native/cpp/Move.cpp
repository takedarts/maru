#include "Move.h"

#include <sstream>

namespace deepgo {

/**
 * 着手オブジェクトを作成する。
 * @param x 置いた石のX座標
 * @param y 置いた石のY座標
 * @param color 置いた石の色
 */
Move::Move(int8_t x, int8_t y, int8_t color)
    : _x(x),
      _y(y),
      _color(color) {
}

/**
 * 着手オブジェクトを作成する。
 * 不正な着手を表すオブジェクトを作成する。
 */
Move::Move()
    : _x(-2),
      _y(-2),
      _color(EMPTY) {
}

/**
 * 着手オブジェクトの文字列表現を取得する。
 * @return 着手オブジェクトの文字列表現
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
