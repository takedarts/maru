#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace deepgo {

/**
 * 盤面のパターンの情報を保持するクラス。
 */
class Pattern {
 public:
  /**
   * 盤面パターンのオブジェクトを作成する。
   * @param width 盤面の幅
   * @param height 盤面の高さ
   */
  Pattern(int width, int height);

  /**
   * コピーした盤面パターンのオブジェクトを作成する。
   * @param pattern コピー元の盤面パターンのオブジェクト
   */
  Pattern(const Pattern& pattern);

  /**
   * 盤面パターンのオブジェクトを破棄する。
   */
  virtual ~Pattern() = default;

  /**
   * 石の並びの表現値を初期化する。
   */
  void clear();

  /**
   * 指定された座標に石を置いた状態に更新する。
   * @param x X座標
   * @param y Y座標
   * @param color 石の色
   */
  void put(int32_t x, int32_t y, int32_t color);

  /**
   * 指定された座標の石を取り除いた状態に更新する。
   * @param x X座標
   * @param y Y座標
   * @param color 石の色
   */
  void remove(int32_t x, int32_t y, int32_t color);

  /**
   * パターンを表現する値を取得する。
   * @return パターンを表現する値
   */
  std::vector<int32_t> values();

  /**
   * パターンを表現する値をコピーする。
   * @param pattern コピー元のパターン
   */
  void copyFrom(const Pattern& pattern);

 private:
  /**
   * 盤面の幅。
   */
  int32_t _width;

  /**
   * 盤面の高さ。
   */
  int32_t _height;

  /**
   * データの長さ。
   */
  int32_t _length;

  /**
   * 盤面のデータ。
   */
  std::unique_ptr<int32_t[]> _values;
};

}  // namespace deepgo
