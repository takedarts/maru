#pragma once

#include "Processor.h"

namespace deepgo {

/**
 * ノードオブジェクトを作成するときに使用するパラメータクラス。
 */
class NodeParameter {
 public:
  /**
   * パラメータオブジェクトを作成する。
   * @param processor 推論を実行するオブジェクト
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @param komi コミの目数
   * @param rule 勝敗判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   */
  NodeParameter(
      Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko);

  /**
   * パラメータオブジェクトを破棄する。
   */
  virtual ~NodeParameter() = default;

  /**
   * 推論を実行するオブジェクトを返す。
   * @return 推論を実行するオブジェクト
   */
  Processor* getProcessor();

  /**
   * 盤面の幅を返す。
   * @return 盤面の幅
   */
  int32_t getWidth();

  /**
   * 盤面の高さを返す。
   * @return 盤面の高さ
   */
  int32_t getHeight();

  /**
   * コミの目数を返す。
   * @return コミの目数
   */
  float getKomi();

  /**
   * 勝敗判定ルールを返す。
   * @return 勝敗判定ルール
   */
  int32_t getRule();

  /**
   * スーパーコウルールを適用するならtrueを返す。
   * @return スーパーコウルールを適用するならtrue
   */
  bool getSuperko();

 private:
  /**
   * 推論を実行するオブジェクト。
   */
  Processor* _processor;

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
};

}  // namespace deepgo
