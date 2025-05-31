#include "NodeResult.h"

#include "Node.h"

namespace deepgo {

/**
 * ノードの評価結果オブジェクトを作成する。
 * @param node 次に評価するノードオブジェクト
 * @param value 予想勝率
 */
NodeResult::NodeResult(Node* node, float value)
    : _node(node),
      _value(value) {
}

/**
 * 次に評価するノードオブジェクトを取得する。
 * @return 次に評価するノードオブジェクト
 */
Node* NodeResult::getNode() {
  return _node;
}

/**
 * 予想勝率を取得する。
 * @return 予想勝率
 */
float NodeResult::getValue() {
  return _value;
}

}  // namespace deepgo
