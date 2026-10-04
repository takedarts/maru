#include "MctsNode.h"

#include <algorithm>
#include <cmath>
#include <numeric>
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
      _move(Move::createPassMove(COLOR_WHITE)),
      _komi(manager->getParameter().getKomi()),
      _captured(0),
      _passed(0),
      _probability(0.0f),
      _evaluating(false),
      _evaluated(false),
      _nodeValue(0.0f),
      _nodeScore(0.0f),
      _policies(),
      _parent(nullptr),
      _children(),
      _appearedBoardHashes(),
      _visits(0),
      _pvVisits(0),
      _mctsValue(),
      _mctsSelects(0),
      _mctsProceeds(0),
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
  _move = Move::createPassMove(COLOR_WHITE);
  _komi = _manager->getParameter().getKomi();
  _captured = 0;
  _passed = 0;
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
  _komi = _manager->getParameter().getKomi();
  _captured = captured;
  _passed = 0;
}

/**
 * Applies an inference result.
 * @param result Inference result
 */
void MctsNode::applyInferenceResult(const InferenceResult& result) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // Update the board evaluation value
  _nodeValue = result.getValue();

  // Update predicted territory probabilities
  _territories = result.getTerritories();

  // For pass positions, set the predicted score from predicted territories
  // Otherwise, use the model output for the predicted score difference
  _nodeScore = (_passed > 0) ? _calculateScoreFromTerritories() : result.getScore();

  // Update the list of predicted move probabilities
  _policies.clear();

  // Create candidates from inference results for ordinary positions and root positions
  // Treat non-root positions with at least two consecutive passes as terminal nodes
  if (_passed < 2 || _parent == nullptr) {
    // Create a working board for applying candidate moves
    Board candidate_board(_board);

    // Build a candidate list excluding superko moves
    const auto& result_policies = result.getPolicies();
    std::vector<std::pair<Move, float>> filtered_policies;

    for (const auto& policy : result_policies) {
      // Exclude passes from superko checks
      if (policy.first.isPass()) {
        filtered_policies.push_back(policy);
        continue;
      }

      // Apply the candidate to the working board
      candidate_board.copyFrom(&_board);
      candidate_board.play(policy.first);

      // Do not register candidates that violate superko
      if (_isSuperkoBoard(&candidate_board)) {
        continue;
      }

      filtered_policies.push_back(policy);
    }

    // After filtering superko moves,
    // check whether any non-pass candidates remain
    bool has_move = std::any_of(
        filtered_policies.begin(), filtered_policies.end(),
        [](const auto& policy) { return !policy.first.isPass(); });

    // For a root position with at least two consecutive passes,
    // resume searching only if on-board candidates remain
    bool can_expand = _passed < 2 || has_move;

    // Outside Japanese rules, exclude pass when on-board candidates remain
    bool exclude_pass = getRule() != RULE_JP && has_move;
    float total_probability = 0.0f;
    int32_t policy_count = 0;

    for (const auto& policy : filtered_policies) {
      if (!can_expand || (exclude_pass && policy.first.isPass())) {
        continue;
      }

      total_probability += policy.second;
      policy_count++;
    }

    // Normalize the remaining candidate probabilities
    bool can_normalize =
        std::isfinite(total_probability) && total_probability > 0.0f;
    float default_probability =
        policy_count > 0 ? 1.0f / static_cast<float>(policy_count) : 0.0f;

    for (const auto& policy : filtered_policies) {
      if (!can_expand || (exclude_pass && policy.first.isPass())) {
        continue;
      }

      float probability = can_normalize
                              ? policy.second / total_probability
                              : default_probability;

      _policies.emplace_back(policy.first, probability);
    }
  }

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

  // Increment the visit count `_mctsProceeds`
  // For root nodes, increment `_mctsSelects` for itself
  _mctsProceeds.fetch_add(1, std::memory_order_relaxed);

  if (_parent == nullptr) {
    _mctsSelects.fetch_add(1, std::memory_order_relaxed);
  }

  // If this node has already been evaluated, find the next node to evaluate
  if (_evaluated) {
    // If the search is canceled
    if (isCanceled()) {
      // The variable `_mctsSelects` and `_mctsProceeds` have already been incremented
      // Cancel the changes in these variables for this node and parent nodes
      MctsNode* current_node = this;

      while (current_node != nullptr) {
        current_node->_mctsSelects.fetch_sub(1, std::memory_order_relaxed);
        current_node->_mctsProceeds.fetch_sub(1, std::memory_order_relaxed);
        current_node = current_node->_parent;
      }

      // Return nullptr when search is canceled
      return nullptr;
    }

    // If there is a next node to evaluate, return it
    if (!_policies.empty()) {
      return _pickupNextNode(equally, width, temperature, noise);
    }

    // Increment the search count because this is a leaf node
    MctsNode* current_node = this;

    while (current_node != nullptr) {
      current_node->_incrementVisits();
      current_node = current_node->_parent;
    }

    return this;
  }

  // Increment the search counts of this node and its ancestors
  MctsNode* current_node = this;

  while (current_node != nullptr) {
    current_node->_incrementVisits();
    current_node = current_node->_parent;
  }

  // Mark this node as being evaluated
  _evaluating = true;

  return this;
}

/**
 * Sets this node as the root node.
 * Detach the parent and reset evaluation and statistics when needed.
 */
void MctsNode::setAsRootNode() {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // Detach the parent and reset the move probability
  _parent = nullptr;
  _probability = 1.0f;

  // Reevaluate an evaluated node with no candidates as a root position
  if (_evaluated && _policies.empty()) {
    // Return child nodes to the node pool
    for (const auto& child : _children) {
      _manager->releaseTree(child.second);
    }

    // Reset the node while preserving the board and information needed to change roots
    Move move = _move;
    float komi = _komi;
    int32_t captured = _captured;
    int32_t passed = _passed;
    std::set<BoardHash> appeared_board_hashes = _appearedBoardHashes;

    _resetNode();
    _move = move;
    _komi = komi;
    _captured = captured;
    _passed = passed;
    _probability = 1.0f;
    _appearedBoardHashes = appeared_board_hashes;
  }
}

/**
 * Inherit the historical positions when the root changes.
 * @param oldRootNode Previous root node
 */
void MctsNode::copyAppearedBoardHashes(const MctsNode* oldRootNode) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // Copy positions that occurred before the previous root
  _appearedBoardHashes = oldRootNode->_appearedBoardHashes;

  // Add the previous root position to the history
  _appearedBoardHashes.insert(BoardHash(&oldRootNode->_board));
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
 * Get the board's predicted score difference.
 * @return Predicted score difference of the board
 */
float MctsNode::getNodeScore() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _nodeScore;
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
  return _komi;
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
  MctsPolicy max_policy = _policies[0];

  for (const MctsPolicy& policy : _policies) {
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
 * Return a newly created node if the child does not exist.
 * Do not register the newly created node in the child list.
 * @param move Move
 * @return Node corresponding to the move
 */
MctsNode* MctsNode::getChild(const Move& move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // Return the existing child node
  int32_t hash = move.getHash();
  auto child = _children.find(hash);

  if (child != _children.end()) {
    return child->second;
  }

  // Create an unregistered node if the child does not exist
  MctsNode* node = _manager->createNode();

  node->_resetNode();
  node->_board.copyFrom(&_board);
  node->_captured = node->_board.play(move).getCaptured();
  node->_board.updateStatus();
  node->_move = move;
  node->_komi = _getChildKomi(move);
  node->_passed = move.isPass() ? _passed + 1 : 0;
  node->_probability = 0.0f;

  return node;
}

/**
 * Removes the child node corresponding to the specified move.
 * @param move Move
 */
void MctsNode::removeChild(const Move& move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _children.erase(move.getHash());

  // Recompute the maximum from the remaining children
  int32_t pv_visits = 0;

  for (const auto& child : _children) {
    pv_visits = std::max(pv_visits, child.second->getVisits());
  }

  _pvVisits.store(pv_visits, std::memory_order_relaxed);
}

/**
 * Returns the visit count of this node.
 * @return Visit count
 */
int32_t MctsNode::getVisits() {
  return _visits.load(std::memory_order_relaxed);
}

/**
 * Get the maximum visit count among the immediate children.
 * @return Maximum child visit count
 */
int32_t MctsNode::getPvVisits() {
  return _pvVisits.load(std::memory_order_relaxed);
}

/**
 * Update the MCTS evaluation and predicted score difference.
 * @param value Evaluation value
 * @param score Predicted score difference
 */
void MctsNode::updateMctsValue(float value, float score) {
  _mctsValue.update(value, score);
}

/**
 * Returns the MCTS evaluation value.
 * @return MCTS evaluation value
 */
float MctsNode::getMctsValue() {
  return _mctsValue.getValue(_nodeValue);
}

/**
 * Get the predicted score difference aggregated by MCTS.
 * @return Mean predicted score difference
 */
float MctsNode::getMctsScore() {
  return _mctsValue.getScore(_nodeScore);
}

/**
 * Get the lower confidence bound of the MCTS evaluation.
 * @return Lower confidence bound
 */
float MctsNode::getMctsValueLCB() {
  return _mctsValue.getValueLCB(_move.getColor(), _nodeValue);
}

/**
 * Returns the priority based on PUCB.
 * @param totalVisits Total visit count
 * @param childrenSize Number of children of the parent node
 * @return Pair of minimum-visit eligibility and PUCB priority
 */
std::pair<bool, float> MctsNode::getPriorityByPUCB(
    int32_t totalVisits, int32_t childrenSize) {
  std::shared_lock<std::shared_mutex> lock(_mutex);

  int32_t visits = _mctsSelects.load(std::memory_order_relaxed);
  float pucb_constant_base = _manager->getParameter().getPucbConstantBase();
  float pucb_constant_init = _manager->getParameter().getPucbConstantInit();
  float pucb_min_visits_rate = _manager->getParameter().getPucbMinVisitsRate();

  // Add the Policy-based search bonus to the evaluation value from the current player's perspective
  float value = _mctsValue.getValue(_nodeValue) * _move.getColor();
  float c_pucb_inc = std::log((1 + totalVisits + pucb_constant_base) / pucb_constant_base);
  float c_pucb = pucb_constant_init * (1.0f + c_pucb_inc);
  float ucb = _probability * std::sqrt(static_cast<float>(totalVisits)) / (1 + visits);
  float avg_visits = static_cast<float>(totalVisits) / static_cast<float>(childrenSize);
  float min_visits = avg_visits * pucb_min_visits_rate;

  return std::make_pair(static_cast<float>(visits) < min_visits, value + c_pucb * ucb);
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
 * @return Predicted territory probabilities with settled territories applied
 */
std::array<float, MODEL_TERRITORY_SIZE> MctsNode::getTerritories() {
  // Lock the synchronization mutex
  std::shared_lock<std::shared_mutex> lock(_mutex);

  // Copy the model's predicted territory probabilities for this node
  std::array<float, MODEL_TERRITORY_SIZE> territories = _territories;
  const int32_t width = _board.getWidth();
  const int32_t height = _board.getHeight();
  std::vector<int32_t> board_territories(width * height);

  // Get settled territories from this node's board
  _board.getFixedTerritories(board_territories.data());

  const int32_t model_length = MODEL_BOARD_SIZE * MODEL_BOARD_SIZE;
  const int32_t x_begin = (MODEL_BOARD_SIZE - width) / 2;
  const int32_t y_begin = (MODEL_BOARD_SIZE - height) / 2;

  // Apply settled territories to the predicted territory probabilities
  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      const int32_t board_index = y * width + x;
      const int32_t model_index = (y_begin + y) * MODEL_BOARD_SIZE + (x_begin + x);

      if (board_territories[board_index] == COLOR_BLACK) {
        territories[model_index + 0 * model_length] = 0.0f;
        territories[model_index + 1 * model_length] = 0.0f;
        territories[model_index + 2 * model_length] = 1.0f;
      } else if (board_territories[board_index] == COLOR_WHITE) {
        territories[model_index + 0 * model_length] = 1.0f;
        territories[model_index + 1 * model_length] = 0.0f;
        territories[model_index + 2 * model_length] = 0.0f;
      }
    }
  }

  return territories;
}

/**
 * Initializes all state except the board.
 */
void MctsNode::_resetNode() {
  _move = Move::createPassMove(COLOR_WHITE);
  _komi = _manager->getParameter().getKomi();
  _passed = 0;
  _probability = 0.0f;

  _evaluating = false;
  _evaluated = false;
  _nodeValue = 0.0f;
  _nodeScore = 0.0f;
  _policies.clear();

  _parent = nullptr;
  _children.clear();
  _appearedBoardHashes.clear();

  _mctsSelects.store(0, std::memory_order_relaxed);
  _mctsProceeds.store(0, std::memory_order_relaxed);
  _mctsValue.reset();
  _visits.store(0, std::memory_order_relaxed);
  _pvVisits.store(0, std::memory_order_relaxed);

  _waitingPolicies = std::queue<MctsPolicy>();
  _waitingMoves.clear();
}

/**
 * Update this node's visit count and its parent's maximum child visit count.
 */
void MctsNode::_incrementVisits() {
  int32_t visits = _visits.fetch_add(1, std::memory_order_relaxed) + 1;
  MctsNode* parent_node = _parent;

  // Update the parent's maximum child visit count
  if (parent_node != nullptr) {
    int32_t current = parent_node->_pvVisits.load(std::memory_order_relaxed);

    while (current < visits &&
           !parent_node->_pvVisits.compare_exchange_weak(
               current, visits,
               std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
  }
}

/**
 * Get the komi used to evaluate the position after the specified move.
 * @param move Move
 * @return Komi used to evaluate the position after the move
 */
float MctsNode::_getChildKomi(const Move& move) const {
  // For on-board moves under Japanese rules, adjust komi
  // to convert Chinese area evaluation into Japanese territory evaluation
  if (getRule() == RULE_JP && !move.isPass()) {
    return _komi + move.getColor();
  }

  // Keep the current komi for Chinese-rule moves and Japanese-rule passes
  return _komi;
}

/**
 * Return true if the specified board violates superko.
 * @param board Board to check
 * @return True if the board violates superko
 */
bool MctsNode::_isSuperkoBoard(const Board* board) const {
  // Skip the check when superko is disabled
  if (!getSuperko()) {
    return false;
  }

  BoardHash board_hash(board);
  const MctsNode* current_node = this;
  const MctsNode* root_node = nullptr;

  // Compare the board against ancestors on the search path
  while (current_node != nullptr) {
    BoardHash appeared_hash(&current_node->_board);

    if (!(board_hash < appeared_hash) && !(appeared_hash < board_hash)) {
      return true;
    }

    root_node = current_node;
    current_node = current_node->_parent;
  }

  // Check positions that occurred before the root
  return root_node != nullptr &&
         root_node->_appearedBoardHashes.find(board_hash) !=
             root_node->_appearedBoardHashes.end();
}

/**
 * Calculate the predicted score difference from predicted territories.
 * @return Predicted score difference from Black's perspective
 */
float MctsNode::_calculateScoreFromTerritories() {
  const int32_t width = _board.getWidth();
  const int32_t height = _board.getHeight();
  const int32_t model_length = MODEL_BOARD_SIZE * MODEL_BOARD_SIZE;
  const int32_t x_begin = (MODEL_BOARD_SIZE - width) / 2;
  const int32_t y_begin = (MODEL_BOARD_SIZE - height) / 2;
  std::vector<int32_t> territories(width * height);
  std::vector<int32_t> fixed_territories(width * height);
  std::vector<int32_t> owners(width * height);

  // Use the most probable owner at each coordinate as its predicted territory
  for (int32_t y = 0; y < height; y++) {
    for (int32_t x = 0; x < width; x++) {
      int32_t board_index = y * width + x;
      int32_t model_index = (y_begin + y) * MODEL_BOARD_SIZE + (x_begin + x);
      int32_t max_owner = 0;

      for (int32_t owner = 1; owner < 3; owner++) {
        if (_territories[model_index + owner * model_length] >
            _territories[model_index + max_owner * model_length]) {
          max_owner = owner;
        }
      }

      // Convert channel indices into white, neutral, and black ownership values
      territories[board_index] = max_owner - 1;
    }
  }

  // Get settled territories from the board
  _board.getFixedTerritories(fixed_territories.data());

  // Apply settled territories to the predicted territories
  for (int32_t index = 0; index < width * height; index++) {
    if (fixed_territories[index] != COLOR_EMPTY) {
      territories[index] = fixed_territories[index];
    }
  }

  // Apply on-board stones and scoring rules to the predicted territories
  _board.getOwners(owners.data(), territories.data(), getRule());

  // Subtract komi from total ownership from Black's perspective
  return std::accumulate(owners.begin(), owners.end(), 0.0f) - _komi;
}

/**
 * Gets the next node to evaluate.
 * @param equally True if search count should be equally distributed
 * @param width Search width
 * @param temperature Temperature parameter for search
 * @param noise Strength of Gumbel noise
 * @return Next node to evaluate
 */
MctsNode* MctsNode::_pickupNextNode(bool equally, int32_t width, float temperature, float noise) {
  // If there are remaining Policy candidates and search width allows, add a new move as an
  // expansion candidate
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

    // Select the next candidate based on predicted probability, temperature, Gumbel noise, and
    // unexpanded-first priority
    for (int32_t i = 0; i < static_cast<int32_t>(_policies.size()); i++) {
      MctsPolicy& policy = _policies[i];
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
        int32_t policy_hash = policy.getMove().getHash();

        if (_children.find(policy_hash) != _children.end() ||
            _waitingMoves.find(policy_hash) != _waitingMoves.end()) {
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
    MctsPolicy& max_policy = _policies[max_index];
    int32_t max_policy_hash = max_policy.getMove().getHash();

    if (_children.find(max_policy_hash) == _children.end() &&
        _waitingMoves.find(max_policy_hash) == _waitingMoves.end()) {
      _waitingPolicies.push(max_policy);
      _waitingMoves.insert(max_policy_hash);
    }

    _policies[max_index].incrementVisits();
  }

  // If no search width is specified or the number of child nodes has not reached the specified
  // width,
  // and if there is a candidate in the waiting list, create a new child node and return it as the
  // next search target
  if (_waitingPolicies.size() > 0 && (width <= 0 || _children.size() < width)) {
    // Get the first registered candidate from the waiting list
    MctsPolicy policy = _waitingPolicies.front();
    int32_t policy_hash = policy.getMove().getHash();

    _waitingPolicies.pop();
    _waitingMoves.erase(policy_hash);

    // For an unregistered candidate, create and return a new child as the next search target
    // Initialize the node evaluation to the minimum evaluation value
    if (_children.find(policy_hash) == _children.end()) {
      // Create a new child node
      MctsNode* node = _manager->createNode();

      node->_resetNode();
      node->_board.copyFrom(&_board);
      node->_captured = node->_board.play(policy.getMove()).getCaptured();
      node->_board.updateStatus();
      node->_move = policy.getMove();
      node->_komi = _getChildKomi(policy.getMove());
      node->_passed = policy.getMove().isPass() ? _passed + 1 : 0;
      node->_probability = policy.getProbability();
      node->_nodeValue = _move.getColor();
      node->_parent = this;
      _children[policy_hash] = node;

      // Increment visit count
      node->_mctsSelects.fetch_add(1, std::memory_order_relaxed);

      // Return the new child node as the next search target
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
  int32_t total_visits = _mctsProceeds.load(std::memory_order_relaxed);
  int32_t max_priority_type = -1;
  float max_priority = -std::numeric_limits<float>::infinity();
  MctsNode* max_node = children[0].first;

  for (std::pair<MctsNode*, float> child : children) {
    int32_t priority_type = 0;
    float priority;

    // When search counts should be distributed equally,
    // prioritize by visit count and use evaluation values to break ties
    if (equally) {
      float visits = static_cast<float>(child.first->_mctsSelects.load(std::memory_order_relaxed));
      float value = child.first->getMctsValue() * getNextColor();
      priority = 1.0f / (visits + 1 - value * 0.5f);
    }
    // Otherwise, calculate priority based on PUCB
    else {
      std::pair<bool, float> pucb_priority = child.first->getPriorityByPUCB(
          total_visits, static_cast<int32_t>(_children.size()));

      priority_type = pucb_priority.first ? 1 : 0;
      priority = pucb_priority.second;
    }

    // Keep the node with the highest priority
    if (priority_type > max_priority_type ||
        (priority_type == max_priority_type && max_priority < priority)) {
      max_node = child.first;
      max_priority_type = priority_type;
      max_priority = priority;
    }
  }

  // Increment visit count
  max_node->_mctsSelects.fetch_add(1, std::memory_order_relaxed);

  // Return the node with the highest priority as the next search target
  return max_node;
}

}  // namespace deepgo
