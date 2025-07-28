#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "Board.h"
#include "Candidate.h"
#include "Config.h"
#include "Node.h"
#include "NodeManager.h"
#include "Processor.h"
#include "ThreadPool.h"

namespace deepgo {

/**
 * ゲームを進行するプレイヤを表すクラス。
 */
class Player {
 public:
  /**
   * プレイヤオブジェクトを作成する。
   * @param processor 推論を実行するオブジェクト
   * @param threads スレッドの数
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @param komi コミの目数
   * @param rule 勝敗の判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   * @param evalLeafOnly 葉ノードのみを評価対象とするならtrue
   */
  Player(
      Processor* processor, int32_t threads,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      bool evalLeafOnly);

  /**
   * プレイヤオブジェクトを破棄する。
   */
  virtual ~Player();

  /**
   * プレイヤオブジェクトの状態を初期化する。
   */
  void initialize();

  /**
   * 盤面に石を置く。
   * @param x 石を置く位置のx座標
   * @param y 石を置く位置のy座標
   * @return 打ち上げた石の数
   */
  int32_t play(int32_t x, int32_t y);

  /**
   * パスの候補手を取得する。
   * @return パスの候補手
   */
  std::vector<Candidate> getPass();

  /**
   * ランダムに次の候補手を選ぶ。
   * @param temperature 温度（大きいほどランダム性が高くなる）
   * @return ランダムに選ばれた候補手
   */
  std::vector<Candidate> getRandom(float temperature = 0.0);

  /**
   * 盤面評価を開始する。
   * 探索処理は別スレッドで実行される。
   * @param equally 探索回数を均等にするならばtrue、UCB1かPUCBを使用するならばfalse
   * @param useUcb1 探索先の基準としてUCB1を使用するならばtrue、PUCBを使用するならばfalse
   * @param width 探索幅(0の場合は探索幅を自動で調整する)
   * @param temperature 探索の温度パラメータ
   * @param noise 探索のガンベルノイズの強さ
   */
  void startEvaluation(
      bool equally, bool useUcb1, int32_t width, float temperature, float noise);

  /**
   * 指定された訪問数とプレイアウト数になるまで待機する。
   * @param visits 訪問数
   * @param playouts プレイアウト数
   * @param timelimit 時間制限
   * @param stop 探索を停止するならばtrue
   */
  void waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop);

  /**
   * 候補手の一覧を取得する。
   * @return 候補手の一覧
   */
  std::vector<Candidate> getCandidates();

  /**
   * 次の石の色を取得する。
   * @return 石の色
   */
  int32_t getColor();

  /**
   * 盤面の状態を取得する。
   * @return 盤面の状態
   */
  std::vector<int32_t> getBoardState();

 private:
  /**
   * 同期オブジェクト。
   */
  std::mutex _mutex;

  /**
   * 条件変数。
   */
  std::condition_variable _condition;

  /**
   * 探索ノードを管理するオブジェクト。
   */
  NodeManager _nodeManager;

  /**
   * スレッド管理オブジェクト。
   */
  ThreadPool _threadPool;

  /**
   * 探索を実行するスレッド。
   */
  std::unique_ptr<std::thread> _thread;

  /**
   * ルートノード。
   */
  Node* _root;

  /**
   * 葉ノードのみを評価対象とするならtrue。
   */
  bool _evalLeafOnly;

  /**
   * 実行する訪問数。
   */
  int32_t _searchVisits;

  /**
   * 実行するプレイアウト数。
   */
  int32_t _searchPlayouts;

  /**
   * 探索回数を均等にするならtrue。
   */
  bool _searchEqually;

  /**
   * 探索先の基準としてUCB1を使用するならtrue。
   */
  bool _searchUseUcb1;

  /**
   * 探索幅。
   */
  int32_t _searchWidth;

  /**
   * 探索の温度パラメータ。
   */
  float _searchTemperature;

  /**
   * 探索のガンベルノイズの強さ。
   */
  float _searchNoise;

  /**
   * 実行中のスレッド数。
   */
  int32_t _runnings;

  /**
   * 探索を一時停止しているならtrue。
   */
  bool _paused;

  /**
   * 探索を停止しているならtrue。
   */
  bool _stopped;

  /**
   * 探索を終了しているならtrue。
   */
  bool _terminated;

  /**
   * 探索処理を起動する。
   */
  void _run();

  /**
   * 探索を実行する。
   * @return 探索のプレイアウト数
   */
  int32_t _evaluate();

  /**
   * ルートノード以外のノードオブジェクトを返却する。
   * @param node 返却するノードオブジェクト
   */
  void _releaseNode(Node* node);
};

}  // namespace deepgo
