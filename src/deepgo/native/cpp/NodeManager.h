#pragma once

#include <mutex>
#include <vector>

#include "Node.h"
#include "NodeParameter.h"

namespace deepgo {

/**
 * Class for managing node objects.
 */
class NodeManager {
 public:
  /**
   * Create a class to manage node objects.
   * @param processor Object to execute inference
   * @param width Board width
   * @param height Board height
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply superko rule
   */
  NodeManager(
      Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko);

  /**
   * Create a node object.
   * If there is an unused node object, return it; otherwise, create a new one.
   * @return Node object
   */
  Node* createNode();

  /**
   * Set the node object to unused state.
   * @param node Node object
   */
  void releaseNode(Node* node);

  /**
   * Output the information of this node manager object.
   * @param os Output destination
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * Synchronization object.
   */
  std::mutex _mutex;

  /**
   * Parameters used when creating node objects.
   */
  NodeParameter _parameter;

  /**
   * List of node objects.
   */
  std::vector<std::unique_ptr<Node>> _nodes;

  /**
   * List of unused node objects.
   */
  std::vector<Node*> _poolNodes;

  /**
   * List of node objects in use.
   */
  std::set<Node*> _usedNodes;
};

}  // namespace deepgo
