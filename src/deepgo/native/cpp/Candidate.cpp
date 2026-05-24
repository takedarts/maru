#include "Candidate.h"

#include <iomanip>
#include <sstream>

namespace deepgo {

/**
 * Creates candidate move data.
 * @param x x coordinate
 * @param y y coordinate
 * @param color stone color
 * @param visits number of visits
 * @param playouts number of playouts
 * @param policy predicted move probability
 * @param value evaluation value
 * @param variations predicted sequence of moves
 * @param territories predicted territory probabilities
 */
Candidate::Candidate(
    Move move, int32_t visits, int32_t playouts,
    float policy, float value, const std::vector<Move> variations,
    const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories)
    : _move(move),
      _visits(visits),
      _playouts(playouts),
      _policy(policy),
      _value(value),
      _variations(variations),
      _territories(territories) {
}

/**
 * Creates candidate move data from a node object.
 * @param node node object
 */
Candidate::Candidate(MctsNode* node)
    : _move(node->getMove()),
      _visits(node->getVisits()),
      _playouts(node->getPlayouts()),
      _policy(node->getProbability()),
      _value(node->getMctsValue()),
      _variations(node->getVariations()),
      _territories(node->getTerritories()) {
}

/**
 * Copies a candidate move object.
 * @param other source candidate move object to copy from
 */
Candidate::Candidate(const Candidate& other)
    : _move(other._move),
      _visits(other._visits),
      _playouts(other._playouts),
      _policy(other._policy),
      _value(other._value),
      _variations(other._variations),
      _territories(other._territories) {
}

/**
 * Creates a candidate move object.
 * Creates an object representing an invalid candidate move.
 */
Candidate::Candidate()
    : _move(MOVE_INVALID),
      _visits(0),
      _playouts(0),
      _policy(0.0f),
      _value(0.0f),
      _variations(),
      _territories() {
  std::fill(std::begin(_territories), std::end(_territories), 0.0f);
}

/**
 * Returns the string representation of the candidate move.
 * @return string representation of the candidate move
 */
std::string Candidate::toString() const {
  std::stringstream ss;

  ss << "Move: (" << _move << ")";
  ss << ", Visits: " << _visits;
  ss << ", Playouts: " << _playouts;
  ss << ", Policy: " << std::fixed << std::setprecision(4) << _policy;
  ss << ", Value: " << std::fixed << std::setprecision(4) << _value;
  ss << ", Variations: [";

  for (size_t i = 0; i < _variations.size(); ++i) {
    const auto& variation = _variations[i];
    ss << "(" << variation << ")";

    if (i < _variations.size() - 1) {
      ss << ", ";
    }
  }

  ss << "]";

  return ss.str();
}

}  // namespace deepgo
