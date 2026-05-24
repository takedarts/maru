#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "Config.h"
#include "Policy.h"

namespace deepgo {

/**
 * 推論結果を表す構造体。
 */
class InferenceResult {
 public:
  /**
   * 推論結果を作成する。
   * @param value 評価値
   * @param policies 候補手の予測確率
   * @param territories 地の予測確率
   */
  InferenceResult(
      float value, const std::vector<Policy>& policies,
      const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories);

  /**
   * 推論結果をコピーする。
   * @param other コピー元の推論結果
   */
  InferenceResult(const InferenceResult& other);

  /**
   * 推論結果を作成する。
   */
  InferenceResult();

  /**
   * 推論結果を破棄する。
   */
  virtual ~InferenceResult() = default;

  /**
   * 評価値を返す。
   * @return 評価値
   */
  inline float getValue() const {
    return _value;
  }

  /**
   * 候補手の予測確率を返す。
   * @return 候補手の予測確率
   */
  inline const std::vector<Policy>& getPolicies() const {
    return _policies;
  }

  /**
   * 地の予測確率を返す。
   * @return 地の予測確率
   */
  inline const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& getTerritories() const {
    return _territories;
  }

 private:
  /**
   * 評価値。
   */
  float _value;

  /**
   * 候補手の予測確率。
   */
  std::vector<Policy> _policies;

  /**
   * 地の予測確率。
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;
};

}  // namespace deepgo
