#pragma once

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
   * @param move move
   * @param visits number of visits
   * @param playouts number of playouts
   * @param policy predicted move probability
   * @param value evaluation value
   * @param variations predicted sequence of moves
   * @param territories predicted territory probabilities
   */
  Candidate(
      Move move, int32_t visits, int32_t playouts,
      float policy, float value, const std::vector<Move> variations,
      const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories);

  /**
   * Creates candidate move data from a node object.
   * @param node node object
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
   * Returns the number of playouts.
   * @return number of playouts
   */
  inline int32_t getPlayouts() const {
    return _playouts;
  }

  /**
   * Returns the predicted move probability.
   * @return predicted move probability
   */
  inline float getPolicy() const {
    return _policy;
  }

  /**
   * Returns the evaluation value.
   * @return evaluation value
   */
  inline float getValue() const {
    return _value;
  }

  /**
   * Returns the predicted sequence of moves.
   * @return predicted sequence of moves
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
   * Number of playouts.
   */
  int32_t _playouts;

  /**
   * Predicted move probability.
   */
  float _policy;

  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Predicted sequence of moves.
   */
  std::vector<Move> _variations;

  /**
   * Predicted territory probabilities.
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;
};

}  // namespace deepgo
