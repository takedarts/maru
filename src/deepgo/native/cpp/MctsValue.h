#pragma once

#include <cstdint>
#include <mutex>

namespace deepgo {

/**
 * MCTSの評価値を管理するクラス。
 */
class MctsValue {
 public:
  /**
   * 評価値オブジェクトを作成する。
   */
  MctsValue();

  /**
   * 評価値オブジェクトをコピーする。
   * @param other コピー元のオブジェクト
   */
  MctsValue(const MctsValue& other);

  /**
   * 評価値を初期化する。
   */
  void reset();

  /**
   * 評価値を更新する。
   * @param value 評価値
   */
  void update(float value);

  /**
   * 評価値の平均を取得する。
   * @param defaultValue 評価回数が0の場合に返す値
   * @return 評価値の平均
   */
  float getValue(float defaultValue);

  /**
   * 評価値の信頼区間の下限を取得する。
   * @param color 着手した石の色
   * @param defaultValue 評価回数が0の場合に返す値
   * @return 評価値の信頼区間の下限
   */
  float getValueLCB(int32_t color, float defaultValue);

 private:
  /**
   * 同期用ミューテックス。
   */
  std::mutex _mutex;

  /**
   * 評価値の合計。
   */
  float _value;

  /**
   * 評価回数。
   */
  int32_t _count;
};

}  // namespace deepgo
