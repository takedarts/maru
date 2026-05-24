#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <queue>
#include <set>
#include <shared_mutex>
#include <vector>

#include "Board.h"
#include "Config.h"
#include "InferenceResult.h"
#include "MctsParameter.h"
#include "MctsValue.h"
#include "Move.h"
#include "Policy.h"

namespace deepgo {

class MctsManager;

/**
 * 探索ノードクラス。
 */
class MctsNode {
 public:
  /**
   * 探索ノードオブジェクトを作成する。
   * @param manager ノード管理オブジェクト
   */
  explicit MctsNode(MctsManager* manager);

  /**
   * 初期盤面ノードとして設定する。
   */
  void initialize();

  /**
   * 初期盤面ノードとして設定する。
   * @param board 盤面
   * @param x 着手座標のX座標
   * @param y 着手座標のY座標
   * @param previousColor 直前に打った石の色
   * @param captured 打ち上げた石の数
   */
  void initialize(
      const Board* board, int x, int y, int32_t previousColor, int32_t captured);

  /**
   * 推論結果を適用する。
   * @param result 推論結果
   */
  void applyInferenceResult(const InferenceResult& result);

  /**
   * 次に評価するノードを取得する。
   * @param equally 探索回数を均等にする場合はtrue
   * @param width 探索幅
   * @param temperature 探索の温度パラメータ
   * @param noise ガンベルノイズの強さ
   * @return 次に評価するノード
   */
  MctsNode* pickupNextNode(bool equally, int32_t width, float temperature, float noise);

  /**
   * ルートノードとして設定する。
   */
  void setAsRootNode();

  /**
   * 評価済みならtrueを返す。
   * @return 評価済みならtrue
   */
  bool isEvaluated();

  /**
   * 盤面評価値を取得する。
   * @return 盤面評価値
   */
  float getNodeValue();

  /**
   * 直前に打った石の色を設定する。
   * @param color 直前に打った石の色
   */
  void setPreviousColor(int32_t color);

  /**
   * コミを取得する。
   * @return コミ
   */
  float getKomi() const;

  /**
   * ルールを取得する。
   * @return ルール
   */
  int32_t getRule() const;

  /**
   * スーパーコウルールを適用するならtrueを返す。
   * @return スーパーコウルールを適用するならtrue
   */
  bool getSuperko() const;

  /**
   * PolicyNetworkの評価値が最も高い候補手を取得する。
   * @return 候補手
   */
  Move getPolicyMove();

  /**
   * 子ノードの一覧を取得する。
   * @return 子ノードの一覧
   */
  std::vector<MctsNode*> getChildren();

  /**
   * 親ノードを取得する。
   * @return 親ノード
   */
  MctsNode* getParent();

  /**
   * 指定した着手に対応するノードを取得する。
   * @param move 着手
   * @return ノード
   */
  MctsNode* getChild(Move move);

  /**
   * 指定した着手に対応する子ノードを削除する。
   * @param move 着手
   */
  void removeChild(Move move);

  /**
   * このノードの探索回数を取得する。
   * @return 探索回数
   */
  int32_t getVisits();

  /**
   * プレイアウト数を取得する。
   * @return プレイアウト数
   */
  int32_t getPlayouts();

  /**
   * MCTS評価値を更新する。
   * @param value 評価値
   */
  void updateMctsValue(float value);

  /**
   * MCTS評価値を取得する。
   * @return MCTS評価値
   */
  float getMctsValue();

  /**
   * MCTS評価値の信頼区間の下限を取得する。
   * @return 信頼区間の下限
   */
  float getMctsValueLCB();

  /**
   * PUCBに基づいて優先度を取得する。
   * @param totalVisits 探索回数の合計
   * @return 優先度
   */
  float getPriorityByPUCB(int32_t totalVisits);

  /**
   * このノードの予想進行を取得する。
   * @return 予想進行
   */
  std::vector<Move> getVariations();

  /**
   * 領域の予測確率を取得する。
   * @return 領域の予測確率
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> getTerritories();

  /**
   * 盤面の状態を取得する。
   * @return 盤面の状態
   */
  std::vector<int32_t> getBoardState();

  /**
   * 盤面を取得する。
   * @return 盤面
   */
  inline const Board& getBoard() {
    return _board;
  }

  /**
   * 着手情報を取得する。
   * @return 着手情報
   */
  inline const Move& getMove() {
    return _move;
  }

  /**
   * 次に打つ石の色を取得する。
   * @return 次に打つ石の色
   */
  inline int32_t getNextColor() const {
    return OPPOSITE(_move.getColor());
  }

  /**
   * このノードで打ち上げた石の数を取得する。
   * @return 打ち上げた石の数
   */
  inline int32_t getCaptured() const {
    return _captured;
  }

  /**
   * このノードの予想着手確率を取得する。
   * @return 予想着手確率
   */
  inline float getProbability() const {
    return _probability;
  }

 private:
  /**
   * 同期用ミューテックス。
   */
  std::shared_mutex _mutex;

  /**
   * 評価完了待機用の条件変数。
   */
  std::condition_variable_any _condition;

  /**
   * ノード管理オブジェクト。
   */
  MctsManager* _manager;

  /**
   * このノードで評価する盤面。
   */
  Board _board;

  /**
   * 着手。
   */
  Move _move;

  /**
   * 打ち上げた石の数。
   */
  int32_t _captured;

  /**
   * 予想着手確率。
   */
  float _probability;

  /**
   * 最初に作成された子ノードならtrue。
   */
  bool _firstChild;

  /**
   * 評価中ならtrue。
   */
  bool _evaluating;

  /**
   * 評価済みならtrue。
   */
  bool _evaluated;

  /**
   * 盤面評価値。
   */
  float _nodeValue;

  /**
   * 次の着手確率の一覧。
   */
  std::vector<Policy> _policies;

  /**
   * 親ノード。
   */
  MctsNode* _parent;

  /**
   * 子ノードの一覧。
   */
  std::map<int32_t, MctsNode*> _children;

  /**
   * 探索回数。
   */
  int32_t _visits;

  /**
   * プレイアウト数。
   */
  std::atomic<int32_t> _playouts;

  /**
   * MCTS評価値。
   */
  MctsValue _mctsValue;

  /**
   * 領域の予測確率の一覧。
   */
  std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> _territories;

  /**
   * 子ノードへの登録を待機している候補手の一覧。
   */
  std::queue<Policy> _waitingPolicies;

  /**
   * 子ノードへの登録を待機している候補手のセット。
   */
  std::set<int32_t> _waitingMoves;

  /**
   * 盤面以外の状態を初期化する。
   */
  void _resetNode();

  /**
   * 次に評価するノードを取得する。
   * @param equally 探索回数を均等にする場合はtrue
   * @param width 探索幅
   * @param temperature 探索の温度パラメータ
   * @param noise ガンベルノイズの強さ
   * @return 次に評価するノード
   */
  MctsNode* _pickupNextNode(bool equally, int32_t width, float temperature, float noise);

  /**
   * 着手に対応するインデックスを取得する。
   * @param move 着手
   * @return インデックス
   */
  inline int32_t _getMoveIndex(Move move) const {
    return (move.getY() * _board.getWidth() + move.getX()) * 3 + move.getColor();
  }
};

}  // namespace deepgo
