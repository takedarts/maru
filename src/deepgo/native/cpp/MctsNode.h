#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <shared_mutex>
#include <utility>
#include <vector>

#include "Board.h"
#include "BoardHash.h"
#include "Config.h"
#include "InferenceResult.h"
#include "MctsParameter.h"
#include "MctsPolicy.h"
#include "MctsValue.h"
#include "Move.h"

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
   * Detach the parent and reset evaluation and statistics when needed.
   */
  void setAsRootNode();

  /**
   * Inherit the historical positions when the root changes.
   * @param oldRootNode Previous root node
   */
  void copyAppearedBoardHashes(const MctsNode* oldRootNode);

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
   * Get the board's predicted score difference.
   * @return Predicted score difference of the board
   */
  float getNodeScore();

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
   * Return a newly created node if the child does not exist.
   * Do not register the newly created node in the child list.
   * @param move Move
   * @return Node corresponding to the move
   */
  MctsNode* getChild(const Move& move);

  /**
   * Removes the child node corresponding to the specified move.
   * @param move Move
   */
  void removeChild(const Move& move);

  /**
   * Returns the visit count of this node.
   * @return Visit count
   */
  int32_t getVisits();

  /**
   * Get the maximum visit count among the immediate children.
   * @return Maximum child visit count
   */
  int32_t getPvVisits();

  /**
   * Update the MCTS evaluation and predicted score difference.
   * @param value Evaluation value
   * @param score Predicted score difference
   */
  void updateMctsValue(float value, float score);

  /**
   * Returns the MCTS evaluation value.
   * @return MCTS evaluation value
   */
  float getMctsValue();

  /**
   * Get the predicted score difference aggregated by MCTS.
   * @return Mean predicted score difference
   */
  float getMctsScore();

  /**
   * Get the lower confidence bound of the MCTS evaluation.
   * @return Lower confidence bound
   */
  float getMctsValueLCB();

  /**
   * Returns the priority based on PUCB.
   * @param totalVisits Total visit count
   * @param childrenSize Number of children of the parent node
   * @return Pair of minimum-visit eligibility and PUCB priority
   */
  std::pair<bool, float> getPriorityByPUCB(
      int32_t totalVisits, int32_t childrenSize);

  /**
   * Returns the predicted variation from this node.
   * @return Predicted variation
   */
  std::vector<Move> getVariations();

  /**
   * Returns the predicted territory probabilities.
   * @return Predicted territory probabilities with settled territories applied
   */
  std::array<float, MODEL_TERRITORY_SIZE> getTerritories();

  /**
   * Returns the board state.
   * @return Board state
   */
  inline const Board& getBoard() const {
    return _board;
  }

  /**
   * Returns the move information.
   * @return Move information
   */
  inline const Move& getMove() const {
    return _move;
  }

  /**
   * Get the color of the next stone to play.
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
   * Komi used to evaluate this node's position.
   */
  float _komi;

  /**
   * Number of captured stones.
   */
  int32_t _captured;

  /**
   * Number of consecutive passes.
   */
  int32_t _passed;

  /**
   * Predicted move probability.
   */
  float _probability;

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
   * Predicted score difference of the board.
   */
  float _nodeScore;

  /**
   * List of next-move probabilities.
   */
  std::vector<MctsPolicy> _policies;

  /**
   * Parent node.
   */
  MctsNode* _parent;

  /**
   * List of child nodes.
   */
  std::map<int32_t, MctsNode*> _children;

  /**
   * Board hashes of positions that occurred before the root node.
   * Only valid for the root node.
   */
  std::set<BoardHash> _appearedBoardHashes;

  /**
   * Search count.
   */
  std::atomic<int32_t> _visits;

  /**
   * Maximum visit count among the immediate children.
   */
  std::atomic<int32_t> _pvVisits;

  /**
   * MCTS evaluation and predicted score difference.
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
  std::array<float, MODEL_TERRITORY_SIZE> _territories;

  /**
   * Queue of candidate moves waiting to be registered as child nodes.
   */
  std::queue<MctsPolicy> _waitingPolicies;

  /**
   * Set of candidate moves waiting to be registered as child nodes.
   */
  std::set<int32_t> _waitingMoves;

  /**
   * Initializes all state except the board.
   */
  void _resetNode();

  /**
   * Update this node's visit count and its parent's maximum child visit count.
   */
  void _incrementVisits();

  /**
   * Get the komi used to evaluate the position after the specified move.
   * @param move Move
   * @return Komi used to evaluate the position after the move
   */
  float _getChildKomi(const Move& move) const;

  /**
   * Return true if the specified board violates superko.
   * @param board Board to check
   * @return True if the board violates superko
   */
  bool _isSuperkoBoard(const Board* board) const;

  /**
   * Calculate the predicted score difference from predicted territories.
   * @return Predicted score difference from Black's perspective
   */
  float _calculateScoreFromTerritories();

  /**
   * Gets the next node to evaluate.
   * @param equally True if search count should be equally distributed
   * @param width Search width
   * @param temperature Temperature parameter for search
   * @param noise Strength of Gumbel noise
   * @return Next node to evaluate
   */
  MctsNode* _pickupNextNode(bool equally, int32_t width, float temperature, float noise);
};

}  // namespace deepgo
