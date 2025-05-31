#pragma once

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
   * @param value 予想勝率
   */
  NodeResult(Node* node, float value);

  /**
   * 次に評価するノードオブジェクトを取得する。
   * @return 次に評価するノードオブジェクト
   */
  Node* getNode();

  /**
   * 予想勝率を取得する。
   * @return 予想勝率
   */
  float getValue();

 private:
  /**
   * 次に評価するノードオブジェクト。
   */
  Node* _node;

  /**
   * 予想勝率。
   */
  float _value;
};

}  // namespace deepgo
