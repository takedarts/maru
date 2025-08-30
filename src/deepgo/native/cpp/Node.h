#pragma once

#include <cstdint>
#include <iostream>
#include <queue>
#include <shared_mutex>

#include "Board.h"
#include "Config.h"
#include "Evaluator.h"
#include "NodeResult.h"
#include "Policy.h"

namespace deepgo {
class NodeManager;

/**
 * Search node class.
 */
class Node {
 public:
  /**
   * Create a search node object.
   * @param manager Node manager object
   * @param processor Object to execute inference
   * @param width Board width
   * @param height Board height
   * @param komi Komi points
   * @param rule Rule for determining the winner
   * @param superko True to apply superko rule
   */
  Node(
      NodeManager* manager, Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko);

  /**
   * Set as the initial board node.
   */
  void initialize();

  /**
   * Evaluate the search node.
   * @param equally True to equalize the number of searches
   * @param width Search width (if 0, width is automatically adjusted)
   * @param useUcb1 True to use UCB1, false to use PUCB
   * @param temperature Temperature parameter for search
   * @param noise Strength of Gumbel noise
   * @return Evaluation result
   */
  NodeResult evaluate(
      bool equally, int32_t width, bool useUcb1, float temperature, float noise);

  /**
   * Update the evaluation value of the search node.
   * @param value Evaluation value
   */
  void updateValue(float value);

  /**
   * Cancel the evaluation value of the search node.
   * @param value Evaluation value
   */
  void cancelValue(float value);

  /**
   * Get a candidate move randomly based on the PolicyNetwork evaluation value.
   * @param temperature Temperature
   * @return Candidate move
   */
  std::pair<int32_t, int32_t> getRandomMove(float temperature);

  /**
   * Get the candidate move with the highest PolicyNetwork evaluation value.
   * @return Candidate move
   */
  std::pair<int32_t, int32_t> getPolicyMove();

  /**
   * Get the x-coordinate of the move.
   * @return x-coordinate
   */
  int32_t getX() const;

  /**
   * Get the y-coordinate of the move.
   * @return y-coordinate
   */
  int32_t getY() const;

  /**
   * Get the color of the placed stone.
   * @return Stone color
   */
  int32_t getColor() const;

  /**
   * Get the number of captured stones in this node.
   * @return Number of captured stones
   */
  int32_t getCaptured() const;

  /**
   * Get the predicted move probability of this node.
   * @return Predicted move probability
   */
  float getPolicy() const;

  /**
   * Get the list of child nodes.
   * @return List of node objects
   */
  std::vector<Node*> getChildren();

  /**
   * Get the node object when a move is made at the specified coordinates.
   * If the node object does not exist, a new object is returned.
   * The created node object is not registered as a child node of this node object.
   * @param x X-coordinate of the move
   * @param y Y-coordinate of the move
   * @return Pointer to the node object
   */
  Node* getChild(int32_t x, int32_t y);

  /**
   * Get the number of searches for this node.
   * @return Number of searches
   */
  int32_t getVisits();

  /**
   * Get the number of playouts.
   * @return Number of playouts
   */
  int32_t getPlayouts();

  /**
   * Set the number of playouts.
   * @param playouts Number of playouts
   */
  void setPlayouts(int32_t playouts);

  /**
   * Get the evaluation value of this node.
   * @return Evaluation value
   */
  float getValue();

  /**
   * Get the number of times the evaluation value has been added for this node.
   * @return Number of additions to evaluation value
   */
  int getCount();

  /**
   * Get the lower bound of the confidence interval for the evaluation value of this node.
   * @return Lower bound of confidence interval
   */
  float getValueLCB();

  /**
   * Get the priority of this node based on PUCB.
   * @param totalVisits Total number of searches
   * @return Priority
   */
  float getPriorityByPUCB(int32_t totalVisits);

  /**
   * Get the priority of this node based on UCB1.
   * @param totalVisits Total number of searches
   * @return Priority
   */
  float getPriorityByUCB1(int32_t totalVisits);

  /**
   * Get the predicted sequence for this node.
   * @return Predicted sequence
   */
  std::vector<std::pair<int32_t, int32_t>> getVariations();

  /**
   * Get the board state.
   * @return Board state
   */
  std::vector<int32_t> getBoardState();

  /**
   * Output the information of this node.
   * @param os Output destination
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * Mutex for evaluation synchronization.
   */
  std::shared_mutex _evalMutex;

  /**
   * Mutex for value synchronization.
   */
  std::shared_mutex _valueMutex;

  /**
   * Node manager object.
   */
  NodeManager* _manager;

  /**
   * Board to be evaluated in this node.
   */
  Board _board;

  /**
   * X-coordinate of the move.
   */
  int32_t _x;

  /**
   * Y-coordinate of the move.
   */
  int32_t _y;

  /**
   * Color of the placed stone.
   */
  int32_t _color;

  /**
   * Number of captured stones.
   */
  int32_t _captured;

  /**
   * Predicted move probability.
   */
  float _policy;

  /**
   * Object to evaluate the board.
   */
  Evaluator _evaluator;

  /**
   * List of child nodes.
   */
  std::unordered_map<int32_t, Node*> _children;

  /**
   * List of next move probabilities.
   */
  std::vector<Policy> _childPolicies;

  /**
   * List of candidate moves waiting to be registered as child nodes.
   */
  std::queue<Policy> _waitingQueue;

  /**
   * Set of candidate moves waiting to be registered as child nodes.
   */
  std::set<int32_t> _waitingSet;

  /**
   * Number of searches.
   */
  int32_t _visits;

  /**
   * Number of playouts.
   */
  int32_t _playouts;

  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Number of additions to evaluation value.
   */
  int32_t _count;

  /**
   * Execute the evaluation of this node.
   */
  void _evaluate();

  /**
   * Initialize the evaluation information of the node.
   */
  void _reset();

  /**
   * Set the value as a continuation node of the specified node.
   * @param prevNode Previous node
   * @param x X-coordinate of the move
   * @param y Y-coordinate of the move
   * @param policy Predicted move probability
   */
  void _setAsNextNode(Node* prevNode, int32_t x, int32_t y, float policy);
};

}  // namespace deepgo
