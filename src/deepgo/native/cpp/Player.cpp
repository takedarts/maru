#include "Player.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>

namespace deepgo {

/**
 * Creates a player object.
 * @param processor Object that executes inference
 * @param threads Number of threads
 * @param maxVisits Maximum number of visits
 * @param width Board width
 * @param height Board height
 * @param komi Komi value
 * @param rule Win/loss determination rule
 * @param superko True if the superko rule is applied
 * @param pucbConstantInit Initial value of the constant multiplied by the PUCB confidence bound
 * @param pucbConstantBase Incremental value of the constant multiplied by the PUCB confidence bound
 * @param pucbMinVisitsRate Minimum child visit ratio prioritized by PUCB
 */
Player::Player(
    InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    float pucbConstantInit, float pucbConstantBase, float pucbMinVisitsRate)
    : _mutex(),
      _searchCondition(),
      _updateCondition(),
      _stopCondition(),
      _waitCondition(),
      _processor(processor),
      _threadPool(threads),
      _searchThread(),
      _updateThread(),
      _nodeManager(MctsParameter(
          width, height, komi, rule, superko, pucbConstantInit, pucbConstantBase,
          pucbMinVisitsRate)),
      _root(_nodeManager.createNode()),
      _captureds({0, 0}),
      _searchMaxVisits(maxVisits),
      _searchEqually(false),
      _searchCandidateWidth(0),
      _searchTemperature(1.0f),
      _searchNoise(0.0f),
      _runnings(0),
      _paused(false),
      _stopped(true),
      _terminated(false),
      _canceled(false),
      _evaluatingNodes(),
      _updatingNodes(0) {
  _root->initialize();
  _searchThread = std::thread(&Player::_runSearch, this);
  _updateThread = std::thread(&Player::_runUpdate, this);
}

/**
 * Destroys the player object.
 */
Player::~Player() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _canceled.store(true, std::memory_order_release);
    _terminated = true;
  }

  _searchCondition.notify_one();
  _updateCondition.notify_one();
  _searchThread.join();
  _updateThread.join();
}

/**
 * Initializes the state of the player object.
 */
void Player::initialize() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the search thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Save the current search tree and create a new root for the initial position
  MctsNode* old_root = _root;

  _root = _nodeManager.createNode();
  _root->initialize();

  // Return the old search tree to the node pool so it can be reused
  _nodeManager.releaseTree(old_root);

  // Reset the captured stone counts
  _captureds = {0, 0};

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();
}

/**
 * Places a stone on the board.
 * @param move Move
 */
void Player::play(Move move) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the search thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Make the node corresponding to the played move the new root
  MctsNode* old_root = _root;

  _root = old_root->getChild(move);
  _root->copyAppearedBoardHashes(old_root);
  _root->setAsRootNode();

  // Accumulate the opponent's stones captured by this move
  if (move.getColor() == COLOR_BLACK) {
    _captureds[1] += _root->getCaptured();
  } else if (move.getColor() == COLOR_WHITE) {
    _captureds[0] += _root->getCaptured();
  }

  // Detach the new root from the old search tree and release the rest
  old_root->removeChild(move);
  _nodeManager.releaseTree(old_root);

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();
}

/**
 * Get the number of captured stones of the specified color.
 * @param color Stone color
 * @return Number of captured stones
 */
int32_t Player::getCaptured(int32_t color) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (color == COLOR_BLACK) {
    return _captureds[0];
  } else if (color == COLOR_WHITE) {
    return _captureds[1];
  } else {
    return 0;
  }
}

/**
 * Get the pass candidate move.
 * @return Pass candidate move
 */
Candidate Player::getPassCandidate() {
  // Create a pass node
  // Create an unevaluated node that is not associated with the root node
  // No need to pause the search thread as it does not affect other searches
  Move pass_move;
  MctsNode* pass_node;

  {
    std::unique_lock<std::mutex> lock(_mutex);

    pass_move = Move::createPassMove(_root->getNextColor());
    pass_node = _root->getChild(pass_move);
  }

  // Evaluate the pass node synchronously if it has not been evaluated
  _evaluateNode(pass_node);

  // Get the node value and create a candidate move object
  Candidate candidate(
      pass_move, 1, 0.0, pass_node->getNodeValue(), pass_node->getNodeScore(),
      std::vector<Move>(), pass_node->getTerritories());

  return candidate;
}

/**
 * Get the root node's predicted territories.
 * @param territories Array receiving predicted territories
 */
void Player::getPredictedTerritories(float* territories) {
  std::unique_lock<std::mutex> lock(_mutex);

  // To avoid races with root replacement and evaluation,
  // pause the search
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Evaluate the root synchronously if it has not been evaluated
  _evaluateNode(_root);

  // Copy the root's predicted territories with settled territories applied
  const std::array<float, MODEL_TERRITORY_SIZE> predicted_territories =
      _root->getTerritories();
  std::copy(
      predicted_territories.begin(), predicted_territories.end(), territories);

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();
}

/**
 * Get the root node's predicted score difference.
 * @return Predicted score difference from Black's perspective
 */
float Player::getPredictedScore() {
  std::unique_lock<std::mutex> lock(_mutex);

  // To avoid races with root replacement and evaluation,
  // pause the search
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Evaluate the root synchronously if it has not been evaluated
  _evaluateNode(_root);

  // Get the root node's predicted score difference
  float score = _root->getNodeScore();

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();

  return score;
}

/**
 * Starts board evaluation.
 * @param equally true to distribute search counts equally
 * @param width Search width for candidate moves
 * @param temperature Temperature parameter for search
 * @param noise Strength of Gumbel noise
 */
void Player::startEvaluation(
    bool equally, int32_t width, float temperature, float noise) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the running search to update all search conditions at once
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Update the conditions to be used in subsequent searches
  _searchEqually = equally;
  _searchCandidateWidth = width;
  _searchTemperature = temperature;
  _searchNoise = noise;

  // Set the search thread to active state
  _stopped = false;

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();
}

/**
 * Wait until the specified search conditions are satisfied.
 * @param visits Number of visits
 * @param timelimit Time limit
 * @param stop true to stop the search
 */
void Player::waitEvaluation(int32_t visits, float timelimit, bool stop) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Wait for the first evaluation
  if (visits > 0) {
    _waitCondition.wait(lock, [this]() {
      return _root->getVisits() > 0;
    });
  }

  // Wait until one of the following conditions is satisfied
  // [Condition 1] The requested number of visits is reached
  // [Condition 2] A candidate exceeds 60% of the requested visits
  // [Condition 3] The specified time has elapsed
  // [Condition 4] The maximum visit count is reached
  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));

  _waitCondition.wait_for(lock, timeout, [this, visits]() {
    if (_root->getVisits() >= _searchMaxVisits) {
      return true;
    } else if (_root->getVisits() >= visits) {
      return true;
    } else if (_root->getPvVisits() > static_cast<float>(visits) * 0.6f) {
      return true;
    } else {
      return false;
    }
  });

  // Set the stop flag if a stop state has been requested
  _stopped = _stopped || stop;
}

/**
 * Get the list of candidate moves.
 * @return List of candidate moves
 */
std::vector<Candidate> Player::getCandidates() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Create a list of candidate moves
  std::vector<Candidate> candidates;

  for (MctsNode* node : _root->getChildren()) {
    candidates.emplace_back(node);
  }

  // If there are no candidates, add a move from the Policy Network
  if (candidates.empty()) {
    Move move = _root->getPolicyMove();

    if (!move.isPass()) {
      candidates.emplace_back(
          move, 0, 1.0f, _root->getMctsValue(), _root->getMctsScore(),
          std::vector<Move>(), _root->getTerritories());
    }
  }

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();

  return candidates;
}

/**
 * Gets the color of the next stone.
 * @return Stone color
 */
int32_t Player::getColor() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _root->getNextColor();
}

/**
 * Copy the board state to the specified board object.
 * @param board Destination board object
 */
void Player::copyBoardTo(Board* board) {
  std::lock_guard<std::mutex> lock(_mutex);

  // Copy the board without racing with root replacement
  board->copyFrom(&_root->getBoard());
}

/**
 * Gets the string representation of the player object.
 * @return String representation of the player object
 */
std::string Player::toString() {
  std::unique_lock<std::mutex> lock(_mutex);
  std::stringstream ss;

  // Pause the thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _isSearchIdle();
  });

  // Convert the board state to a string
  ss << "--- Board ---" << std::endl
     << _root->getBoard() << std::endl;

  // Convert the current state to a string by traversing the search tree depth-first
  std::vector<std::pair<MctsNode*, std::string>> stack = {{_root, ""}};

  while (!stack.empty()) {
    MctsNode* current = stack.back().first;
    std::string prefix = stack.back().second;
    stack.pop_back();

    // Get the parent's child count for PUCB minimum-visit checks
    MctsNode* parent = current->getParent();
    int32_t children_size = (parent == nullptr)
                                ? 1
                                : static_cast<int32_t>(parent->getChildren().size());
    std::pair<bool, float> pucb_priority = current->getPriorityByPUCB(
        _root->getVisits(), children_size);

    ss << prefix
       << "Move=(" << current->getMove() << ")"
       << ", Visits=" << current->getVisits()
       << ", Value=" << std::setprecision(4) << current->getMctsValue()
       << ", Score=" << std::setprecision(4) << current->getMctsScore()
       << ", Policy=" << std::setprecision(4) << current->getProbability()
       << ", PUCB=" << std::setprecision(4) << pucb_priority.second
       << std::endl;

    std::vector<MctsNode*> children = current->getChildren();

    // Output in depth-first order, representing parent-child relationships with indentation
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      stack.emplace_back(*it, prefix + "  ");
    }
  }

  // Resume the thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();

  return ss.str();
}

/**
 * Return true when both searching and node updates are idle.
 * The caller must hold _mutex.
 * @return True when both searching and node updates are idle
 */
bool Player::_isSearchIdle() const {
  return _runnings == 0 && _evaluatingNodes.empty() && _updatingNodes == 0;
}

/**
 * Evaluate the specified node synchronously if it has not been evaluated.
 * @param node Node to evaluate
 */
void Player::_evaluateNode(MctsNode* node) {
  // Skip inference if the node has already been evaluated
  if (node->isEvaluated()) {
    return;
  }

  // To wait for node evaluation to finish,
  // create a synchronization object and a condition variable
  std::mutex mutex;
  std::condition_variable cv;
  std::unique_lock<std::mutex> lock(mutex);

  // Evaluate the node and wait for the inference result to be applied
  _processor->submit(node, [&cv](MctsNode*) {
    cv.notify_one();
  });
  cv.wait(lock, [node] {
    return node->isEvaluated();
  });
}

/**
 * Launches the search process.
 */
void Player::_runSearch() {
  // Calculate the maximum number of evaluating nodes
  const int32_t max_evaluating_size =
      _processor->getBatchSize() * _processor->getThreadSize() * 5;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mutex);

      // Wait until the search process becomes executable
      // Conditions for the search process to become executable are one of the following:
      // - [Stop] Termination is requested, no running threads, and no nodes being evaluated
      // - [Search] Neither termination, stop request, nor pause request is active,
      //   the number of running threads is less than the thread pool size,
      //   the number of evaluating nodes is less than the maximum, and visits are below the maximum
      _searchCondition.wait(lock, [this, max_evaluating_size]() {
        if (_terminated && _isSearchIdle()) {
          return true;
        } else if (
            !_terminated && !_stopped && !_paused &&
            _runnings < _threadPool.getSize() &&
            _evaluatingNodes.size() < static_cast<size_t>(max_evaluating_size) &&
            _root->getVisits() < _searchMaxVisits) {
          return true;
        } else {
          return false;
        }
      });

      // If the stop condition is met, exit the loop
      if (_terminated && _isSearchIdle()) {
        break;
      }

      // Otherwise, execute the search process
      _runnings += 1;
    }

    // Submit the search tree expansion process to the thread pool
    _threadPool.submit([this]() {
      _runExpand();

      {
        std::unique_lock<std::mutex> lock(_mutex);
        _runnings -= 1;
      }

      _searchCondition.notify_one();
      _updateCondition.notify_one();
      _stopCondition.notify_all();
    });
  }
}

/**
 * Expands the search tree.
 */
void Player::_runExpand() {
  // Copy the search settings to local variables
  bool search_equally = _searchEqually;
  int32_t search_width = _searchCandidateWidth;
  float search_temperature = _searchTemperature;
  float search_noise = _searchNoise;

  // Start the search from the root node
  // Traverse the search tree while getting the next node to evaluate
  MctsNode* node = nullptr;
  MctsNode* next_node = _root;

  while (true) {
    // Get the next node to evaluate
    node = next_node;
    next_node = node->pickupNextNode(
        search_equally, search_width, search_temperature, search_noise,
        [this]() { return _canceled.load(std::memory_order_acquire); });

    // If the search is canceled, end the search
    if (next_node == nullptr) {
      return;
    }

    // If there is no next node to evaluate, proceed to node evaluation
    if (next_node == node) {
      break;
    }

    // Update the search settings
    search_equally = false;
    search_width = 0;
    search_temperature = 1.0f;
    search_noise = 0.0f;
  }

  // The last call to `node->pickupNextNode()` has updated the visit count
  // Notify the waiting thread that the visit count has changed
  _waitCondition.notify_all();

  // If not yet evaluated
  if (!node->isEvaluated()) {
    // Register the node as an evaluation target in the board evaluation inference model
    _processor->submit(node, [this](MctsNode*) {
      std::unique_lock<std::mutex> lock(_mutex);
      _updateCondition.notify_one();
    });
  }

  // Add the node to the list of nodes being evaluated
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _evaluatingNodes.push(node);
    _updateCondition.notify_one();
  }
}

/**
 * Updates node states.
 */
void Player::_runUpdate() {
  while (true) {
    std::vector<MctsNode*> finished_nodes;

    {
      std::unique_lock<std::mutex> lock(_mutex);

      // Wait until the update process becomes executable
      // Conditions for the update process to become executable are one of the following:
      // - [Stop] Termination is requested, no running threads, and no nodes being evaluated.
      // - [Evaluate] There are nodes being evaluated and the evaluation of the front node is
      // complete
      _updateCondition.wait(lock, [this]() {
        if (_terminated && _isSearchIdle()) {
          return true;
        } else if (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
          return true;
        } else {
          return false;
        }
      });

      // If the stop condition is met, exit the loop
      if (_terminated && _isSearchIdle()) {
        break;
      }

      // Retrieve evaluated nodes
      while (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
        finished_nodes.push_back(_evaluatingNodes.front());
        _evaluatingNodes.pop();
      }

      // Allow shutdown to observe ongoing updates even when the queue is empty
      _updatingNodes += static_cast<int32_t>(finished_nodes.size());
    }

    // Propagate evaluated nodes' values and predicted scores to their ancestors
    for (MctsNode* node : finished_nodes) {
      float mcts_value = node->getNodeValue();
      float mcts_score = node->getNodeScore();
      MctsNode* current_node = node;

      while (current_node != nullptr) {
        current_node->updateMctsValue(mcts_value, mcts_score);
        current_node = current_node->getParent();
      }
    }

    {
      std::unique_lock<std::mutex> lock(_mutex);

      // Check that the update count is at least the amount to subtract
      assert(_updatingNodes >= static_cast<int32_t>(finished_nodes.size()));

      // Record completion of all statistics updates for the retrieved nodes
      _updatingNodes -= static_cast<int32_t>(finished_nodes.size());
    }

    // Notify the search process
    _searchCondition.notify_one();
    _stopCondition.notify_all();
  }
}

}  // namespace deepgo
