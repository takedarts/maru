#include "InferenceHash.h"

#include "Config.h"

namespace deepgo {

/**
 * Create an inference cache key.
 * @param board Board object
 * @param color Color of the stone to play next
 * @param komi Komi value
 * @param rule Win/loss determination rule
 * @param superko True if the superko rule is applied
 */
InferenceHash::InferenceHash(
    const Board* board, int32_t color, float komi, int32_t rule, bool superko)
    : _boardHash(board),
      _color(color),
      _koIndex(-1),
      _komi(static_cast<int32_t>(komi * color / 10.0f * MODEL_VALUE_SCALE)),
      _rule(rule),
      _superko(superko) {
  // Keep only the ko position active for the player to move
  std::pair<int32_t, int32_t> ko = board->getKo(color);

  if (ko.first >= 0 && ko.second >= 0) {
    _koIndex = ko.second * board->getWidth() + ko.first;
  }
}

/**
 * Compare cache keys.
 * @param other Cache key to compare against
 * @return True if this key is less than other
 */
bool InferenceHash::operator<(const InferenceHash& other) const {
  if (_boardHash < other._boardHash) {
    return true;
  }

  if (other._boardHash < _boardHash) {
    return false;
  }

  if (_color != other._color) {
    return _color < other._color;
  }

  if (_koIndex != other._koIndex) {
    return _koIndex < other._koIndex;
  }

  if (_komi != other._komi) {
    return _komi < other._komi;
  }

  if (_rule != other._rule) {
    return _rule < other._rule;
  }

  return _superko < other._superko;
}

}  // namespace deepgo
