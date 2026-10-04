#pragma once

#include <cmath>
#include <cstdint>
#include <ostream>
#include <vector>

#include "MctsNode.h"
#include "Move.h"

namespace deepgo {

/**
 * Candidate move class.
 */
class Candidate {
 public:
  /**
   * Creates candidate move data.
   * @param move Move
   * @param visits number of visits
   * @param policy predicted move probability
   * @param value Evaluation value
   * @param score Predicted score difference
   * @param variations predicted sequence of moves
   * @param territories predicted territory probabilities
   */
  Candidate(
      Move move, int32_t visits, float policy, float value, float score,
      const std::vector<Move> variations,
      const std::array<float, MODEL_TERRITORY_SIZE>& territories);

  /**
   * Creates candidate move data from a node object.
   * @param node Node object
   */
  Candidate(MctsNode* node);

  /**
   * Copies a candidate move object.
   * @param other source candidate move object to copy from
   */
  Candidate(const Candidate& other);

  /**
   * Creates a candidate move object.
   * Creates an object representing an invalid candidate move.
   */
  Candidate();

  /**
   * Returns the string representation of the candidate move.
   * @return string representation of the candidate move
   */
  std::string toString() const;

  /**
   * Destructor.
   */
  virtual ~Candidate() = default;

  /**
   * Returns the move.
   * @return move
   */
  inline Move getMove() const {
    return _move;
  }

  /**
   * Returns the number of visits.
   * @return number of visits
   */
  inline int32_t getVisits() const {
    return _visits;
  }

  /**
   * Get the predicted move probability.
   * @return Predicted move probability
   */
  inline float getPolicy() const {
    return _policy;
  }

  /**
   * Returns the evaluation value.
   * @return Evaluation value
   */
  inline float getValue() const {
    return _value;
  }

  /**
   * Get the predicted score difference.
   * @return Predicted score difference
   */
  inline float getScore() const {
    return _score;
  }

  /**
   * Returns the predicted sequence of moves.
   * @return Predicted variation
   */
  inline std::vector<Move> getVariations() const {
    return _variations;
  }

  /**
   * Returns the predicted territory probabilities.
   * @param territories array to store the predicted territory probabilities
   */
  inline void getTerritories(float* territories) const {
    std::copy(std::begin(_territories), std::end(_territories), territories);
  }

  /**
   * Returns the lower confidence bound of the evaluation value.
   * @return Lower confidence bound of the evaluation value
   */
  inline float getValueLCB() const {
    return _value - _move.getColor() * 1.96f * 0.5f / std::sqrt(_visits + 1);
  }

  /**
   * Returns the predicted win rate.
   * @return predicted win rate
   */
  inline float getWinChance() const {
    return _value * _move.getColor() * 0.5f + 0.5f;
  }

  /**
   * Returns the lower bound of the confidence interval for the predicted win rate.
   * @return lower bound of the confidence interval for the predicted win rate
   */
  inline float getWinChanceLCB() const {
    return getValueLCB() * _move.getColor() * 0.5f + 0.5f;
  }

  /**
   * Writes the string representation of the candidate move to an output stream.
   * @param os output stream
   * @param candidate candidate move object
   * @return output stream
   */
  friend std::ostream& operator<<(std::ostream& os, const Candidate& candidate) {
    os << candidate.toString();
    return os;
  }

 private:
  /**
   * Move.
   */
  Move _move;

  /**
   * Number of visits.
   */
  int32_t _visits;

  /**
   * Predicted move probability.
   */
  float _policy;

  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Predicted score difference.
   */
  float _score;

  /**
   * Predicted variation.
   */
  std::vector<Move> _variations;

  /**
   * Predicted territory probabilities.
   */
  std::array<float, MODEL_TERRITORY_SIZE> _territories;
};

}  // namespace deepgo
