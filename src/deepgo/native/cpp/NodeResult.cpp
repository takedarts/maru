#include "NodeResult.h"

#include "Node.h"

namespace deepgo {

/**
 * Create a node evaluation result object.
 * @param node Node object to be evaluated next
 * @param value Evaluation value
 * @param playouts Number of playouts
 */
NodeResult::NodeResult(Node* node, float value, int32_t playouts)
    : _node(node),
      _value(value),
      _playouts(playouts) {
}

/**
 * Get the node object to be evaluated next.
 * @return Node object to be evaluated next
 */
Node* NodeResult::getNode() {
  return _node;
}

/**
 * Get the evaluation value.
 * @return Evaluation value
 */
float NodeResult::getValue() {
  return _value;
}

/**
 * Get the number of playouts.
 * @return Number of playouts
 */
int32_t NodeResult::getPlayouts() {
  return _playouts;
}

}  // namespace deepgo
