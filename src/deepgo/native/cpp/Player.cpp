#include "Player.h"

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
 * @param komi Komi points
 * @param rule Win/loss determination rule
 * @param superko true to apply the superko rule
 * @param pucbConstantInit Initial value of the constant multiplied by the PUCB confidence upper bound
 * @param pucbConstantBase Change value of the constant multiplied by the PUCB confidence upper bound
 */
Player::Player(
    InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    float pucbConstantInit, float pucbConstantBase)
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
          width, height, komi, rule, superko, pucbConstantInit, pucbConstantBase)),
      _root(_nodeManager.createNode()),
      _maxVisits(maxVisits),
      _searchEqually(false),
      _searchCandidateWidth(0),
      _searchTemperature(1.0f),
      _searchNoise(0.0f),
      _runnings(0),
      _paused(false),
      _stopped(true),
      _terminated(false),
      _canceled(false),
      _evaluatingNodes() {
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
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // Save the current search tree and create a new root for the initial position
  MctsNode* old_root = _root;

  _root = _nodeManager.createNode();
  _root->initialize();

  // Return the old search tree to the node pool so it can be reused
  _nodeManager.releaseTree(old_root);

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();
}

/**
 * Places a stone on the board.
 * @param move Move to play
 * @return Number of captured stones
 */
int32_t Player::play(Move move) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the search thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // Set the child node at the played move as the new root
  MctsNode* old_root = _root;

  _root = old_root->getChild(move);
  _root->setAsRootNode();

  // Detach the new root from the old search tree and release the rest
  old_root->removeChild(move);
  _nodeManager.releaseTree(old_root);

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();

  return _root->getCaptured();
}

/**
 * Gets the pass candidate move.
 * @return Pass candidate move
 */
std::vector<Candidate> Player::getPass() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the search thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // Create the pass candidate move
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      Move::createPassMove(_root->getNextColor()), 0, 0, 1.0f,
      _root->getMctsValue(), std::vector<Move>(), _root->getTerritories());

  // Resume the search thread
  _paused = false;
  _canceled.store(false, std::memory_order_release);
  _searchCondition.notify_one();

  return candidates;
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
    return _runnings == 0 && _evaluatingNodes.empty();
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
 * Waits until the specified visit count and playout count are reached.
 * @param visits Number of visits
 * @param playouts Number of playouts
 * @param timelimit Time limit
 * @param stop true to stop the search
 */
void Player::waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Wait for the first evaluation
  if (visits > 0 || playouts > 0) {
    _waitCondition.wait(lock, [this]() {
      return _root->getVisits() > 0;
    });
  }

  // Wait until one of the following conditions is satisfied
  // [Condition 1] Both the visit and playout count of the root node reach the specified values
  // [Condition 2] The specified time has elapsed
  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));

  _waitCondition.wait_for(lock, timeout, [this, visits, playouts]() {
    return _root->getVisits() >= visits && _root->getPlayouts() >= playouts;
  });

  // Set the stop flag if a stop state has been requested
  _stopped = _stopped || stop;
}

/**
 * Gets the list of candidate moves.
 * @return List of candidate moves
 */
std::vector<Candidate> Player::getCandidates() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _canceled.store(true, std::memory_order_release);
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // Create the list of candidate moves
  std::vector<Candidate> candidates;

  for (MctsNode* node : _root->getChildren()) {
    candidates.emplace_back(node);
  }

  // If there are no candidates, add a move from the Policy Network
  if (candidates.empty()) {
    Move move = _root->getPolicyMove();

    if (!move.isPass()) {
      candidates.emplace_back(
          move, 0, 0, 1.0f, _root->getMctsValue(),
          std::vector<Move>(), _root->getTerritories());
    }
  }

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
 * Gets the board state.
 * @return Board state
 */
std::vector<int32_t> Player::getBoardState() {
  return _root->getBoardState();
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
    return _runnings == 0 && _evaluatingNodes.empty();
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

    ss << prefix
       << "Move=(" << current->getMove() << ")"
       << ", Visits=" << current->getVisits()
       << ", Playouts=" << current->getPlayouts()
       << ", Value=" << std::setprecision(4) << current->getMctsValue()
       << ", Policy=" << std::setprecision(4) << current->getProbability()
       << ", PUCB=" << std::setprecision(4) << current->getPriorityByPUCB(_root->getVisits())
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
        if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
          return true;
        } else if (
            !_terminated && !_stopped && !_paused &&
            _runnings < _threadPool.getSize() &&
            _evaluatingNodes.size() < static_cast<size_t>(max_evaluating_size) &&
            _root->getVisits() < _maxVisits) {
          return true;
        } else {
          return false;
        }
      });

      // If the stop condition is met, exit the loop
      if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
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

  // The visit and playout count have been updated by the last executed `node->pickupNextNode()`
  // Notify the waiting thread that the visit and playout counts have been updated
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
      // - [Evaluate] There are nodes being evaluated and the evaluation of the front node is complete
      _updateCondition.wait(lock, [this]() {
        if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
          return true;
        } else if (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
          return true;
        } else {
          return false;
        }
      });

      // If the stop condition is met, exit the loop
      if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
        break;
      }

      // Retrieve evaluated nodes
      while (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
        finished_nodes.push_back(_evaluatingNodes.front());
        _evaluatingNodes.pop();
      }
    }

    // Update the statistics of evaluated nodes
    // For nodes where a tsume-go sequence has been found, set the value to NodeValue
    for (MctsNode* node : finished_nodes) {
      float mcts_value = node->getNodeValue();
      MctsNode* current_node = node;

      while (current_node != nullptr) {
        current_node->updateMctsValue(mcts_value);
        current_node = current_node->getParent();
      }
    }

    // Notify the search process
    _searchCondition.notify_one();
    _stopCondition.notify_all();
  }
}

}  // namespace deepgo
