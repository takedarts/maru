#pragma once

#include <cstdint>
#include <ostream>
#include <string>

#include "Move.h"

namespace deepgo {

/**
 * Class that computes predicted move probabilities.
 */
class Policy {
 public:
  /**
   * Creates a predicted move probability object.
   * @param move move coordinate
   * @param probability predicted move probability
   * @param visits number of searches
   */
  Policy(Move move, float probability, int32_t visits);

  /**
   * Creates a copy of a predicted move probability object.
   * @param policy source predicted move probability to copy from
   */
  Policy(const Policy& policy) = default;

  /**
   * Creates a predicted move probability object.
   */
  Policy();

  /**
   * Destroys the predicted move probability object.
   */
  virtual ~Policy() = default;

  /**
   * Returns the string representation of the predicted move probability object.
   * @return string representation of the predicted move probability object
   */
  std::string toString() const;

  /**
   * Returns the move coordinate.
   * @return move coordinate
   */
  inline Move getMove() const {
    return _move;
  }

  /**
   * Returns the predicted move probability.
   * @return predicted move probability
   */
  inline float getProbability() const {
    return _probability;
  }

  /**
   * Returns the number of searches.
   * @return number of searches
   */
  inline int32_t getVisits() const {
    return _visits;
  }

  /**
   * Increments the search count by 1.
   */
  inline void incrementVisits() {
    _visits += 1;
  }

  /**
   * Writes the string representation of the predicted move probability to an output stream.
   * @param os output stream
   * @param policy predicted move probability object
   * @return output stream
   */
  friend std::ostream& operator<<(std::ostream& os, const Policy& policy) {
    os << policy.toString();
    return os;
  }

 private:
  /**
   * Move coordinate.
   */
  Move _move;

  /**
   * Predicted move probability.
   */
  float _probability;

  /**
   * Number of searches.
   */
  int32_t _visits;
};

}  // namespace deepgo
