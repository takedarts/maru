#pragma once

#include <cstdint>

namespace deepgo {

/**
 * 探索ノードを作成するときに使用するパラメータクラス。
 */
class MctsParameter {
 public:
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
  MctsParameter(
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      float pucbConstantInit, float pucbConstantBase);

  /**
   * 盤面の幅を返す。
   * @return 盤面の幅
   */
  int32_t getWidth() const;

  /**
   * 盤面の高さを返す。
   * @return 盤面の高さ
   */
  int32_t getHeight() const;

  /**
   * コミの目数を返す。
   * @return コミの目数
   */
  float getKomi() const;

  /**
   * 勝敗判定ルールを返す。
   * @return 勝敗判定ルール
   */
  int32_t getRule() const;

  /**
   * スーパーコウルールを適用するならtrueを返す。
   * @return スーパーコウルールを適用するならtrue
   */
  bool getSuperko() const;

  /**
   * PUCBの信頼上限に掛ける定数の初期値を返す。
   * @return PUCBの信頼上限に掛ける定数の初期値
   */
  float getPucbConstantInit() const;

  /**
   * PUCBの信頼上限に掛ける定数の変化値を返す。
   * @return PUCBの信頼上限に掛ける定数の変化値
   */
  float getPucbConstantBase() const;

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
   * コミの目数。
   */
  float _komi;

  /**
   * 勝敗判定ルール。
   */
  int32_t _rule;

  /**
   * スーパーコウルールを適用するならtrue。
   */
  bool _superko;

  /**
   * PUCBの信頼上限に掛ける定数の初期値。
   */
  float _pucbConstantInit;

  /**
   * PUCBの信頼上限に掛ける定数の変化値。
   */
  float _pucbConstantBase;
};

}  // namespace deepgo
