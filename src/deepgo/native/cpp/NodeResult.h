#pragma once

#include <cstdint>

namespace deepgo {
class Node;

/**
 * ノードの評価結果を表すクラス
 */
class NodeResult {
 public:
  /**
   * ノードの評価結果オブジェクトを作成する。
   * @param node 次に評価するノードオブジェクト
   * @param value 評価値
   * @param playouts プレイアウト数
   */
  NodeResult(Node* node, float value, int32_t playouts);

  /**
   * 次に評価するノードオブジェクトを取得する。
   * @return 次に評価するノードオブジェクト
   */
  Node* getNode();

  /**
   * 評価値を取得する。
   * @return 評価値
   */
  float getValue();

  /**
   * プレイアウト数を取得する。
   * @return プレイアウト数
   */
  int32_t getPlayouts();

 private:
  /**
   * 次に評価するノードオブジェクト。
   */
  Node* _node;

  /**
   * 加算する評価値。
   */
  float _value;

  /**
   * プレイアウト数。
   */
  int32_t _playouts;
};

}  // namespace deepgo
