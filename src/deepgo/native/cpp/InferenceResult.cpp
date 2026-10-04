#include "InferenceResult.h"

namespace deepgo {

/**
 * Creates an inference result.
 * @param value Evaluation value
 * @param score Predicted score difference
 * @param policies Predicted probabilities of candidate moves
 * @param territories Predicted probabilities of territories
 */
InferenceResult::InferenceResult(
    float value, float score,
    const std::vector<std::pair<Move, float>>& policies,
    const std::array<float, MODEL_TERRITORY_SIZE>& territories)
    : _value(value),
      _score(score),
      _policies(policies),
      _territories(territories) {
}

/**
 * Copies an inference result.
 * @param other Inference result to copy from
 */
InferenceResult::InferenceResult(const InferenceResult& other)
    : _value(other._value),
      _score(other._score),
      _policies(other._policies),
      _territories(other._territories) {
}

/**
 * Creates an inference result.
 */
InferenceResult::InferenceResult()
    : _value(0.0f),
      _score(0.0f),
      _policies(),
      _territories({0}) {
}

}  // namespace deepgo
