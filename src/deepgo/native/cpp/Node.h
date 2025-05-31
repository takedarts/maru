#pragma once

#include <cstdint>
#include <iostream>
#include <queue>
#include <shared_mutex>

#include "Board.h"
#include "Config.h"
#include "Evaluator.h"
#include "NodeResult.h"
#include "Policy.h"

namespace deepgo {
class NodeManager;

/**
 * 探索ノードクラス。
 */
class Node {
 public:
  /**
   * 探索ノードオブジェクトを作成する。
   * @param manager ノード管理オブジェクト
   * @param processor 推論を実行するオブジェクト
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @param komi コミの目数
   * @param rule 勝敗判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   */
  Node(
      NodeManager* manager, Processor* processor,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko);

  /**
   * ノードの評価情報を初期化する。
   */
  void reset();

  /**
   * 探索ノードを評価する。
   * @param equally 探索回数を均等にする場合はtrue
   * @param width 探索幅(0の場合は探索幅を自動で調整する)
   * @param useUcb1 UCB1を使用する場合はtrue・PUCBを使用する場合はfalse
   * @return 評価結果
   */
  NodeResult evaluate(bool equally, int32_t width, bool useUcb1);

  /**
   * 探索ノードを更新する。
   * @param value 反映する予想勝率
   */
  void update(float value);

  /**
   * PolicyNetworkの評価値をもとに候補手をランダムに取得する。
   * @param temperature 温度
   * @return 候補手
   */
  std::pair<int32_t, int32_t> getRandomMove(float temperature);

  /**
   * PolicyNetworkの評価値が最も高い候補手を取得する。
   * @return 候補手
   */
  std::pair<int32_t, int32_t> getPolicyMove();

  /**
   * 着手座標のx座標を取得する。
   * @return x座標
   */
  int32_t getX();

  /**
   * 着手座標のy座標を取得する。
   * @return y座標
   */
  int32_t getY();

  /**
   * 着手した石の色を取得する。
   * @return 石の色
   */
  int32_t getColor();

  /**
   * このノードで打ち上げた石の数を取得する。
   * @return 打ち上げた石の数
   */
  int32_t getCaptured();

  /**
   * このノードの予想着手確率を取得する。
   * @return 予想着手確率
   */
  float getPolicy();

  /**
   * 子ノードの一覧を取得する。
   * @return ノードオブジェクトの一覧
   */
  std::vector<Node*> getChildren();

  /**
   * 指定した座標に着手したときのノードオブジェクトを取得する。
   * ノードオブジェクトが存在しない場合は新しく作成したオブジェクトを返す。
   * 作成したノードオブジェクトはこのノードオブジェクトの子ノードとしては登録されない。
   * @param x 着手するX座標
   * @param y 着手するY座標
   * @return ノードオブジェクトへのポインタ
   */
  Node* getChild(int32_t x, int32_t y);

  /**
   * このノードの探索回数を取得する。
   * @return 探索回数
   */
  int32_t getVisits();

  /**
   * このノードの評価値を取得する。
   * @return 評価値
   */
  float getValue();

  /**
   * このノードの評価値の信頼区間の下限を取得する。
   * @return 信頼区間の下限
   */
  float getValueLCB();

  /**
   * PUCBに基づいてこのノードの優先度を取得する。
   * @param totalVisits 探索回数の合計
   * @return 優先度
   */
  float getPriorityByPUCB(int32_t totalVisits);

  /**
   * UCB1に基づいてこのノードの優先度を取得する。
   * @param totalVisits 探索回数の合計
   * @return 優先度
   */
  float getPriorityByUCB1(int32_t totalVisits);

  /**
   * このノードの予想進行を取得する。
   * @return 予想進行
   */
  std::vector<std::pair<int32_t, int32_t>> getVariations();

  /**
   * 盤面の状態を取得する。
   * @return 盤面の状態
   */
  std::vector<int32_t> getBoardState();

  /**
   * このノードの情報を出力する。
   * @param os 出力先
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * 評価同期用のミューテックス。
   */
  std::shared_mutex _evalMutex;

  /**
   * 値同期用のミューテックス。
   */
  std::shared_mutex _valueMutex;

  /**
   * ノード管理オブジェクト。
   */
  NodeManager* _manager;

  /**
   * このノードで評価する盤面。
   */
  Board _board;

  /**
   * 着手座標のx座標。
   */
  int32_t _x;

  /**
   * 着手座標のy座標。
   */
  int32_t _y;

  /**
   * 着手した石の色。
   */
  int32_t _color;

  /**
   * 打ち上げた石の数。
   */
  int32_t _captured;

  /**
   * 予想着手確率。
   */
  float _policy;

  /**
   * 盤面を評価するオブジェクト。
   */
  Evaluator _evaluator;

  /**
   * 子ノードの一覧。
   */
  std::unordered_map<int32_t, Node*> _children;

  /**
   * 次の着手確率の優先度付きキュー。
   */
  std::priority_queue<Policy> _priorityQueue;

  /**
   * 子ノードへの登録を待機している候補手の一覧。
   */
  std::queue<Policy> _waitingQueue;

  /**
   * 子ノードへの登録を待機している候補手のセット。
   */
  std::set<int32_t> _waitingSet;

  /**
   * 探索回数。
   */
  int32_t _visits;

  /**
   * 予想勝率。
   */
  float _value;

  /**
   * 予想勝率と予想目数の加算回数。
   */
  int32_t _count;

  /**
   * このノードの評価を実行する。
   */
  void _evaluate();

  /**
   * 指定されたノードの継続ノードとしての値を設定する。
   * @param prevNode 前のノード
   * @param x 着手するX座標
   * @param y 着手するY座標
   * @param policy 予想着手確率
   */
  void _setAsNextNode(Node* prevNode, int32_t x, int32_t y, float policy);
};

}  // namespace deepgo
