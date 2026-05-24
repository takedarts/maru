#include "InferenceResult.h"

namespace deepgo {

/**
 * 推論結果を作成する。
 * @param value 評価値
 * @param policies 候補手の予測確率
 * @param territories 地の予測確率
 */
InferenceResult::InferenceResult(
    float value, const std::vector<Policy>& policies,
    const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories)
    : _value(value),
      _policies(policies),
      _territories(territories) {
}

/**
 * 推論結果をコピーする。
 * @param other コピー元の推論結果
 */
InferenceResult::InferenceResult(const InferenceResult& other)
    : _value(other._value),
      _policies(other._policies),
      _territories(other._territories) {
}

/**
 * 推論結果を作成する。
 */
InferenceResult::InferenceResult()
    : _value(0.0f),
      _policies(),
      _territories({0}) {
}

}  // namespace deepgo
