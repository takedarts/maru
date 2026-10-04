#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Board.h"
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
   * @param komi Komi value
   * @param rule Win/loss determination rule
   * @param superko True if the superko rule is applied
   * @param pucbConstantInit Initial value of the constant multiplied by the PUCB confidence bound
   * @param pucbConstantBase Incremental value of the constant multiplied by the PUCB confidence
   * bound
   * @param pucbMinVisitsRate Minimum child visit ratio prioritized by PUCB
   */
  Player(
      InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      float pucbConstantInit, float pucbConstantBase, float pucbMinVisitsRate);

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
   * @param move Move
   */
  void play(Move move);

  /**
   * Get the number of captured stones of the specified color.
   * @param color Stone color
   * @return Number of captured stones
   */
  int32_t getCaptured(int32_t color);

  /**
   * Get the pass candidate move.
   * @return Pass candidate move
   */
  Candidate getPassCandidate();

  /**
   * Get the root node's predicted territories.
   * @param territories Array receiving predicted territories
   */
  void getPredictedTerritories(float* territories);

  /**
   * Get the root node's predicted score difference.
   * @return Predicted score difference from Black's perspective
   */
  float getPredictedScore();

  /**
   * Starts board evaluation.
   * @param equally true to distribute search counts equally
   * @param width Search width for candidate moves
   * @param temperature Temperature parameter for search
   * @param noise Strength of Gumbel noise
   */
  void startEvaluation(bool equally, int32_t width, float temperature, float noise);

  /**
   * Wait until the specified search conditions are satisfied.
   * @param visits Number of visits
   * @param timelimit Time limit
   * @param stop true to stop the search
   */
  void waitEvaluation(int32_t visits, float timelimit, bool stop);

  /**
   * Get the list of candidate moves.
   * @return List of candidate moves
   */
  std::vector<Candidate> getCandidates();

  /**
   * Gets the color of the next stone.
   * @return Stone color
   */
  int32_t getColor();

  /**
   * Copy the board state to the specified board object.
   * @param board Destination board object
   */
  void copyBoardTo(Board* board);

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
   * Condition variable for waiting until the requested visit count is reached.
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
   * Captured stone counts by color (Black, White).
   */
  std::array<int32_t, 2> _captureds;

  /**
   * Maximum number of visits.
   */
  int32_t _searchMaxVisits;

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
   * Number of nodes whose statistics are being updated after removal from the evaluation queue.
   */
  int32_t _updatingNodes;

  /**
   * Return true when both searching and node updates are idle.
   * The caller must hold _mutex.
   * @return True when both searching and node updates are idle
   */
  bool _isSearchIdle() const;

  /**
   * Evaluate the specified node synchronously if it has not been evaluated.
   * @param node Node to evaluate
   */
  void _evaluateNode(MctsNode* node);

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
