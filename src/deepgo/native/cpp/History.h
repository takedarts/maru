#pragma once

#include <cstdint>
#include <vector>

namespace deepgo {

class History {
 public:
  /**
   * 着手履歴を保持するオブジェクトを生成する。
   */
  History();

  /**
   * コピーした着手履歴を保持するオブジェクトを生成する。
   * @param history コピー元の着手履歴
   */
  History(const History& moves);

  /**
   * オブジェクトを破棄する。
   */
  virtual ~History() = default;

  /**
   * 履歴を初期化する。
   */
  void clear();

  /**
   * 着手座標を追加する。
   * @param move 着手座標
   */
  void add(int32_t move);

  /**
   * 着手履歴を取得する。
   * @return 着手履歴
   */
  std::vector<int32_t> get();

 private:
  /**
   * 値を追加する位置。
   */
  int32_t _index;

  /**
   * 着手座標の一覧
   */
  int32_t _moves[3];
};

}  // namespace deepgo
