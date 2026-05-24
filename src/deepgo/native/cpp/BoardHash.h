#pragma once

#include <cstdint>

#include "Board.h"

namespace deepgo {

/**
 * 盤面オブジェクトのハッシュ値を管理するクラス。
 */
class BoardHash {
 public:
  /**
   * 盤面オブジェクトのハッシュ値を管理するオブジェクトを作成する。
   * @param board 盤面オブジェクト
   */
  BoardHash(const Board* board, int32_t color = EMPTY);

  /**
   * コピーした盤面オブジェクトのハッシュ値を管理するオブジェクトを作成する。
   * @param boardHash コピー元の盤面オブジェクトのハッシュ値を管理するオブジェクト
   */
  BoardHash(const BoardHash& boardHash) = default;

  /**
   * 盤面オブジェクトのハッシュ値を管理するオブジェクトを破棄する。
   */
  virtual ~BoardHash() = default;

  /**
   * 盤面オブジェクトのハッシュ値を管理するオブジェクトを比較する。
   * @param other 比較対象の盤面オブジェクトのハッシュ値を管理するオブジェクト
   * @return 盤面オブジェクトのハッシュ値を管理するオブジェクトがotherより小さいならtrue
   */
  inline bool operator<(const BoardHash& other) const {
    if (_hash != other._hash) {
      return _hash < other._hash;
    }

    for (int i = 0; i < BITBOARD_SIZE; i++) {
      if (_bitBoard[i] != other._bitBoard[i]) {
        return _bitBoard[i] < other._bitBoard[i];
      }
    }

    if (_koIndex != other._koIndex) {
      return _koIndex < other._koIndex;
    }

    if (_koColor != other._koColor) {
      return _koColor < other._koColor;
    }

    return _color < other._color;
  }

 private:
  /**
   * 盤面のハッシュ値。
   */
  uint64_t _hash;

  /**
   * 石が置かれている場所を表すビットボード。
   */
  uint64_t _bitBoard[BITBOARD_SIZE];

  /**
   * コウが発生している場所。
   */
  int32_t _koIndex;

  /**
   * コウの対象となる色。
   */
  int32_t _koColor;

  /**
   * 手番。
   */
  int32_t _color;
};

}  // namespace deepgo
