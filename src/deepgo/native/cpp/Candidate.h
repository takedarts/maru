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
   * @param policy 予想着手確率
   * @param value 予想勝率
   * @param variations 予想進行
   */
  Candidate(
      int32_t x, int32_t y, int32_t color,
      int32_t visits, float policy, float value,
      std::vector<std::pair<int32_t, int32_t>> variations);

  /**
   * インスタンスを破棄する。
   */
  virtual ~Candidate() = default;

  /**
   * x座標を取得する。
   * @return x座標
   */
  int32_t getX();

  /**
   * y座標を取得する。
   * @return y座標
   */
  int32_t getY();

  /**
   * 石の色を取得する。
   * @return 石の色
   */
  int32_t getColor();

  /**
   * 訪問回数を取得する。
   * @return 訪問回数
   */
  int32_t getVisits();

  /**
   * 予想着手確率を取得する。
   * @return 予想着手確率
   */
  float getPolicy();

  /**
   * 予想勝率を取得する。
   * @return 予想勝率
   */
  float getValue();

  /**
   * 予想進行を取得する。
   * @return 予想進行
   */
  std::vector<std::pair<int32_t, int32_t>> getVariations();

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
   * 予想着手確率。
   */
  float _policy;

  /**
   * 予想勝率。
   */
  float _value;

  /**
   * 予想進行。
   */
  std::vector<std::pair<int32_t, int32_t>> _variations;
};

}  // namespace deepgo
