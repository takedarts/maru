#include <sstream>

#include "Policy.h"

namespace deepgo {

/**
 * Creates a predicted move probability object.
 * @param move move coordinate
 * @param probability predicted move probability
 * @param visits number of searches
 */
Policy::Policy(Move move, float probability, int32_t visits)
    : _move(move),
      _probability(probability),
      _visits(visits) {
}

/**
 * Creates a predicted move probability object.
 */
Policy::Policy()
    : _move(Move()),
      _probability(0.0f),
      _visits(0) {
}

/**
 * Returns the string representation of the predicted move probability object.
 * @return string representation of the predicted move probability object
 */
std::string Policy::toString() const {
  std::stringstream ss;

  ss << "Policy(move=" << _move
     << ", probability=" << _probability
     << ", visits=" << _visits
     << ")";

  return ss.str();
}

}  // namespace deepgo
