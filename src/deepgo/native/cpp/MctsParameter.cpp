#include "MctsParameter.h"

namespace deepgo {

/**
 * パラメータオブジェクトを作成する。
 * @param width 盤面の幅
 * @param height 盤面の高さ
 * @param komi コミの目数
 * @param rule 勝敗判定ルール
 * @param superko スーパーコウルールを適用するならtrue
 * @param pucbConstantInit PUCBの信頼上限に掛ける定数の初期値
 * @param pucbConstantBase PUCBの信頼上限に掛ける定数の変化値
 */
MctsParameter::MctsParameter(
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    float pucbConstantInit, float pucbConstantBase)
    : _width(width),
      _height(height),
      _komi(komi),
      _rule(rule),
      _superko(superko),
      _pucbConstantInit(pucbConstantInit),
      _pucbConstantBase(pucbConstantBase) {
}

/**
 * 盤面の幅を返す。
 * @return 盤面の幅
 */
int32_t MctsParameter::getWidth() const {
  return _width;
}

/**
 * 盤面の高さを返す。
 * @return 盤面の高さ
 */
int32_t MctsParameter::getHeight() const {
  return _height;
}

/**
 * コミの目数を返す。
 * @return コミの目数
 */
float MctsParameter::getKomi() const {
  return _komi;
}

/**
 * 勝敗判定ルールを返す。
 * @return 勝敗判定ルール
 */
int32_t MctsParameter::getRule() const {
  return _rule;
}

/**
 * スーパーコウルールを適用するならtrueを返す。
 * @return スーパーコウルールを適用するならtrue
 */
bool MctsParameter::getSuperko() const {
  return _superko;
}

/**
 * PUCBの信頼上限に掛ける定数の初期値を返す。
 * @return PUCBの信頼上限に掛ける定数の初期値
 */
float MctsParameter::getPucbConstantInit() const {
  return _pucbConstantInit;
}

/**
 * PUCBの信頼上限に掛ける定数の変化値を返す。
 * @return PUCBの信頼上限に掛ける定数の変化値
 */
float MctsParameter::getPucbConstantBase() const {
  return _pucbConstantBase;
}

}  // namespace deepgo
