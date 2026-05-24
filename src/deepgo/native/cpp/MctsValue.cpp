#include "MctsValue.h"

#include <cmath>

#include "Config.h"

namespace deepgo {

/**
 * Creates an evaluation value object.
 */
MctsValue::MctsValue()
    : _mutex(),
      _value(0.0f),
      _count(0) {
}

/**
 * Copies an evaluation value object.
 * @param other Source object to copy from
 */
MctsValue::MctsValue(const MctsValue& other)
    : _mutex(),
      _value(other._value),
      _count(other._count) {
}

/**
 * Resets the evaluation value.
 */
void MctsValue::reset() {
  std::lock_guard<std::mutex> lock(_mutex);
  _value = 0.0f;
  _count = 0;
}

/**
 * Updates the evaluation value.
 * @param value Evaluation value
 */
void MctsValue::update(float value) {
  std::lock_guard<std::mutex> lock(_mutex);
  _value += value;
  _count++;
}

/**
 * Returns the average evaluation value.
 * @param defaultValue Value to return when the evaluation count is 0
 * @return Average evaluation value
 */
float MctsValue::getValue(float defaultValue) {
  std::lock_guard<std::mutex> lock(_mutex);
  return (_count != 0) ? _value / _count : defaultValue;
}

/**
 * Returns the lower confidence bound of the evaluation value.
 * @param color Color of the played stone
 * @param defaultValue Value to return when the evaluation count is 0
 * @return Lower confidence bound of the evaluation value
 */
float MctsValue::getValueLCB(int32_t color, float defaultValue) {
  std::lock_guard<std::mutex> lock(_mutex);
  float value = (_count != 0) ? _value / _count : defaultValue;
  float lower = 1.96f * 0.5f / std::sqrt(static_cast<float>(_count + 1));

  return value - (lower * color);
}

}  // namespace deepgo
