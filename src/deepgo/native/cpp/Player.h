#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Candidate.h"
#include "Config.h"
#include "InferenceProcessor.h"
#include "MctsManager.h"
#include "MctsNode.h"
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
   * @param maxVisits 最大訪問数
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @param komi コミの目数
   * @param rule 勝敗の判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   * @param pucbConstantInit PUCBの信頼上限に掛ける定数の初期値
   * @param pucbConstantBase PUCBの信頼上限に掛ける定数の変化値
   */
  Player(
      InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
      int32_t width, int32_t height, float komi, int32_t rule, bool superko,
      float pucbConstantInit, float pucbConstantBase);

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
   * @param move 着手
   * @return 打ち上げた石の数
   */
  int32_t play(Move move);

  /**
   * パスの候補手を取得する。
   * @return パスの候補手
   */
  std::vector<Candidate> getPass();

  /**
   * 盤面評価を開始する。
   * @param equally 探索回数を均等にするならtrue
   * @param width 候補手の探索幅
   * @param temperature 探索の温度パラメータ
   * @param noise ガンベルノイズの強さ
   */
  void startEvaluation(bool equally, int32_t width, float temperature, float noise);

  /**
   * 指定された訪問数とプレイアウト数になるまで待機する。
   * @param visits 訪問数
   * @param playouts プレイアウト数
   * @param timelimit 時間制限
   * @param stop 探索を停止するならtrue
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

  /**
   * プレイヤオブジェクトの文字列表現を取得する。
   * @return プレイヤオブジェクトの文字列表現
   */
  std::string toString();

 private:
  /**
   * 同期オブジェクト。
   */
  std::mutex _mutex;

  /**
   * 探索条件変数。
   */
  std::condition_variable _searchCondition;

  /**
   * 更新条件変数。
   */
  std::condition_variable _updateCondition;

  /**
   * 停止待機条件変数。
   */
  std::condition_variable _stopCondition;

  /**
   * 推論を実行するオブジェクト。
   */
  InferenceProcessor* _processor;

  /**
   * スレッド管理オブジェクト。
   */
  ThreadPool _threadPool;

  /**
   * 探索管理スレッド。
   */
  std::thread _searchThread;

  /**
   * 更新管理スレッド。
   */
  std::thread _updateThread;

  /**
   * 探索ノードを管理するオブジェクト。
   */
  MctsManager _nodeManager;

  /**
   * ルートノード。
   */
  MctsNode* _root;

  /**
   * 最大訪問数。
   */
  int32_t _maxVisits;

  /**
   * 探索回数を均等にするならtrue。
   */
  bool _searchEqually;

  /**
   * 候補手の探索幅。
   */
  int32_t _searchCandidateWidth;

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
   * 評価待ちノード。
   */
  std::queue<MctsNode*> _evaluatingNodes;

  /**
   * 探索処理を起動する。
   */
  void _runSearch();

  /**
   * 探索木を展開する。
   */
  void _runExpand();

  /**
   * ノードの状態を更新する。
   */
  void _runUpdate();
};

}  // namespace deepgo
