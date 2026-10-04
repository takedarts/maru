#pragma once

#include <cstdint>

#include "Move.h"

namespace deepgo {

/**
 * Class managing predicted move probabilities in MCTS nodes.
 * Store the move, its predicted probability, and the number of times it was searched.
 */
class MctsPolicy {
 public:
  /**
   * Create an object managing a predicted move probability.
   * @param move Move information
   * @param probability Predicted move probability
   */
  MctsPolicy(Move move, float probability);

  /**
   * Copy an object managing a predicted move probability.
   * @param policy Source object to copy
   */
  MctsPolicy(const MctsPolicy& policy) = default;

  /**
   * Destroy an object managing a predicted move probability.
   */
  virtual ~MctsPolicy() = default;

  /**
   * Returns the move information.
   * @return Move information
   */
  inline Move getMove() const {
    return _move;
  }

  /**
   * Get the predicted move probability.
   * @return Predicted move probability
   */
  inline float getProbability() const {
    return _probability;
  }

  /**
   * Get the number of times this move was searched.
   * @return Number of times this move was searched
   */
  inline int32_t getVisits() const {
    return _visits;
  }

  /**
   * Increment this move's search count by one.
   */
  inline void incrementVisits() {
    _visits++;
  }

 private:
  /**
   * Move information.
   */
  Move _move;

  /**
   * Predicted move probability.
   */
  float _probability;

  /**
   * Number of times this move was searched.
   */
  int32_t _visits;
};

}  // namespace deepgo
