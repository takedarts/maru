#include "Node.h"

#include <cmath>
#include <random>

#include "NodeManager.h"

namespace deepgo {

// Random number generator
static std::random_device random_seed_gen;
static std::default_random_engine random_engine(random_seed_gen());

/**
 * Creates a search node object.
 * @param manager Node management object
 * @param processor Object to execute inference
 * @param width Board width
 * @param height Board height
 * @param komi Komi points
 * @param rule Rule for determining the winner
 * @param superko True to apply the superko rule
 */
Node::Node(
    NodeManager* manager, Processor* processor, int32_t width, int32_t height,
    float komi, int32_t rule, bool superko)
    : _evalMutex(),
      _valueMutex(),
      _manager(manager),
      _board(width, height),
      _x(-1),
      _y(-1),
      _color(WHITE),
      _captured(0),
      _policy(0.0f),
      _evaluator(processor, komi, rule, superko),
      _children(),
      _childPolicies(),
      _waitingQueue(),
      _waitingSet(),
      _visits(0),
      _playouts(0),
      _value(0.0f),
      _count(0) {
}

/**
 * Sets as the initial board node.
 */
void Node::initialize() {
  std::unique_lock<std::shared_mutex> lock(_evalMutex);

  _board.clear();
  _x = -1;
  _y = -1;
  _color = WHITE;
  _captured = 0;
  _reset();
}

/**
 * Evaluates the search node and obtains the next node object to evaluate.
 * Returns nullptr if there is no next node object to evaluate.
 * @param equally True to make the number of searches equal
 * @param width Search width (if 0, search width is automatically adjusted)
 * @param useUcb1 True to use UCB1; false to use PUCB
 * @param temperature Temperature parameter for search
 * @param noise Strength of Gumbel noise
 * @return Next node object to evaluate
 */
NodeResult Node::evaluate(
    bool equally, int32_t width, bool useUcb1, float temperature, float noise) {
  std::unique_lock<std::shared_mutex> lock(_evalMutex);

  // Execute node evaluation
  _evaluate();

  // Increase the number of visits
  _visits += 1;

  // If this is the first visit, return the evaluation result of this node
  if (_visits == 1) {
    return NodeResult(nullptr, _evaluator.getValue(), 1);
  }

  // If there are no candidate moves, return the evaluation value of this node
  if (_childPolicies.empty()) {
    return NodeResult(nullptr, _evaluator.getValue(), 1);
  }

  // Get candidate moves to add to evaluation
  int32_t children_size = _children.size() + _waitingSet.size();

  if (children_size < _childPolicies.size() && (width < 1 || children_size < width)) {
    int32_t max_index = 0;
    int32_t max_priority_type = 0;
    float max_priority = 0.0f;

    // Calculate the temperature parameter
    float win_chance = getValue() * OPPOSITE(_color) * 0.5 + 0.5;
    float temperature_power = win_chance + (1.0 / temperature) * (1 - win_chance);

    // Create an object for generating Gumbel noise
    // Do not add noise if the number of child nodes is 4 or less
    float noise_scale = (children_size <= 4) ? 0.0f : noise;
    std::extreme_value_distribution<float> noise_dist(0.0f, noise_scale);

    // Find the candidate move with the highest priority
    for (int i = 0; i < _childPolicies.size(); i++) {
      Policy& policy = _childPolicies[i];
      float probability = policy.policy;

      // Reflect the temperature parameter
      probability = std::pow(probability, temperature_power);

      // Add Gumbel noise
      // Since noise is added to logits, multiply e^noise to the probability
      probability *= std::exp(noise_dist(random_engine));

      // Calculate priority
      int32_t priority_type = 1;
      float priority = probability / (policy.visits + 1);

      // Lower the priority of registered candidate moves if the setting is to make the number of searches equal
      if (equally) {
        int32_t policy_index = policy.y * _board.getWidth() + policy.x;

        if (_children.find(policy_index) != _children.end() ||
            _waitingSet.find(policy_index) != _waitingSet.end()) {
          priority_type = 0;
        }
      }

      if (priority_type > max_priority_type ||
          (priority_type == max_priority_type && priority > max_priority)) {
        max_index = i;
        max_priority_type = priority_type;
        max_priority = priority;
      }
    }

    // If the candidate move to add to evaluation is unregistered, register it in the waiting list
    Policy& max_policy = _childPolicies[max_index];
    int32_t max_policy_index = max_policy.y * _board.getWidth() + max_policy.x;

    if (_children.find(max_policy_index) == _children.end() &&
        _waitingSet.find(max_policy_index) == _waitingSet.end()) {
      _waitingQueue.push(max_policy);
      _waitingSet.insert(max_policy_index);
    }

    // Increase the number of visits
    _childPolicies[max_index].visits += 1;
  }

  // If search width is not specified and the number of child nodes has not reached the specified search width,
  // and if there are candidate moves in the waiting list, create a new child node and return it as the next search destination
  // If this is the first child node and only leaf nodes are to be evaluated, return a result that cancels the evaluation value of this node
  if (_waitingQueue.size() > 0 && (width <= 0 || _children.size() < width)) {
    // Get the first registered candidate move in the waiting list
    Policy policy = _waitingQueue.front();
    int32_t policy_index = policy.y * _board.getWidth() + policy.x;

    _waitingQueue.pop();
    _waitingSet.erase(policy_index);

    // If the candidate move is unregistered, create a new child node and return it as the next search destination
    if (_children.find(policy_index) == _children.end()) {
      Node* node = _manager->createNode();
      bool leaf = _children.empty();

      node->_setAsNextNode(this, policy.x, policy.y, policy.policy);
      _children[policy_index] = node;

      if (leaf) {
        return NodeResult(node, _evaluator.getValue(), -1);
      } else {
        return NodeResult(node, _evaluator.getValue(), 0);
      }
    }
  }

  // Create a list of child nodes to be searched
  std::vector<std::pair<Node*, float>> children;

  for (std::pair<int32_t, Node*> child : _children) {
    children.push_back(std::make_pair(
        child.second, child.second->getValueLCB() * child.second->getColor()));
  }

  // Limit the number of child nodes to be searched if search width is specified
  if (width > 0 && children.size() > width) {
    std::sort(children.begin(), children.end(), [](auto a, auto b) {
      return a.second > b.second;
    });

    children.resize(width);
  }

  // Return the node with the highest priority as the next search destination
  Node* max_node = children[0].first;
  float max_priority = -1.0;

  for (std::pair<Node*, float> child : children) {
    float priority;

    if (equally) {
      float visits = child.first->getVisits();
      float value = child.first->getValue() * child.first->getColor();
      priority = 1.0 / (visits + 1 - value * 0.5);
    } else if (useUcb1) {
      priority = child.first->getPriorityByUCB1(_visits);
    } else {
      priority = child.first->getPriorityByPUCB(_visits);
    }

    if (max_priority < priority) {
      max_node = child.first;
      max_priority = priority;
    }
  }

  // Return the next node to search
  return NodeResult(max_node, _evaluator.getValue(), 0);
}

/**
 * Updates the evaluation value of the search node.
 * @param value Evaluation value
 */
void Node::updateValue(float value) {
  std::unique_lock<std::shared_mutex> lock(_valueMutex);
  _count += 1;
  _value += value;
}

/**
 * Cancels the evaluation value of the search node.
 * @param value Evaluation value
 */
void Node::cancelValue(float value) {
  std::unique_lock<std::shared_mutex> lock(_valueMutex);
  _count -= 1;
  _value -= value;
}

/**
 * Randomly obtains candidate moves based on the evaluation value of the PolicyNetwork.
 * @param temperature Temperature
 * @return Candidate move
 */
std::pair<int32_t, int32_t> Node::getRandomMove(float temperature) {
  // Adjust the temperature parameter
  temperature = std::max(temperature, 0.1f);

  // Execute node evaluation
  {
    std::unique_lock<std::shared_mutex> lock(_evalMutex);
    _evaluate();
  }

  // Get the list of candidate moves
  std::vector<Policy> policies;
  std::vector<float> probs;

  {
    std::shared_lock<std::shared_mutex> lock(_evalMutex);
    for (Policy policy : _evaluator.getPolicies()) {
      policies.push_back(policy);
      probs.push_back(std::pow(policy.policy, 1.0 / temperature));
    }
  }

  // If there are no candidate moves, return pass
  if (policies.empty()) {
    return std::make_pair(-1, -1);
  }

  // Randomly select based on move probability
  std::discrete_distribution<int32_t> dist(probs.begin(), probs.end());
  int32_t index = dist(random_engine);
  Policy policy = policies[index];

  return std::make_pair(policy.x, policy.y);
}

/**
 * Obtains the candidate move with the highest evaluation value from the PolicyNetwork.
 * @return Candidate move
 */
std::pair<int32_t, int32_t> Node::getPolicyMove() {
  {
    // Execute node evaluation
    std::unique_lock<std::shared_mutex> lock(_evalMutex);
    _evaluate();
  }

  // Get the list of candidate moves
  std::vector<Policy> policies;

  {
    std::shared_lock<std::shared_mutex> lock(_evalMutex);
    for (Policy policy : _evaluator.getPolicies()) {
      policies.push_back(policy);
    }
  }

  // If there are no candidate moves, return pass
  if (policies.empty()) {
    return std::make_pair(-1, -1);
  }

  // Get the candidate move with the highest move probability
  Policy max_policy = policies[0];

  for (Policy policy : policies) {
    if (max_policy.policy < policy.policy) {
      max_policy = policy;
    }
  }

  // Return the candidate move with the highest move probability
  return std::make_pair(max_policy.x, max_policy.y);
}

/**
 * Gets the x coordinate of the move.
 * @return x coordinate
 */
int32_t Node::getX() const {
  return _x;
}

/**
 * Gets the y coordinate of the move.
 * @return y coordinate
 */
int32_t Node::getY() const {
  return _y;
}

/**
 * Gets the color of the stone played.
 * @return Stone color
 */
int32_t Node::getColor() const {
  return _color;
}

/**
 * Gets the number of captured stones in this node.
 * @return Number of captured stones
 */
int32_t Node::getCaptured() const {
  return _captured;
}

/**
 * Gets the predicted move probability of this node.
 * @return Predicted move probability
 */
float Node::getPolicy() const {
  return _policy;
}

/**
 * Gets the list of child nodes.
 * @return List of node objects
 */
std::vector<Node*> Node::getChildren() {
  std::shared_lock<std::shared_mutex> lock(_evalMutex);
  std::vector<Node*> children;

  for (std::pair<int32_t, Node*> item : _children) {
    children.push_back(item.second);
  }

  return children;
}

/**
 * Gets the node object when a move is made at the specified coordinates.
 * If the node object does not exist, returns a newly created object.
 * The created node object is not registered as a child node of this node object.
 * @param x X coordinate of the move
 * @param y Y coordinate of the move
 * @return Pointer to the node object
 */
Node* Node::getChild(int32_t x, int32_t y) {
  std::unique_lock<std::shared_mutex> lock(_evalMutex);

  // If a child node exists, return that node
  int32_t index = y * _board.getWidth() + x;

  if (_children.find(index) != _children.end()) {
    return _children[index];
  }

  // If a child node does not exist, create a new node
  Node* node = _manager->createNode();

  node->_setAsNextNode(this, x, y, 1.0);

  return node;
}

/**
 * Gets the number of searches for this node.
 * @return Number of searches
 */
int32_t Node::getVisits() {
  std::shared_lock<std::shared_mutex> lock(_evalMutex);
  return _visits;
}

/**
 * Gets the number of playouts.
 * @return Number of playouts
 */
int32_t Node::getPlayouts() {
  std::unique_lock<std::shared_mutex> lock(_valueMutex);
  return _playouts;
}

/**
 * Sets the number of playouts.
 * @param playouts Number of playouts
 */
void Node::setPlayouts(int32_t playouts) {
  std::unique_lock<std::shared_mutex> lock(_valueMutex);
  _playouts = playouts;
}

/**
 * Gets the evaluation value of this node.
 * @return Evaluation value
 */
float Node::getValue() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return 0.0f;
  } else {
    return _value / _count;
  }
}

/**
 * Gets the number of times the evaluation value of this node has been added.
 * @return Number of times the evaluation value has been added
 */
int Node::getCount() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  return _count;
}

/**
 * Gets the lower bound of the confidence interval for the evaluation value of this node.
 * @return Lower bound of the confidence interval
 */
float Node::getValueLCB() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return 0.0f;
  } else {
    float value = _value / _count;
    float lower = 1.96 * 0.5 / std::sqrt(_visits + 1);
    return value - (lower * _color);
  }
}

/**
 * Gets the priority of this node based on PUCB.
 * @param totalVisits Total number of searches
 * @return Priority
 */
float Node::getPriorityByPUCB(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return -99.0f;
  } else {
    float c_puct = std::log((1 + totalVisits + 19652.0) / 19652.0) + 1.25;
    float value = (_value / _count) * _color;
    float upper = c_puct * _policy * std::sqrt(totalVisits) / (1 + _visits);
    return value + 2 * upper;
  }
}

/**
 * Gets the priority of this node based on UCB1.
 * @param totalVisits Total number of searches
 * @return Priority
 */
float Node::getPriorityByUCB1(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return -99.0f;
  } else {
    float value = (_value / _count) * _color;
    float upper = 0.5 * std::sqrt(std::log(totalVisits) / (_visits + 1));
    return value + upper;
  }
}

/**
 * Gets the predicted sequence for this node.
 * @return Predicted sequence
 */
std::vector<std::pair<int32_t, int32_t>> Node::getVariations() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  std::vector<std::pair<int32_t, int32_t>> variations;
  int32_t max_visits = 0;
  Node* max_child = nullptr;

  for (auto child : _children) {
    if (child.second->_visits > max_visits) {
      max_visits = child.second->_visits;
      max_child = child.second;
    }
  }

  variations.push_back(std::make_pair(_x, _y));

  if (max_child != nullptr) {
    std::vector<std::pair<int32_t, int32_t>> child_variations = max_child->getVariations();
    variations.insert(variations.end(), child_variations.begin(), child_variations.end());
  }

  return variations;
}

/**
 * Gets the state of the board.
 * @return State of the board
 */
std::vector<int32_t> Node::getBoardState() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  return _board.getState();
}

/**
 * Outputs the information of this node.
 * @param os Output destination
 */
void Node::print(std::ostream& os) {
  _board.print(os);
  os << "color:" << _color << std::endl;
  os << "visits:" << _visits << std::endl;
  os << "value:" << getValue() << std::endl;
}

/**
 * Executes the evaluation of this node.
 */
void Node::_evaluate() {
  if (_evaluator.isEvaluated()) {
    return;
  }

  _evaluator.evaluate(&_board, OPPOSITE(_color));

  for (Policy policy : _evaluator.getPolicies()) {
    _childPolicies.push_back(policy);
  }
}

/**
 * Initializes the evaluation information of the node.
 */
void Node::_reset() {
  _evaluator.clear();
  _children.clear();
  _childPolicies.clear();
  _waitingQueue = std::queue<Policy>();
  _waitingSet.clear();
  _visits = 0;
  _playouts = 0;
  _value = 0.0f;
  _count = 0;
}

/**
 * Sets the value as a continuation node of the specified node.
 * @param prevNode Previous node
 * @param x X coordinate of the move
 * @param y Y coordinate of the move
 * @param policy Predicted move probability
 */
void Node::_setAsNextNode(Node* prevNode, int32_t x, int32_t y, float policy) {
  _x = x;
  _y = y;
  _color = OPPOSITE(prevNode->_color);
  _policy = policy;

  _board.copyFrom(&prevNode->_board);
  _captured = _board.play(_x, _y, _color);

  _reset();
}

}  // namespace deepgo
