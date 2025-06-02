#include "NodeParameter.h"

namespace deepgo {

/**
 * パラメータオブジェクトを作成する。
 * @param processor 推論を実行するオブジェクト
 * @param width 盤面の幅
 * @param height 盤面の高さ
 * @param komi コミの目数
 * @param rule 勝敗判定ルール
 * @param superko スーパーコウルールを適用するならtrue
 */
NodeParameter::NodeParameter(
    Processor* processor, int32_t width, int32_t height,
    float komi, int32_t rule, bool superko)
    : _processor(processor),
      _width(width),
      _height(height),
      _komi(komi),
      _rule(rule),
      _superko(superko) {
}

/**
 * 推論を実行するオブジェクトを返す。
 * @return 推論を実行するオブジェクト
 */
Processor* NodeParameter::getProcessor() {
  return _processor;
}

/**
 * 盤面の幅を返す。
 * @return 盤面の幅
 */
int32_t NodeParameter::getWidth() {
  return _width;
}

/**
 * 盤面の高さを返す。
 * @return 盤面の高さ
 */
int32_t NodeParameter::getHeight() {
  return _height;
}

/**
 * コミの目数を返す。
 * @return コミの目数
 */
float NodeParameter::getKomi() {
  return _komi;
}

/**
 * 勝敗判定ルールを返す。
 * @return 勝敗判定ルール
 */
int32_t NodeParameter::getRule() {
  return _rule;
}

/**
 * スーパーコウルールを適用するならtrueを返す。
 * @return スーパーコウルールを適用するならtrue
 */
bool NodeParameter::getSuperko() {
  return _superko;
}

}  // namespace deepgo
