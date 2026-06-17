#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <shared_mutex>
#include <vector>

#include "Board.h"
#include "Config.h"
#include "InferenceResult.h"
#include "MctsParameter.h"
#include "MctsValue.h"
#include "Move.h"
#include "Policy.h"

namespace deepgo {

class MctsManager;

/**
 * Search node class.
 */
class MctsNode {
 public:
  /**
   * Creates a search node object.
   * @param manager Node management object
   */
  explicit MctsNode(MctsManager* manager);

  /**
   * Initializes this as an initial board node.
   */
  void initialize();

  /**
   * Initializes this as an initial board node.
   * @param board Board state
   * @param x X coordinate of the move
   * @param y Y coordinate of the move
   * @param previousColor Color of the last played stone
   * @param captured Number of captured stones
   */
  void initialize(
      const Board* board, int x, int y, int32_t previousColor, int32_t captured);

  /**
   * Applies an inference result.
   * @param result Inference result
   */
  void applyInferenceResult(const InferenceResult& result);

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
  MctsNode* pickupNextNode(
      bool equally, int32_t width, float temperature, float noise,
      std::function<bool()> isCanceled);

  /**
   * Sets this node as the root node.
   */
  void setAsRootNode();

  /**
   * Returns true if this node has been evaluated.
   * @return True if evaluated
   */
  bool isEvaluated();

  /**
   * Returns the board evaluation value.
   * @return Board evaluation value
   */
  float getNodeValue();

  /**
   * Sets the color of the previously played stone.
   * @param color Color of the previously played stone
   */
  void setPreviousColor(int32_t color);

  /**
   * Returns the komi.
   * @return Komi
   */
  float getKomi() const;

  /**
   * Returns the rule.
   * @return Rule
   */
  int32_t getRule() const;

  /**
   * Returns true if the superko rule is applied.
   * @return True if the superko rule is applied
   */
  bool getSuperko() const;

  /**
   * Returns the candidate move with the highest PolicyNetwork evaluation value.
   * @return Candidate move
   */
  Move getPolicyMove();

  /**
   * Returns the list of child nodes.
   * @return List of child nodes
   */
  std::vector<MctsNode*> getChildren();

  /**
   * Returns the parent node.
   * @return Parent node
   */
  MctsNode* getParent();

  /**
   * Returns the node corresponding to the specified move.
   * Returns nullptr if the child node does not exist.
   * @param move Move
   * @return Node
   */
  MctsNode* getChild(Move move);

  /**
   * Creates a node corresponding to the specified move.
   * Even if a child node exists, a new node is created.
   * The newly created node does not have a parent-child relationship with this node.
   * @param move Move
   * @return Created node
   */
  MctsNode* createNode(Move move);

  /**
   * Removes the child node corresponding to the specified move.
   * @param move Move
   */
  void removeChild(Move move);

  /**
   * Returns the visit count of this node.
   * @return Visit count
   */
  int32_t getVisits();

  /**
   * Returns the playout count.
   * @return Playout count
   */
  int32_t getPlayouts();

  /**
   * Updates the MCTS evaluation value.
   * @param value Evaluation value
   */
  void updateMctsValue(float value);

  /**
   * Returns the MCTS evaluation value.
   * @return MCTS evaluation value
   */
  float getMctsValue();

  /**
   * Returns the lower confidence bound of the MCTS evaluation value.
   * @return Lower confidence bound
   */
  float getMctsValueLCB();

  /**
   * Returns the priority based on PUCB.
   * @param totalVisits Total visit count
   * @return Priority
   */
  float getPriorityByPUCB(int32_t totalVisits);

  /**
   * Returns the predicted variation from this node.
   * @return Predicted variation
   */
  std::vector<Move> getVariations();

  /**
   * Returns the predicted territory probabilities.
   * @return Predicted territory probabilities
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> getTerritories();

  /**
   * Returns the board state.
   * @return Board state
   */
  std::vector<int32_t> getBoardState();

  /**
   * Returns the board.
   * @return Board
   */
  inline const Board& getBoard() {
    return _board;
  }

  /**
   * Returns the move information.
   * @return Move information
   */
  inline const Move& getMove() {
    return _move;
  }

  /**
   * Returns the color of the next stone to play.
   * @return Color of the next stone to play
   */
  inline int32_t getNextColor() const {
    return OPPOSITE(_move.getColor());
  }

  /**
   * Returns the number of stones captured at this node.
   * @return Number of captured stones
   */
  inline int32_t getCaptured() const {
    return _captured;
  }

  /**
   * Returns the predicted move probability of this node.
   * @return Predicted move probability
   */
  inline float getProbability() const {
    return _probability;
  }

 private:
  /**
   * Mutex for synchronization.
   */
  std::shared_mutex _mutex;

  /**
   * Condition variable for waiting on evaluation completion.
   */
  std::condition_variable_any _condition;

  /**
   * Node management object.
   */
  MctsManager* _manager;

  /**
   * Board to be evaluated at this node.
   */
  Board _board;

  /**
   * Move.
   */
  Move _move;

  /**
   * Number of captured stones.
   */
  int32_t _captured;

  /**
   * Predicted move probability.
   */
  float _probability;

  /**
   * True if this is the first created child node.
   */
  bool _firstChild;

  /**
   * True if currently being evaluated.
   */
  bool _evaluating;

  /**
   * True if already evaluated.
   */
  bool _evaluated;

  /**
   * Board evaluation value.
   */
  float _nodeValue;

  /**
   * List of next move probabilities.
   */
  std::vector<Policy> _policies;

  /**
   * Parent node.
   */
  MctsNode* _parent;

  /**
   * List of child nodes.
   */
  std::map<int32_t, MctsNode*> _children;

  /**
   * Visit count.
   */
  std::atomic<int32_t> _visits;

  /**
   * Playout count.
   */
  std::atomic<int32_t> _playouts;

  /**
   * MCTS evaluation value.
   */
  MctsValue _mctsValue;

  /**
   * Number of times this node was selected in MCTS.
   * This variable is updated by the parent node when selected by the parent node.
   */
  std::atomic<int32_t> _mctsSelects;

  /**
   * Number of times exploration was performed at this node in MCTS.
   * This variable is updated by this node when expanding the search tree.
   */
  std::atomic<int32_t> _mctsProceeds;

  /**
   * List of predicted territory probabilities.
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;

  /**
   * Queue of candidate moves waiting to be registered as child nodes.
   */
  std::queue<Policy> _waitingPolicies;

  /**
   * Set of candidate moves waiting to be registered as child nodes.
   */
  std::set<int32_t> _waitingMoves;

  /**
   * Initializes all state except the board.
   */
  void _resetNode();

  /**
   * Returns the next node to evaluate.
   * @param equally True to equalize the visit count
   * @param width Search width
   * @param temperature Temperature parameter for search
   * @param noise Gumbel noise strength
   * @return Next node to evaluate
   */
  MctsNode* _pickupNextNode(bool equally, int32_t width, float temperature, float noise);
};

}  // namespace deepgo
