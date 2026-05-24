#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

#include "MctsNode.h"
#include "Move.h"

namespace deepgo {

/**
 * 候補手クラス。
 */
class Candidate {
 public:
  /**
   * 候補手データを作成する。
   * @param move 着手
   * @param visits 訪問回数
   * @param playouts プレイアウト数
   * @param policy 予想着手確率
   * @param value 評価値
   * @param variations 予想進行
   * @param territories 予測領域確率
   */
  Candidate(
      Move move, int32_t visits, int32_t playouts,
      float policy, float value, const std::vector<Move> variations,
      const std::array<float, 3 * MODEL_SIZE * MODEL_SIZE>& territories);

  /**
   * ノードオブジェクトから候補手データを作成する。
   * @param node ノードオブジェクト
   */
  Candidate(MctsNode* node);

  /**
   * 候補手オブジェクトをコピーする。
   * @param other コピー元の候補手オブジェクト
   */
  Candidate(const Candidate& other);

  /**
   * 候補手オブジェクトを作成する。
   * 不正な候補手を表すオブジェクトを作成する。
   */
  Candidate();

  /**
   * 候補手の文字列表現を取得する。
   * @return 候補手の文字列表現
   */
  std::string toString() const;

  /**
   * インスタンスを破棄する。
   */
  virtual ~Candidate() = default;

  /**
   * 着手を取得する。
   * @return 着手
   */
  inline Move getMove() const {
    return _move;
  }

  /**
   * 訪問回数を取得する。
   * @return 訪問回数
   */
  inline int32_t getVisits() const {
    return _visits;
  }

  /**
   * プレイアウト数を取得する。
   * @return プレイアウト数
   */
  inline int32_t getPlayouts() const {
    return _playouts;
  }

  /**
   * 予想着手確率を取得する。
   * @return 予想着手確率
   */
  inline float getPolicy() const {
    return _policy;
  }

  /**
   * 評価値を取得する。
   * @return 評価値
   */
  inline float getValue() const {
    return _value;
  }

  /**
   * 予想進行を取得する。
   * @return 予想進行
   */
  inline std::vector<Move> getVariations() const {
    return _variations;
  }

  /**
   * 予測領域確率を取得する。
   * @param territories 予測領域確率を格納する配列
   */
  inline void getTerritories(float* territories) const {
    std::copy(std::begin(_territories), std::end(_territories), territories);
  }

  /**
   * 候補手の文字列表現を出力ストリームに書き込む。
   * @param os 出力ストリーム
   * @param candidate 候補手オブジェクト
   * @return 出力ストリーム
   */
  friend std::ostream& operator<<(std::ostream& os, const Candidate& candidate) {
    os << candidate.toString();
    return os;
  }

 private:
  /**
   * 着手。
   */
  Move _move;

  /**
   * 訪問回数。
   */
  int32_t _visits;

  /**
   * プレイアウト数。
   */
  int32_t _playouts;

  /**
   * 予想着手確率。
   */
  float _policy;

  /**
   * 評価値。
   */
  float _value;

  /**
   * 予想進行。
   */
  std::vector<Move> _variations;

  /**
   * 予測領域確率。
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;
};

}  // namespace deepgo
