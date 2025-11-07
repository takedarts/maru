#include "Policy.h"

namespace deepgo {

/**
 * Creates an object for predicted move probability.
 * @param x X coordinate
 * @param y Y coordinate
 * @param policy Predicted move probability
 * @param visits Number of searches
 */
Policy::Policy(int32_t x, int32_t y, float policy, int32_t visits)
    : x(x),
      y(y),
      policy(policy),
      visits(visits) {
}

}  // namespace deepgo
