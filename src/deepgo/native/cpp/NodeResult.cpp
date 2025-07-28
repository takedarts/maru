#include "NodeResult.h"

#include "Node.h"

namespace deepgo {

/**
 * ノードの評価結果オブジェクトを作成する。
 * @param node 次に評価するノードオブジェクト
 * @param value 評価値
 * @param playouts プレイアウト数
 */
NodeResult::NodeResult(Node* node, float value, int32_t playouts)
    : _node(node),
      _value(value),
      _playouts(playouts) {
}

/**
 * 次に評価するノードオブジェクトを取得する。
 * @return 次に評価するノードオブジェクト
 */
Node* NodeResult::getNode() {
  return _node;
}

/**
 * 評価値を取得する。
 * @return 評価値
 */
float NodeResult::getValue() {
  return _value;
}

/**
 * プレイアウト数を取得する。
 * @return プレイアウト数
 */
int32_t NodeResult::getPlayouts() {
  return _playouts;
}

}  // namespace deepgo
