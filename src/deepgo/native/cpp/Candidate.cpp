#include "Candidate.h"

#include <iomanip>
#include <sstream>

namespace deepgo {

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
Candidate::Candidate(
    Move move, int32_t visits, float policy, float value, float score,
    const std::vector<Move> variations,
    const std::array<float, MODEL_TERRITORY_SIZE>& territories)
    : _move(move),
      _visits(visits),
      _policy(policy),
      _value(value),
      _score(score),
      _variations(variations),
      _territories(territories) {
}

/**
 * Creates candidate move data from a node object.
 * @param node Node object
 */
Candidate::Candidate(MctsNode* node)
    : _move(node->getMove()),
      _visits(node->getVisits()),
      _policy(node->getProbability()),
      _value(node->getMctsValue()),
      _score(node->getMctsScore()),
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
      _policy(other._policy),
      _value(other._value),
      _score(other._score),
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
      _policy(0.0f),
      _value(0.0f),
      _score(0.0f),
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
  ss << ", Policy: " << std::fixed << std::setprecision(4) << _policy;
  ss << ", Value: " << std::fixed << std::setprecision(4) << _value;
  ss << ", Score: " << std::fixed << std::setprecision(2) << _score;
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
