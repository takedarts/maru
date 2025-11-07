#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "Board.h"
#include "Candidate.h"
#include "Config.h"
#include "Node.h"
#include "NodeManager.h"
#include "Processor.h"
#include "ThreadPool.h"

namespace deepgo {

/**
 * Class representing a player that progresses the game.
 */
class Player {
 public:
  /**
   * Creates a player object.
   * @param processor Object to execute inference
   * @param threads Number of threads
   * @param width Board width
   * @param height Board height
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply the superko rule
   * @param evalLeafOnly True to evaluate only leaf nodes
   */
  Player(
      Processor* processor, int32_t threads,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      bool evalLeafOnly);

  /**
   * Destroys the player object.
   */
  virtual ~Player();

  /**
   * Initializes the state of the player object.
   */
  void initialize();

  /**
   * Places a stone on the board.
   * @param x x coordinate of the position to place the stone
   * @param y y coordinate of the position to place the stone
   * @return Number of captured stones
   */
  int32_t play(int32_t x, int32_t y);

  /**
   * Gets candidate moves for pass.
   * @return Candidate moves for pass
   */
  std::vector<Candidate> getPass();

  /**
   * Selects the next candidate move randomly.
   * @param temperature Temperature (higher value increases randomness)
   * @return Randomly selected candidate move
   */
  std::vector<Candidate> getRandom(float temperature = 0.0);

  /**
   * Starts board evaluation.
   * Search processing is executed in a separate thread.
   * @param equally True to make the number of searches equal; false to use UCB1 or PUCB
   * @param useUcb1 True to use UCB1 as the search criterion; false to use PUCB
   * @param width Search width (if 0, search width is automatically adjusted)
   * @param temperature Temperature parameter for search
   * @param noise Strength of Gumbel noise for search
   */
  void startEvaluation(
      bool equally, bool useUcb1, int32_t width, float temperature, float noise);

  /**
   * Waits until the specified number of visits and playouts is reached.
   * @param visits Number of visits
   * @param playouts Number of playouts
   * @param timelimit Time limit
   * @param stop True to stop search
   */
  void waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop);

  /**
   * Gets the list of candidate moves.
   * @return List of candidate moves
   */
  std::vector<Candidate> getCandidates();

  /**
   * Gets the color of the next stone.
   * @return Stone color
   */
  int32_t getColor();

  /**
   * Gets the state of the board.
   * @return State of the board
   */
  std::vector<int32_t> getBoardState();

 private:
  /**
   * Synchronization object.
   */
  std::mutex _mutex;

  /**
   * Condition variable.
   */
  std::condition_variable _condition;

  /**
   * Object to manage search nodes.
   */
  NodeManager _nodeManager;

  /**
   * Thread management object.
   */
  ThreadPool _threadPool;

  /**
   * Thread to execute search.
   */
  std::unique_ptr<std::thread> _thread;

  /**
   * Root node.
   */
  Node* _root;

  /**
   * True to evaluate only leaf nodes.
   */
  bool _evalLeafOnly;

  /**
   * Number of visits to execute.
   */
  int32_t _searchVisits;

  /**
   * Number of playouts to execute.
   */
  int32_t _searchPlayouts;

  /**
   * True to make the number of searches equal.
   */
  bool _searchEqually;

  /**
   * True to use UCB1 as the search criterion.
   */
  bool _searchUseUcb1;

  /**
   * Search width.
   */
  int32_t _searchWidth;

  /**
   * Temperature parameter for search.
   */
  float _searchTemperature;

  /**
   * Strength of Gumbel noise for search.
   */
  float _searchNoise;

  /**
   * Number of running threads.
   */
  int32_t _runnings;

  /**
   * True if search is paused.
   */
  bool _paused;

  /**
   * True if search is stopped.
   */
  bool _stopped;

  /**
   * True if search is terminated.
   */
  bool _terminated;

  /**
   * Starts the search process.
   */
  void _run();

  /**
   * Executes search.
   * @return Number of search playouts
   */
  int32_t _evaluate();

  /**
   * Releases node objects other than the root node.
   * @param node Node object to release
   */
  void _releaseNode(Node* node);
};

}  // namespace deepgo
