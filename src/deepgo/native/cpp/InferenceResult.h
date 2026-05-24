#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Config.h"
#include "Policy.h"

namespace deepgo {

/**
 * A class representing an inference result.
 */
class InferenceResult {
 public:
  /**
   * Creates an inference result.
   * @param value Evaluation value
   * @param policies Predicted probabilities of candidate moves
   * @param territories Predicted probabilities of territories
   */
  InferenceResult(
      float value, const std::vector<Policy>& policies,
      const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories);

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
   * Returns the predicted probabilities of candidate moves.
   * @return Predicted probabilities of candidate moves
   */
  inline const std::vector<Policy>& getPolicies() const {
    return _policies;
  }

  /**
   * Returns the predicted probabilities of territories.
   * @return Predicted probabilities of territories
   */
  inline const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& getTerritories() const {
    return _territories;
  }

 private:
  /**
   * Evaluation value.
   */
  float _value;

  /**
   * Predicted probabilities of candidate moves.
   */
  std::vector<Policy> _policies;

  /**
   * Predicted probabilities of territories.
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;
};

}  // namespace deepgo
