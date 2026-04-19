#include "Evaluation.h"

namespace deepgo {

/**
 * モデルによる推論結果を格納するオブジェクトを作成する。
 * @param value モデルによる推論結果の評価値
 * @param policies モデルによる推論結果の候補手の一覧
 */
Evaluation::Evaluation(float value, std::vector<Policy> policies)
    : _value(value),
      _policies(policies) {
}

}  // namespace deepgo
