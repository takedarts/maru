#pragma once

#include <cstdint>
#include <vector>

namespace deepgo {

/**
 * 候補手クラス。
 */
class Candidate {
 public:
  /**
   * 候補手データを作成する。
   * @param x x座標
   * @param y y座標
   * @param color 石の色
   * @param visits 訪問回数
   * @param playouts プレイアウト数
   * @param policy 予想着手確率
   * @param value 評価値
   * @param variations 予想進行
   */
  Candidate(
      int32_t x, int32_t y, int32_t color,
      int32_t visits, int32_t playouts, float policy, float value,
      std::vector<std::pair<int32_t, int32_t>> variations);

  /**
   * インスタンスを破棄する。
   */
  virtual ~Candidate() = default;

  /**
   * x座標を取得する。
   * @return x座標
   */
  int32_t getX() const;

  /**
   * y座標を取得する。
   * @return y座標
   */
  int32_t getY() const;

  /**
   * 石の色を取得する。
   * @return 石の色
   */
  int32_t getColor() const;

  /**
   * 訪問回数を取得する。
   * @return 訪問回数
   */
  int32_t getVisits() const;

  /**
   * プレイアウト数を取得する。
   * @return プレイアウト数
   */
  int32_t getPlayouts() const;

  /**
   * 予想着手確率を取得する。
   * @return 予想着手確率
   */
  float getPolicy() const;

  /**
   * 評価値を取得する。
   * @return 評価値
   */
  float getValue() const;

  /**
   * 予想進行を取得する。
   * @return 予想進行
   */
  std::vector<std::pair<int32_t, int32_t>> getVariations() const;

 private:
  /**
   * x座標。
   */
  int32_t _x;
  /**
   * y座標。
   */
  int32_t _y;

  /**
   * 石の色。
   */
  int32_t _color;

  /**
   * 訪問回数。
   */
  int32_t _visits;

  /**
   * プレイアウト数。
   */
  int32_t _playouts;

  /**
   * 予想着手確率。
   */
  float _policy;

  /**
   * 評価値。
   */
  float _value;

  /**
   * 予想進行。
   */
  std::vector<std::pair<int32_t, int32_t>> _variations;
};

}  // namespace deepgo
