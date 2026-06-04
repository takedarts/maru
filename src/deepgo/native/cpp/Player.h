#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Candidate.h"
#include "Config.h"
#include "InferenceProcessor.h"
#include "MctsManager.h"
#include "MctsNode.h"
#include "ThreadPool.h"

namespace deepgo {

/**
 * A class representing a player that manages game progression.
 */
class Player {
 public:
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
  Player(
      InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      float pucbConstantInit, float pucbConstantBase);

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
   * @param move Move to play
   * @return Number of captured stones
   */
  int32_t play(Move move);

  /**
   * Gets the pass candidate move.
   * @return Pass candidate move
   */
  std::vector<Candidate> getPass();

  /**
   * Starts board evaluation.
   * @param equally true to distribute search counts equally
   * @param width Search width for candidate moves
   * @param temperature Temperature parameter for search
   * @param noise Strength of Gumbel noise
   */
  void startEvaluation(bool equally, int32_t width, float temperature, float noise);

  /**
   * Waits until the specified visit count and playout count are reached.
   * @param visits Number of visits
   * @param playouts Number of playouts
   * @param timelimit Time limit
   * @param stop true to stop the search
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
   * Gets the board state.
   * @return Board state
   */
  std::vector<int32_t> getBoardState();

  /**
   * Gets the string representation of the player object.
   * @return String representation of the player object
   */
  std::string toString();

 private:
  /**
   * Synchronization object.
   */
  std::mutex _mutex;

  /**
   * Condition variable to trigger search.
   */
  std::condition_variable _searchCondition;

  /**
   * Condition variable to trigger node update processing.
   */
  std::condition_variable _updateCondition;

  /**
   * Condition variable to wait for search termination.
   */
  std::condition_variable _stopCondition;

  /**
   * Condition variable to wait until the specified visit and playout counts are met.
   */
  std::condition_variable _waitCondition;

  /**
   * Object that executes inference.
   */
  InferenceProcessor* _processor;

  /**
   * Thread management object.
   */
  ThreadPool _threadPool;

  /**
   * Search management thread.
   */
  std::thread _searchThread;

  /**
   * Update management thread.
   */
  std::thread _updateThread;

  /**
   * Object that manages search nodes.
   */
  MctsManager _nodeManager;

  /**
   * Root node.
   */
  MctsNode* _root;

  /**
   * Maximum number of visits.
   */
  int32_t _maxVisits;

  /**
   * true to distribute search counts equally.
   */
  bool _searchEqually;

  /**
   * Search width for candidate moves.
   */
  int32_t _searchCandidateWidth;

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
   * true if search is paused.
   */
  bool _paused;

  /**
   * true if search is stopped.
   */
  bool _stopped;

  /**
   * true if search has terminated.
   */
  bool _terminated;

  /**
   * true if search is canceled.
   */
  std::atomic<bool> _canceled;

  /**
   * Nodes awaiting evaluation.
   */
  std::queue<MctsNode*> _evaluatingNodes;

  /**
   * Launches the search process.
   */
  void _runSearch();

  /**
   * Expands the search tree.
   */
  void _runExpand();

  /**
   * Updates node states.
   */
  void _runUpdate();
};

}  // namespace deepgo
