#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "Config.h"
#include "Move.h"

namespace deepgo {

/**
 * Structure representing an inference result.
 */
class InferenceResult {
 public:
  /**
   * Creates an inference result.
   * @param value Evaluation value
   * @param score Predicted score difference
   * @param policies Predicted probabilities of candidate moves
   * @param territories Predicted probabilities of territories
   */
  InferenceResult(
      float value, float score,
      const std::vector<std::pair<Move, float>>& policies,
      const std::array<float, MODEL_TERRITORY_SIZE>& territories);

  /**
   * Copies an inference result.
   * @param other Inference result to copy from
   */
  InferenceResult(const InferenceResult& other);

  /**
   * Creates an inference result.
   */
  InferenceResult();

  /**
   * Destroys the inference result.
   */
  virtual ~InferenceResult() = default;

  /**
   * Returns the evaluation value.
   * @return Evaluation value
   */
  inline float getValue() const {
    return _value;
  }

  /**
   * Return the predicted score difference.
   * @return Predicted score difference
   */
  inline float getScore() const {
    return _score;
  }

  /**
   * Return predicted candidate move probabilities.
   * @return Predicted candidate move probabilities
   */
  inline const std::vector<std::pair<Move, float>>& getPolicies() const {
    return _policies;
  }

  /**
   * Returns the predicted probabilities of territories.
   * @return Predicted probabilities of territories
   */
  inline const std::array<float, MODEL_TERRITORY_SIZE>& getTerritories() const {
    return _territories;
  }

 private:
  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Predicted score difference.
   */
  float _score;

  /**
   * Predicted probabilities of candidate moves.
   */
  std::vector<std::pair<Move, float>> _policies;

  /**
   * Predicted territory probabilities.
   */
  std::array<float, MODEL_TERRITORY_SIZE> _territories;
};

}  // namespace deepgo
