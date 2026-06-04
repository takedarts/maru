#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "MctsParameter.h"

namespace deepgo {

class MctsNode;

/**
 * Class for managing node objects.
 */
class MctsManager {
 public:
  /**
   * Creates a node management object.
   * @param parameter Node creation parameter
   */
  explicit MctsManager(const MctsParameter& parameter);

  /**
   * Creates a node object.
   * @return Node object
   */
  MctsNode* createNode();

  /**
   * Releases a node object back to the unused pool.
   * @param node Node object
   */
  void releaseNode(MctsNode* node);

  /**
   * Releases a node and all its descendants.
   * @param root Root of the search tree to release
   */
  void releaseTree(MctsNode* root);

  /**
   * Returns the state of the MCTS search node manager as a string.
   * @return String representing the state
   */
  std::string toString();

  /**
   * Returns the node creation parameter.
   * @return Node creation parameter
   */
  inline const MctsParameter& getParameter() const {
    return _parameter;
  }

 private:
  /**
   * Synchronization object.
   */
  std::mutex _mutex;

  /**
   * Node creation parameter.
   */
  MctsParameter _parameter;

  /**
   * List of node objects.
   */
  std::vector<std::unique_ptr<MctsNode>> _nodes;

  /**
   * List of unused node objects.
   */
  std::vector<MctsNode*> _poolNodes;

  /**
   * List of in-use node objects.
   */
  std::set<MctsNode*> _usedNodes;
};

}  // namespace deepgo
