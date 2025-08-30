#include "NodeManager.h"

namespace deepgo {

/**
 * Create a class to manage node objects.
 * @param processor Object to execute inference
 * @param width Board width
 * @param height Board height
 * @param komi Komi points
 * @param rule Rule for determining the winner
 * @param superko True to apply superko rule
 */
NodeManager::NodeManager(
    Processor* processor, int32_t width, int32_t height,
    float komi, int32_t rule, bool superko)
    : _mutex(),
      _parameter(processor, width, height, komi, rule, superko),
      _nodes(),
      _poolNodes(),
      _usedNodes() {
}

/**
 * Create a node object.
 * If there is an unused node object, return it; otherwise, create a new one.
 * @return Node object
 */
Node* NodeManager::createNode() {
  std::lock_guard<std::mutex> lock(_mutex);

  // Create a node object
  // Use an unused node object if available
  Node* node;

  if (_poolNodes.empty()) {
    _nodes.emplace_back(std::make_unique<Node>(
        this,
        _parameter.getProcessor(),
        _parameter.getWidth(),
        _parameter.getHeight(),
        _parameter.getKomi(),
        _parameter.getRule(),
        _parameter.getSuperko()));
    node = _nodes.back().get();
  } else {
    node = _poolNodes.back();
    _poolNodes.pop_back();
  }

  // Register as a node object in use
  _usedNodes.insert(node);

  // Return the node object
  return node;
}

/**
 * Set the node object to unused state.
 * @param node Node object
 */
void NodeManager::releaseNode(Node* node) {
  std::lock_guard<std::mutex> lock(_mutex);

  // Do nothing if not a registered node object
  if (_usedNodes.find(node) == _usedNodes.end()) {
    return;
  }

  // Reset the node object and set to unused state
  _usedNodes.erase(node);
  _poolNodes.push_back(node);
}

/**
 * Output the information of this node manager object.
 * @param os Output destination
 */
void NodeManager::print(std::ostream& os) {
  os << "NodeManager: nodes=" << _nodes.size();
  os << "(" << _usedNodes.size() << "/" << _poolNodes.size() << ")";
  os << std::endl;
}

}  // namespace deepgo
