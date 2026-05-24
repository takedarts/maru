#pragma once

#include <cstdint>
#include <mutex>

namespace deepgo {

/**
 * Class for managing MCTS evaluation values.
 */
class MctsValue {
 public:
  /**
   * Creates an evaluation value object.
   */
  MctsValue();

  /**
   * Copies an evaluation value object.
   * @param other Source object to copy from
   */
  MctsValue(const MctsValue& other);

  /**
   * Resets the evaluation value.
   */
  void reset();

  /**
   * Updates the evaluation value.
   * @param value Evaluation value
   */
  void update(float value);

  /**
   * Returns the average evaluation value.
   * @param defaultValue Value to return when the evaluation count is 0
   * @return Average evaluation value
   */
  float getValue(float defaultValue);

  /**
   * Returns the lower confidence bound of the evaluation value.
   * @param color Color of the played stone
   * @param defaultValue Value to return when the evaluation count is 0
   * @return Lower confidence bound of the evaluation value
   */
  float getValueLCB(int32_t color, float defaultValue);

 private:
  /**
   * Mutex for synchronization.
   */
  std::mutex _mutex;

  /**
   * Sum of evaluation values.
   */
  float _value;

  /**
   * Number of evaluations.
   */
  int32_t _count;
};

}  // namespace deepgo
