#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "Config.h"

namespace deepgo {

/**
 * 着手の情報を管理するクラス。
 */
class Move {
 public:
  /**
   * パスの着手値を生成する。
   * @param color 置いた石の色
   * @return パスの着手値
   */
  inline static Move createPassMove(int8_t color) {
    return Move(-1, -1, color);
  }

  /**
   * 着手オブジェクトを作成する。
   * @param x 置いた石のX座標
   * @param y 置いた石のY座標
   * @param color 置いた石の色
   */
  Move(int8_t x, int8_t y, int8_t color);

  /**
   * 着手オブジェクトをコピーする。
   * @param other コピー元の着手オブジェクト
   */
  Move(const Move& other) = default;

  /**
   * 着手オブジェクトを作成する。
   * 不正な着手を表すオブジェクトを作成する。
   */
  Move();

  /**
   * 着手オブジェクトを破棄する。
   */
  virtual ~Move() = default;

  /**
   * 着手オブジェクトの文字列表現を取得する。
   * @return 着手オブジェクトの文字列表現
   */
  std::string toString() const;

  /**
   * X座標を取得する。
   * @return X座標
   */
  inline int8_t getX() const {
    return _x;
  }

  /**
   * Y座標を取得する。
   * @return Y座標
   */
  inline int8_t getY() const {
    return _y;
  }

  /**
   * 置いた石の色を取得する。
   * @return 石の色
   */
  inline int8_t getColor() const {
    return _color;
  }

  /**
   * 着手オブジェクトが有効な値を持っているかどうかを返す。
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @return 有効な値を持っているならtrue
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
   * 着手オブジェクトがパスを表しているかどうかを返す。
   * @return パスを表しているならtrue
   */
  inline bool isPass() const {
    return _x == -1 && _y == -1;
  }

  /**
   * 着手オブジェクトの文字列表現を出力ストリームに書き込む。
   * @param os 出力ストリーム
   * @param move 着手オブジェクト
   * @return 出力ストリーム
   */
  friend std::ostream& operator<<(std::ostream& os, const Move& move) {
    os << move.toString();
    return os;
  }

 private:
  /**
   * X座標。
   */
  int8_t _x;

  /**
   * Y座標。
   */
  int8_t _y;

  /**
   * 置いた石の色。
   */
  int8_t _color;
};

// 無効な着手値
const Move MOVE_INVALID = Move();

}  // namespace deepgo
