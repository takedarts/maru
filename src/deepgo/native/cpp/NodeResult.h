#pragma once

#include <cstdint>

namespace deepgo {
class Node;

/**
 * Class representing the evaluation result of a node
 */
class NodeResult {
 public:
  /**
   * Create a node evaluation result object.
   * @param node Node object to be evaluated next
   * @param value Evaluation value
   * @param playouts Number of playouts
   */
  NodeResult(Node* node, float value, int32_t playouts);

  /**
   * Get the node object to be evaluated next.
   * @return Node object to be evaluated next
   */
  Node* getNode();

  /**
   * Get the evaluation value.
   * @return Evaluation value
   */
  float getValue();

  /**
   * Get the number of playouts.
   * @return Number of playouts
   */
  int32_t getPlayouts();

 private:
  /**
   * Node object to be evaluated next.
   */
  Node* _node;

  /**
   * Evaluation value to be added.
   */
  float _value;

  /**
   * Number of playouts.
   */
  int32_t _playouts;
};

}  // namespace deepgo
