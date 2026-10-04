#include "MctsPolicy.h"

namespace deepgo {

/**
 * Create an object managing a predicted move probability.
 * @param move Move information
 * @param probability Predicted move probability
 */
MctsPolicy::MctsPolicy(Move move, float probability)
    : _move(move),
      _probability(probability),
      _visits(0) {
}

}  // namespace deepgo
