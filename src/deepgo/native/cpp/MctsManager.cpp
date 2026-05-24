#include "MctsManager.h"

#include <sstream>

#include "MctsNode.h"

namespace deepgo {

/**
 * Creates a node management object.
 * @param parameter Node creation parameter
 */
MctsManager::MctsManager(const MctsParameter& parameter)
    : _mutex(),
      _parameter(parameter),
      _nodes(),
      _poolNodes(),
      _usedNodes() {
}

/**
 * Creates a node object.
 * @return Node object
 */
MctsNode* MctsManager::createNode() {
  std::lock_guard<std::mutex> lock(_mutex);

  // Create a node object
  // Reuse an unused node object if available
  MctsNode* node;

  if (_poolNodes.empty()) {
    _nodes.emplace_back(std::make_unique<MctsNode>(this));
    node = _nodes.back().get();
  } else {
    node = _poolNodes.back();
    _poolNodes.pop_back();
  }

  // Register as an in-use node object
  _usedNodes.insert(node);

  // Return the node object
  return node;
}

/**
 * Releases a node object back to the unused pool.
 * @param node Node object
 */
void MctsManager::releaseNode(MctsNode* node) {
  std::lock_guard<std::mutex> lock(_mutex);

  // Do nothing if already in the unused pool
  if (_usedNodes.find(node) == _usedNodes.end()) {
    return;
  }

  // Release the node object to the unused pool
  _usedNodes.erase(node);
  _poolNodes.push_back(node);
}

/**
 * Releases a node and all its descendants.
 * @param root Root of the search tree to release
 */
void MctsManager::releaseTree(MctsNode* root) {
  std::vector<MctsNode*> stack = {root};

  // Traverse depth-first and release each node
  while (!stack.empty()) {
    MctsNode* current = stack.back();
    stack.pop_back();

    for (MctsNode* child : current->getChildren()) {
      stack.push_back(child);
    }

    releaseNode(current);
  }
}

/**
 * Returns the state of the MCTS search node manager as a string.
 * @return String representing the state
 */
std::string MctsManager::toString() {
  std::lock_guard<std::mutex> lock(_mutex);
  std::ostringstream ss;

  ss << "NodeManager: nodes=" << _nodes.size();
  ss << "(" << _usedNodes.size() << "/" << _poolNodes.size() << ")";
  ss << std::endl;

  return ss.str();
}

}  // namespace deepgo
