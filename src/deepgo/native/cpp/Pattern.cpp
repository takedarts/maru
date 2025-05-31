#include "Pattern.h"

#include <cstring>

#include "Config.h"

namespace deepgo {

/**
 * 盤面パターンのオブジェクトを作成する。
 * @param width 盤面の幅
 * @param height 盤面の高さ
 */
Pattern::Pattern(int width, int height)
    : _width(width),
      _height(height),
      _length((width * height - 1) / 16 + 1),
      _values(new int32_t[_length]) {
  memset(_values.get(), 0, sizeof(int32_t) * _length);
}

/**
 * コピーした盤面パターンのオブジェクトを作成する。
 * @param pattern コピー元の盤面パターンのオブジェクト
 */
Pattern::Pattern(const Pattern& pattern)
    : _width(pattern._width),
      _height(pattern._height),
      _length(pattern._length),
      _values(new int32_t[_length]) {
  memcpy(_values.get(), pattern._values.get(), sizeof(int32_t) * _length);
}

/**
 * 石の並びの表現値を初期化する。
 */
void Pattern::clear() {
  memset(_values.get(), 0, sizeof(int32_t) * _length);
}

/**
 * 指定された座標に石を置いた状態に更新する。
 * @param x X座標
 * @param y Y座標
 * @param color 石の色
 */
void Pattern::put(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] |= 1 << shift;
}

/**
 * 指定された座標の石を取り除いた状態に更新する。
 * @param x X座標
 * @param y Y座標
 * @param color 石の色
 */
void Pattern::remove(int32_t x, int32_t y, int32_t color) {
  int32_t index = (y * _width + x) / 16;
  int32_t shift = ((y * _width + x) % 16) * 2 + ((color == BLACK) ? 0 : 1);

  _values[index] &= ~(1 << shift);
}

/**
 * パターンを表現する値を取得する。
 * @return パターンを表現する値
 */
std::vector<int32_t> Pattern::values() {
  std::vector<int32_t> values;

  for (int32_t i = 0; i < _length; i++) {
    values.push_back(_values[i]);
  }

  return values;
}

/**
 * パターンを表現する値をコピーする。
 * @param pattern コピー元のパターン
 */
void Pattern::copyFrom(const Pattern& pattern) {
  memcpy(_values.get(), pattern._values.get(), sizeof(int32_t) * _length);
}

}  // namespace deepgo
