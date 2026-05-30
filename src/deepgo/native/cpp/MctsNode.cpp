#include "MctsNode.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "MctsManager.h"

namespace deepgo {

// Thread-local random number generators used for
// policy sampling and Gumbel noise during search
thread_local static std::random_device random_seed_gen;
thread_local static std::default_random_engine random_engine(random_seed_gen());

/**
 * Creates a search node object.
 * @param manager Node management object
 */
MctsNode::MctsNode(MctsManager* manager)
    : _mutex(),
      _condition(),
      _manager(manager),
      _board(
          manager->getParameter().getWidth(),
          manager->getParameter().getHeight()),
      _move(Move::createPassMove(WHITE)),
      _captured(0),
      _probability(0.0f),
      _firstChild(false),
      _evaluating(false),
      _evaluated(false),
      _nodeValue(0.0f),
      _policies(),
      _parent(nullptr),
      _children(),
      _visits(0),
      _playouts(0),
      _mctsValue(),
      _waitingPolicies(),
      _waitingMoves() {
}

/**
 * Initializes this as an initial board node.
 */
void MctsNode::initialize() {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _resetNode();
  _board.clear();
  _move = Move::createPassMove(WHITE);
  _captured = 0;
}

/**
 * Initializes this as an initial board node.
 * @param board Board state
 * @param x X coordinate of the move
 * @param y Y coordinate of the move
 * @param previousColor Color of the last played stone
 * @param captured Number of captured stones
 */
void MctsNode::initialize(
    const Board* board, int x, int y, int32_t previousColor, int32_t captured) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _resetNode();
  _board.copyFrom(board);
  _move = Move(x, y, previousColor);
  _captured = captured;
}

/**
 * Applies an inference result.
 * @param result Inference result
 */
void MctsNode::applyInferenceResult(const InferenceResult& result) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // Update the board evaluation value
  _nodeValue = result.getValue();

  // Update the list of predicted move probabilities
  _policies = result.getPolicies();

  // Update the predicted territory probabilities
  _territories = result.getTerritories();

  // Mark as evaluated
  _evaluating = false;
  _evaluated = true;

  // Notify threads waiting for evaluation to complete
  _condition.notify_all();
}

/**
 * Gets the next node to evaluate.
 * Returns this node if there is no next node to evaluate.
 * Returns nullptr if the search is canceled.
 * @param equally True if search count should be equally distributed
 * @param width Search width
 * @param temperature Temperature parameter for search
 * @param noise Strength of Gumbel noise
 * @param isCanceled Function that returns true if the search is canceled
 * @return Next node to evaluate
 */
MctsNode* MctsNode::pickupNextNode(
    bool equally, int32_t width, float temperature, float noise,
    std::function<bool()> isCanceled) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // If this node is being evaluated, wait for evaluation to complete
  if (_evaluating) {
    _condition.wait(lock, [this] { return !_evaluating; });
  }

  // If this node has already been evaluated, find the next node to evaluate
  if (_evaluated) {
    // If the search is canceled, return nullptr
    if (isCanceled()) {
      return nullptr;
    }

    // If there is a next node to evaluate, return it
    if (!_policies.empty()) {
      return _pickupNextNode(equally, width, temperature, noise);
    }

    // This node is a leaf node, so increment visit count and playout count
    MctsNode* current_node = this;

    while (current_node != nullptr) {
      current_node->_playouts.fetch_add(1, std::memory_order_relaxed);
      current_node->_visits.fetch_add(1, std::memory_order_relaxed);
      current_node = current_node->_parent;
    }

    return this;
  }

  // Mark this node as being evaluated
  _evaluating = true;

  // This node is a leaf node, so increment visit count and playout count
  _visits.fetch_add(1, std::memory_order_relaxed);
  _playouts.fetch_add(1, std::memory_order_relaxed);

  // Increment visit count and playout count of parent nodes
  MctsNode* current_node = _parent;

  while (current_node != nullptr) {
    current_node->_visits.fetch_add(1, std::memory_order_relaxed);

    if (!_firstChild) {
      current_node->_playouts.fetch_add(1, std::memory_order_relaxed);
    }

    current_node = current_node->_parent;
  }

  return this;
}

/**
 * Sets this node as the root node.
 */
void MctsNode::setAsRootNode() {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _parent = nullptr;
  _probability = 1.0f;
}

/**
 * Returns true if this node has been evaluated.
 * @return True if evaluated
 */
bool MctsNode::isEvaluated() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _evaluated;
}

/**
 * Returns the board evaluation value.
 * @return Board evaluation value
 */
float MctsNode::getNodeValue() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _nodeValue;
}

/**
 * Sets the color of the previously played stone.
 * @param color Color of the previously played stone
 */
void MctsNode::setPreviousColor(int32_t color) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _move = Move(_move.getX(), _move.getY(), color);
}

/**
 * Returns the komi.
 * @return Komi
 */
float MctsNode::getKomi() const {
  return _manager->getParameter().getKomi();
}

/**
 * Returns the rule.
 * @return Rule
 */
int32_t MctsNode::getRule() const {
  return _manager->getParameter().getRule();
}

/**
 * Returns true if the superko rule is applied.
 * @return True if the superko rule is applied
 */
bool MctsNode::getSuperko() const {
  return _manager->getParameter().getSuperko();
}

/**
 * Returns the candidate move with the highest PolicyNetwork evaluation value.
 * @return Candidate move
 */
Move MctsNode::getPolicyMove() {
  std::shared_lock<std::shared_mutex> lock(_mutex);

  // If there are no candidate moves, return a pass
  if (_policies.empty()) {
    return Move::createPassMove(getNextColor());
  }

  // Get the candidate move with the highest move probability
  Policy max_policy = _policies[0];

  for (const Policy& policy : _policies) {
    if (max_policy.getProbability() < policy.getProbability()) {
      max_policy = policy;
    }
  }

  // Return the candidate move with the highest move probability
  return max_policy.getMove();
}

/**
 * Returns the list of child nodes.
 * @return List of child nodes
 */
std::vector<MctsNode*> MctsNode::getChildren() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<MctsNode*> children;

  for (const auto& item : _children) {
    children.push_back(item.second);
  }

  return children;
}

/**
 * Returns the parent node.
 * @return Parent node
 */
MctsNode* MctsNode::getParent() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _parent;
}

/**
 * Returns the node corresponding to the specified move.
 * @param move Move
 * @return Node
 */
MctsNode* MctsNode::getChild(Move move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  int32_t index = _getMoveIndex(move);

  // If the child node exists, return it
  if (_children.find(index) != _children.end()) {
    return _children[index];
  }

  // If no child node exists, create a new node object and return it
  // The created node is not registered as a child node of this node
  MctsNode* node = _manager->createNode();

  node->_resetNode();
  node->_board.copyFrom(&_board);
  node->_captured = std::max(node->_board.play(move), 0);
  node->_board.updateStatus();
  node->_move = move;
  node->_probability = 0.0f;

  return node;
}

/**
 * Removes the child node corresponding to the specified move.
 * @param move Move
 */
void MctsNode::removeChild(Move move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  int32_t index = _getMoveIndex(move);
  _children.erase(index);
}

/**
 * Returns the visit count of this node.
 * @return Visit count
 */
int32_t MctsNode::getVisits() {
  return _visits.load(std::memory_order_relaxed);
}

/**
 * Returns the playout count.
 * @return Playout count
 */
int32_t MctsNode::getPlayouts() {
  return _playouts.load(std::memory_order_relaxed);
}

/**
 * Updates the MCTS evaluation value.
 * @param value Evaluation value
 */
void MctsNode::updateMctsValue(float value) {
  _mctsValue.update(value);
}

/**
 * Returns the MCTS evaluation value.
 * @return MCTS evaluation value
 */
float MctsNode::getMctsValue() {
  return _mctsValue.getValue(_nodeValue);
}

/**
 * Returns the lower confidence bound of the MCTS evaluation value.
 * @return Lower confidence bound
 */
float MctsNode::getMctsValueLCB() {
  return _mctsValue.getValueLCB(_move.getColor(), _nodeValue);
}

/**
 * Returns the priority based on PUCB.
 * @param totalVisits Total visit count
 * @return Priority
 */
float MctsNode::getPriorityByPUCB(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_mutex);

  int32_t visits = _visits.load(std::memory_order_relaxed);
  float pucb_constant_base = _manager->getParameter().getPucbConstantBase();
  float pucb_constant_init = _manager->getParameter().getPucbConstantInit();

  // Add the Policy-based search bonus to the evaluation value from the current player's perspective
  float value = _mctsValue.getValue(_nodeValue) * _move.getColor();
  float c_pucb_inc = std::log((1 + totalVisits + pucb_constant_base) / pucb_constant_base);
  float c_pucb = pucb_constant_init * (1.0f + c_pucb_inc);
  float ucb = _probability * std::sqrt(static_cast<float>(totalVisits)) / (1 + visits);

  return value + c_pucb * ucb;
}

/**
 * Returns the predicted variation from this node.
 * @return Predicted variation
 */
std::vector<Move> MctsNode::getVariations() {
  std::vector<Move> variations;
  MctsNode* max_child = nullptr;

  {
    // Lock the synchronization mutex
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // Add this node's move to the front of the predicted variation
    variations.push_back(_move);

    // Build the predicted variation by following the child node with the highest evaluation LCB
    float max_lcb = -std::numeric_limits<float>::infinity();

    for (auto child : _children) {
      float child_lcb = child.second->getMctsValueLCB() * getNextColor();

      if (child_lcb > max_lcb) {
        max_lcb = child_lcb;
        max_child = child.second;
      }
    }
  }

  // Append the child node's predicted variation to the end
  if (max_child != nullptr) {
    std::vector<Move> child_variations = max_child->getVariations();
    variations.insert(variations.end(), child_variations.begin(), child_variations.end());
  }

  return variations;
}

/**
 * Returns the predicted territory probabilities.
 * @return Predicted territory probabilities
 */
std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> MctsNode::getTerritories() {
  MctsNode* max_child = nullptr;

  {
    // Lock the synchronization mutex
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // If there are no child nodes, return this node's predicted territory probabilities
    if (_children.empty()) {
      const int32_t width = _board.getWidth();
      const int32_t height = _board.getHeight();
      std::vector<int32_t> board_territory(width * height);

      _board.getTerritories(board_territory.data(), BLACK);

      const int32_t x_begin = (MODEL_SIZE - width) / 2;
      const int32_t y_begin = (MODEL_SIZE - height) / 2;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;
          int32_t model_index = (y_begin + y) * MODEL_SIZE + (x_begin + x);

          if (board_territory[board_index] == BLACK) {
            _territories[model_index + 0 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 1 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 2 * MODEL_SIZE * MODEL_SIZE] = 1.0f;
          } else if (board_territory[board_index] == WHITE) {
            _territories[model_index + 0 * MODEL_SIZE * MODEL_SIZE] = 1.0f;
            _territories[model_index + 1 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 2 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
          }
        }
      }

      return _territories;
    }

    // Get the predicted territory probabilities by following the child node with the highest evaluation LCB
    float max_lcb = -std::numeric_limits<float>::infinity();

    for (auto child : _children) {
      float child_lcb = child.second->getMctsValueLCB() * getNextColor();

      if (child_lcb > max_lcb) {
        max_lcb = child_lcb;
        max_child = child.second;
      }
    }
  }

  // Return the child node's predicted territory probabilities
  return max_child->getTerritories();
}

/**
 * Returns the board state.
 * @return Board state
 */
std::vector<int32_t> MctsNode::getBoardState() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _board.getState();
}

/**
 * Initializes all state except the board.
 */
void MctsNode::_resetNode() {
  _probability = 0.0f;
  _firstChild = false;
  _evaluating = false;
  _evaluated = false;
  _nodeValue = 0.0f;
  _policies.clear();
  _parent = nullptr;
  _children.clear();
  _visits.store(0, std::memory_order_relaxed);
  _playouts.store(0, std::memory_order_relaxed);
  _mctsValue.reset();
  _waitingPolicies = std::queue<Policy>();
  _waitingMoves.clear();
}

/**
 * Returns the next node to evaluate.
 * @param equally True to equalize the visit count
 * @param width Search width
 * @param temperature Temperature parameter for search
 * @param noise Gumbel noise strength
 * @return Next node to evaluate
 */
MctsNode* MctsNode::_pickupNextNode(bool equally, int32_t width, float temperature, float noise) {
  // If this is the root node, the Japanese rule is in effect, there is at least one child node,
  // no pass child node exists, and no pass candidate is in the waiting list,
  // add a pass candidate with probability 0 to the waiting list
  Move pass_move = Move::createPassMove(getNextColor());
  int32_t pass_move_index = _getMoveIndex(pass_move);

  if (_parent == nullptr && _manager->getParameter().getRule() == RULE_JP &&
      !_children.empty() && _children.find(pass_move_index) == _children.end() &&
      _waitingMoves.find(pass_move_index) == _waitingMoves.end()) {
    Policy pass_policy(pass_move, 0.0f, 0);

    _waitingPolicies.push(pass_policy);
    _waitingMoves.insert(pass_move_index);
  }

  // If there are remaining Policy candidates and search width allows, add a new move as an expansion candidate
  int32_t children_size = static_cast<int32_t>(_children.size() + _waitingMoves.size());

  if (children_size < static_cast<int32_t>(_policies.size()) &&
      (width < 1 || children_size < width)) {
    int32_t max_index = 0;
    int32_t max_priority_type = 0;
    float max_priority = 0.0f;

    // Calculate the temperature parameter
    float win_chance = _mctsValue.getValue(_nodeValue) * getNextColor() * 0.5f + 0.5f;
    float temperature_power =
        win_chance + (1.0f / std::max(temperature, 1e-3f)) * (1 - win_chance);

    // Create a Gumbel noise distribution object
    // Do not add noise if the number of child nodes is 4 or fewer
    float noise_scale = (children_size <= 4) ? 0.0f : noise;
    std::extreme_value_distribution<float> noise_dist(0.0f, noise_scale);

    // Select the next candidate based on predicted probability, temperature, Gumbel noise, and unexpanded-first priority
    for (int32_t i = 0; i < static_cast<int32_t>(_policies.size()); i++) {
      Policy& policy = _policies[i];
      float probability = policy.getProbability();

      // Apply the temperature parameter
      probability = std::pow(probability, temperature_power);

      // Add Gumbel noise
      // Since noise is added to logits, multiply the probability by e^noise
      probability *= std::exp(noise_dist(random_engine));

      // Calculate the priority
      int32_t priority_type = 1;
      float priority = probability / (policy.getVisits() + 1);

      // If configured to equalize visit counts, lower the priority of already-registered candidates
      if (equally) {
        int32_t policy_index = _getMoveIndex(policy.getMove());

        if (_children.find(policy_index) != _children.end() ||
            _waitingMoves.find(policy_index) != _waitingMoves.end()) {
          priority_type = 0;
        }
      }

      // Keep the candidate with the highest priority
      if (priority_type > max_priority_type ||
          (priority_type == max_priority_type && priority > max_priority)) {
        max_index = i;
        max_priority_type = priority_type;
        max_priority = priority;
      }
    }

    // If the selected candidate is not yet registered, add it to the waiting list
    Policy& max_policy = _policies[max_index];
    int32_t max_policy_index = _getMoveIndex(max_policy.getMove());

    if (_children.find(max_policy_index) == _children.end() &&
        _waitingMoves.find(max_policy_index) == _waitingMoves.end()) {
      _waitingPolicies.push(max_policy);
      _waitingMoves.insert(max_policy_index);
    }

    _policies[max_index].incrementVisits();
  }

  // If no search width is specified or the number of child nodes has not reached the specified width,
  // and if there is a candidate in the waiting list, create a new child node and return it as the next search target
  if (_waitingPolicies.size() > 0 && (width <= 0 || _children.size() < width)) {
    // Get the first registered candidate from the waiting list
    Policy policy = _waitingPolicies.front();
    int32_t policy_index = _getMoveIndex(policy.getMove());

    _waitingPolicies.pop();
    _waitingMoves.erase(policy_index);

    // If this is an unregistered candidate, create a new child node and return it as the next search target
    // Tentatively set the lowest evaluation value for the node
    if (_children.find(policy_index) == _children.end()) {
      MctsNode* node = _manager->createNode();

      node->_resetNode();
      node->_board.copyFrom(&_board);
      node->_captured = std::max(node->_board.play(policy.getMove()), 0);
      node->_board.updateStatus();
      node->_move = policy.getMove();
      node->_probability = policy.getProbability();
      node->_nodeValue = _move.getColor();
      node->_parent = this;
      node->_firstChild = (_children.size() == 0);
      _children[policy_index] = node;

      return node;
    }
  }

  // Build the list of child nodes to search
  std::vector<std::pair<MctsNode*, float>> children;

  for (std::pair<int32_t, MctsNode*> child : _children) {
    children.push_back(std::make_pair(
        child.second, child.second->getMctsValueLCB() * getNextColor()));
  }

  if (children.empty()) {
    return this;
  }

  // If a search width is specified, limit the number of child nodes to search
  if (width > 0 && children.size() > static_cast<size_t>(width)) {
    std::sort(children.begin(), children.end(), [](auto a, auto b) {
      return a.second > b.second;
    });

    children.resize(width);
  }

  // Return the node with the highest priority as the next search target
  int32_t total_visits = _visits.load(std::memory_order_relaxed);
  float max_priority = -std::numeric_limits<float>::infinity();
  MctsNode* max_node = nullptr;

  for (std::pair<MctsNode*, float> child : children) {
    float priority;

    // Do not search pass nodes
    if (child.first->_move.isPass()) {
      continue;
    }
    // If configured to equalize visit counts,
    // calculate priority based on visit count (consider evaluation value if visit counts are equal)
    else if (equally) {
      float visits = static_cast<float>(child.first->getVisits());
      float value = child.first->getMctsValue() * getNextColor();
      priority = 1.0f / (visits + 1 - value * 0.5f);
    }
    // Otherwise, calculate priority based on PUCB
    else {
      priority = child.first->getPriorityByPUCB(total_visits);
    }

    // Keep the node with the highest priority
    if (max_priority < priority) {
      max_node = child.first;
      max_priority = priority;
    }
  }

  // Return the node with the highest priority as the next search target
  return max_node;
}

}  // namespace deepgo
