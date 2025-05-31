#include "Candidate.h"

namespace deepgo {

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
Candidate::Candidate(
    int32_t x, int32_t y, int32_t color,
    int32_t visits, float policy, float value,
    std::vector<std::pair<int32_t, int32_t>> variations)
    : _x(x),
      _y(y),
      _color(color),
      _visits(visits),
      _policy(policy),
      _value(value),
      _variations(variations) {
}

/**
 * x座標を取得する。
 * @return x座標
 */
int32_t Candidate::getX() {
  return _x;
}

/**
 * y座標を取得する。
 * @return y座標
 */
int32_t Candidate::getY() {
  return _y;
}

/**
 * 石の色を取得する。
 * @return 石の色
 */
int32_t Candidate::getColor() {
  return _color;
}

/**
 * 訪問回数を取得する。
 * @return 訪問回数
 */
int32_t Candidate::getVisits() {
  return _visits;
}

/**
 * 予想着手確率を取得する。
 * @return 予想着手確率
 */
float Candidate::getPolicy() {
  return _policy;
}

/**
 * 予想勝率を取得する。
 * @return 予想勝率
 */
float Candidate::getValue() {
  return _value;
}

/**
 * 予想進行を取得する。
 * @return 予想進行
 */
std::vector<std::pair<int32_t, int32_t>> Candidate::getVariations() {
  return _variations;
}

}  // namespace deepgo
