#pragma once

#include <cstdint>
#include <mutex>

namespace deepgo {

/**
 * Class managing MCTS evaluations and predicted score differences.
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
   * Reset the evaluation and predicted score difference.
   */
  void reset();

  /**
   * Update the evaluation and predicted score difference.
   * @param value Evaluation value
   * @param score Predicted score difference
   */
  void update(float value, float score);

  /**
   * Returns the average evaluation value.
   * @param defaultValue Value to return when the evaluation count is 0
   * @return Average evaluation value
   */
  float getValue(float defaultValue);

  /**
   * Get the mean predicted score difference.
   * @param defaultScore Predicted score to return when no evaluations exist
   * @return Mean predicted score difference
   */
  float getScore(float defaultScore);

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
   * Sum of predicted score differences.
   */
  float _score;

  /**
   * Number of evaluations.
   */
  int32_t _count;
};

}  // namespace deepgo
