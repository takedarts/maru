#pragma once

#include <cstdint>
#include <vector>

#include "Policy.h"

namespace deepgo {

/**
 * 盤面評価の結果を格納するクラス。
 */
class Evaluation {
 public:
  /**
   * モデルによる推論結果を格納するオブジェクトを作成する。
   * @param value モデルによる推論結果の評価値
   * @param policies モデルによる推論結果の候補手の一覧
   */
  Evaluation(float value, std::vector<Policy> policies);

  /**
   * モデルによる推論結果の候補手の一覧
   */
  inline const std::vector<Policy>& getPolicies() const {
    return _policies;
  }

  /**
   * モデルによる推論結果の評価値。
   */
  inline float getValue() const {
    return _value;
  }

 private:
  /**
   * モデルによる推論結果の候補手の一覧
   */
  std::vector<Policy> _policies;

  /**
   * モデルによる推論結果の評価値。
   */
  float _value;
};

}  // namespace deepgo
