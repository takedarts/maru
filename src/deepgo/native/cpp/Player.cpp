#include "Player.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <unordered_map>

namespace deepgo {

/**
 * Creates a player object.
 * @param processor Object to evaluate the board
 * @param threads Number of board search threads
 * @param width Board width
 * @param height Board height
 * @param komi Komi points
 * @param rule Game rule
 * @param superko True to apply the superko rule
 * @param evalLeafOnly True to evaluate only leaf nodes
 */
Player::Player(
    Processor* processor, int32_t threads,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    bool evalLeafOnly)
    : _mutex(),
      _condition(),
      _nodeManager(processor, width, height, komi, rule, superko),
      _threadPool(threads),
      _thread(),
      _root(_nodeManager.createNode()),
      _evalLeafOnly(evalLeafOnly),
      _searchVisits(0),
      _searchPlayouts(0),
      _searchEqually(false),
      _searchUseUcb1(false),
      _searchWidth(0),
      _searchTemperature(1.0f),
      _searchNoise(0.0f),
      _runnings(0),
      _paused(false),
      _stopped(true),
      _terminated(false) {
  _thread.reset(new std::thread([this]() { this->_run(); }));
  _root->initialize();
}

/**
 * Destroys the player object.
 */
Player::~Player() {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }

  _thread->join();
}

/**
 * Initializes the state of the player object.
 */
void Player::initialize() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Save the current root node
  Node* old_root = _root;

  // Set the initial node as the root node
  _root = _nodeManager.createNode();
  _root->initialize();

  // Release nodes other than the root node
  _releaseNode(old_root);

  // Resume the thread
  _paused = false;
  _condition.notify_all();
}

/**
 * Places a stone on the board.
 * @param x x coordinate of the position to place the stone
 * @param y y coordinate of the position to place the stone
 */
int32_t Player::play(int32_t x, int32_t y) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Save the current root node
  Node* old_root = _root;

  // Set the new root node
  _root = old_root->getChild(x, y);

  // Release nodes other than the root node
  _releaseNode(old_root);

  // Resume the thread
  _paused = false;
  _condition.notify_all();

  // Return the number of captured stones
  return _root->getCaptured();
}

/**
 * Gets candidate moves for pass.
 * @return Candidate moves for pass
 */
std::vector<Candidate> Player::getPass() {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Create candidate moves
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      -1, -1, OPPOSITE(_root->getColor()),
      0, 0, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // Resume the thread
  _paused = false;
  _condition.notify_all();

  return candidates;
}

/**
 * Selects the next candidate move randomly.
 * @param temperature Temperature (higher value increases randomness)
 * @return Randomly selected candidate move
 */
std::vector<Candidate> Player::getRandom(float temperature) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Create candidate moves
  std::pair<int32_t, int32_t> move = _root->getRandomMove(temperature);
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      move.first, move.second, OPPOSITE(_root->getColor()),
      1, 1, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // Resume the thread
  _paused = false;
  _condition.notify_all();

  return candidates;
}

/**
 * Starts board evaluation.
 * Search processing is executed in a separate thread.
 * @param equally True to make the number of searches equal; false to use UCB1 or PUCB
 * @param useUcb1 True to use UCB1 as the search criterion; false to use PUCB
 * @param width Search width (if 0, search width is automatically adjusted)
 * @param temperature Temperature parameter for search
 * @param noise Strength of Gumbel noise for search
 */
void Player::startEvaluation(
    bool equally, bool useUcb1, int32_t width, float temperature, float noise) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Pause the thread
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Change search settings
  _searchVisits = _root->getVisits();
  _searchPlayouts = _root->getPlayouts();
  _searchEqually = equally;
  _searchUseUcb1 = useUcb1;
  _searchWidth = width;
  _searchTemperature = temperature;
  _searchNoise = noise;

  // Set to running state
  _stopped = false;

  // Resume the thread
  _paused = false;
  _condition.notify_all();
}

/**
 * Waits until the specified number of visits and playouts is reached.
 * @param visits Number of visits
 * @param playouts Number of playouts
 * @param timelimit Time limit
 * @param stop True to stop search
 */
void Player::waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop) {
  std::unique_lock<std::mutex> lock(_mutex);

  // Wait until the specified number of visits and playouts is reached
  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));
  _condition.wait_for(lock, timeout, [this, visits, playouts]() {
    return _searchVisits >= visits && _searchPlayouts >= playouts;
  });

  // Stop search
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
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // Set child nodes of the root node as candidate moves
  std::vector<Candidate> candidates;

  for (Node* node : _root->getChildren()) {
    candidates.emplace_back(
        node->getX(), node->getY(), node->getColor(),
        node->getVisits(), node->getPlayouts(),
        node->getPolicy(), node->getValue(), node->getVariations());
  }

  // If there are no candidate moves, add a move by PolicyNetwork
  if (candidates.empty()) {
    std::pair<int32_t, int32_t> move = _root->getPolicyMove();

    candidates.emplace_back(
        move.first, move.second, OPPOSITE(_root->getColor()),
        1, 1, 1.0f, _root->getValue(),
        std::vector<std::pair<int32_t, int32_t>>());
  }

  // Resume the thread
  _paused = false;
  _condition.notify_all();

  return candidates;
}

/**
 * Gets the color of the next stone.
 * @return Stone color
 */
int32_t Player::getColor() {
  return OPPOSITE(_root->getColor());
}

/**
 * Gets the state of the board.
 * @return State of the board
 */
std::vector<int32_t> Player::getBoardState() {
  return _root->getBoardState();
}

/**
 * Starts the search process.
 */
void Player::_run() {
  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _condition.wait(lock, [this]() {
        if (_terminated) {
          return true;
        } else if (!_stopped && !_paused && _runnings < _threadPool.getSize()) {
          return true;
        } else {
          return false;
        }
      });

      if (_terminated) {
        break;
      }

      _searchVisits += 1;
      _runnings += 1;
      _condition.notify_all();
    }

    _threadPool.submit([this]() {
      int32_t playouts = _evaluate();
      std::unique_lock<std::mutex> lock(_mutex);
      _searchPlayouts += playouts;
      _runnings -= 1;
      _condition.notify_all();
    });
  }
}

/**
 * Executes search.
 * @return Number of search playouts
 */
int32_t Player::_evaluate() {
  std::vector<Node*> nodes = {_root};
  bool search_equally = _searchEqually;
  int32_t search_width = _searchWidth;
  bool search_use_ucb1 = _searchUseUcb1;
  float search_temperature = _searchTemperature;
  float search_noise = _searchNoise;
  int32_t playouts = 0;

  while (true) {
    NodeResult result = nodes.back()->evaluate(
        search_equally, search_width, search_use_ucb1, search_temperature, search_noise);

    // Update the evaluation value of the node
    if (result.getPlayouts() == 1) {
      for (Node* node : nodes) {
        node->updateValue(result.getValue());
      }
    } else if (result.getPlayouts() == -1 && _evalLeafOnly) {
      for (Node* node : nodes) {
        node->cancelValue(result.getValue());
      }
    }

    // Update the number of playouts for the node
    for (Node* node : nodes) {
      node->setPlayouts(node->getPlayouts() + result.getPlayouts());
    }

    // Update the number of playouts for this search
    playouts += result.getPlayouts();

    // If a child node exists, set it as the next node
    if (result.getNode() != nullptr) {
      nodes.push_back(result.getNode());
    } else {
      break;
    }

    // Reset settings that apply only to the root node
    search_equally = 0;
    search_use_ucb1 = false;
    search_width = 0;
    search_temperature = 1.0f;
    search_noise = 0.0f;
  }

  // Return the number of playouts
  return playouts;
}

/**
 * Releases node objects other than the root node.
 * @param node Node object to release
 */
void Player::_releaseNode(Node* node) {
  std::vector<Node*> stack = {node};

  while (!stack.empty()) {
    Node* current = stack.back();
    stack.pop_back();

    if (current == _root) {
      continue;
    }

    for (Node* child : current->getChildren()) {
      stack.push_back(child);
    }

    _nodeManager.releaseNode(current);
  }
}

}  // namespace deepgo
