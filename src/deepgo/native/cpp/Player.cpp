#include "Player.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <unordered_map>

namespace deepgo {

/**
 * プレイヤオブジェクトを生成する。
 * @param processor 盤面を評価するオブジェクト
 * @param threads 盤面探索のスレッド数
 * @param width 盤面の幅
 * @param height 盤面の高さ
 * @param komi コミの目数
 * @param rule ゲームルール
 * @param superko スーパーコウの有無
 */
Player::Player(
    Processor* processor, int32_t threads,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko)
    : _mutex(),
      _condition(),
      _nodeManager(processor, width, height, komi, rule, superko),
      _threadPool(threads),
      _thread(),
      _root(_nodeManager.createInitNode()),
      _searchVisits(0),
      _searchEqually(false),
      _searchUseUcb1(false),
      _searchWidth(0),
      _runnings(0),
      _pause(false),
      _terminated(false) {
  _thread.reset(new std::thread([this]() { this->_run(); }));
}

/**
 * プレイヤオブジェクトを破棄する。
 */
Player::~Player() {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }

  _thread->join();
}

/**
 * プレイヤオブジェクトの状態を初期化する。
 */
void Player::clear() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 現在のルートノードを保存する
  Node* old_root = _root;

  // 初期ノードをルートノードに設定する
  _root = _nodeManager.createInitNode();

  // ルートノード以外のノードを解放する
  _releaseNode(old_root);

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();
}

/**
 * 盤面に石を置く。
 * @param x 石を置く位置のx座標
 * @param y 石を置く位置のy座標
 */
int32_t Player::play(int32_t x, int32_t y) {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 現在のルートノードを保存する
  Node* old_root = _root;

  // 新しいルートノードを設定する
  _root = old_root->getChild(x, y);

  // ルートノード以外のノードを解放する
  _releaseNode(old_root);

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();

  // 打ち上げた石の数を返す
  return _root->getCaptured();
}

/**
 * パスの候補手を取得する。
 * @return パスの候補手
 */
std::vector<Candidate> Player::getPass() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 候補手を作成する
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      -1, -1, OPPOSITE(_root->getColor()),
      0, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();

  return candidates;
}

/**
 * ランダムに次の候補手を選ぶ。
 * @param temperature 温度（大きいほどランダム性が高くなる）
 * @return ランダムに選ばれた候補手
 */
std::vector<Candidate> Player::getRandom(float temperature) {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 候補手を作成する
  std::pair<int32_t, int32_t> move = _root->getRandomMove(temperature);
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      move.first, move.second, OPPOSITE(_root->getColor()),
      0, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();

  return candidates;
}

/**
 * 盤面評価を開始する。
 * 探索処理は別スレッドで実行される。
 * 最大訪問回数に0以下の値を指定すると停止命令を指示するまで探索を続ける。
 * @param visits 最大訪問回数
 * @param equally 探索回数を均等にするならばtrue、UCB1かPUCBを使用するならばfalse
 * @param useUcb1 探索先の基準としてUCB1を使用するならばtrue、PUCBを使用するならばfalse
 * @param width 探索幅(0の場合は探索幅を自動で調整する)
 */
void Player::startEvaluation(int32_t visits, bool equally, bool useUcb1, int32_t width) {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 探索の設定を変更する
  _searchVisits = std::max(visits - _root->getVisits(), 0);
  _searchEqually = equally;
  _searchUseUcb1 = useUcb1;
  _searchWidth = width;

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();
}

/**
 * 盤面評価を停止する。
 */
void Player::stopEvaluation() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドの探索処理をキャンセルする
  _searchVisits = 0;

  // スレッドの探索処理が終了するまで待機する
  _condition.wait(lock, [this]() { return _runnings == 0; });
}

/**
 * 設定された盤面評価処理が終了するまで待機する。
 * @param timelimit 時間制限
 */
void Player::waitEvaluation(float timelimit) {
  std::unique_lock<std::mutex> lock(_mutex);
  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));
  _condition.wait_for(lock, timeout, [this]() {
    return _runnings == 0 && _searchVisits == 0;
  });
}

/**
 * 候補手の一覧を取得する。
 * @return 候補手の一覧
 */
std::vector<Candidate> Player::getCandidates() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _pause = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // ルートノードの子ノードを候補手として設定する
  std::vector<Candidate> candidates;

  for (Node* node : _root->getChildren()) {
    candidates.emplace_back(
        node->getX(), node->getY(), node->getColor(),
        node->getVisits(), node->getPolicy(), node->getValue(),
        node->getVariations());
  }

  // 候補手がない場合はPolicyNetworkによる着手を追加する
  if (candidates.empty()) {
    std::pair<int32_t, int32_t> move = _root->getPolicyMove();

    candidates.emplace_back(
        move.first, move.second, OPPOSITE(_root->getColor()),
        0, 1.0f, _root->getValue(),
        std::vector<std::pair<int32_t, int32_t>>());
  }

  // スレッドを再開する
  _pause = false;
  _condition.notify_all();

  return candidates;
}

/**
 * 次の石の色を取得する。
 * @return 石の色
 */
int32_t Player::getColor() {
  return OPPOSITE(_root->getColor());
}

/**
 * 盤面の状態を取得する。
 * @return 盤面の状態
 */
std::vector<int32_t> Player::getBoardState() {
  return _root->getBoardState();
}

/**
 * 探索処理を起動する。
 */
void Player::_run() {
  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _condition.wait(lock, [this]() {
        if (_terminated) {
          return true;
        } else if (!_pause && _searchVisits > 0 && _runnings < _threadPool.getSize()) {
          return true;
        } else {
          return false;
        }
      });

      if (_terminated) {
        break;
      }

      _runnings += 1;
      _searchVisits -= 1;
    }

    _threadPool.submit([this]() {
      _evaluate();
      std::unique_lock<std::mutex> lock(_mutex);
      _runnings -= 1;
      _condition.notify_all();
    });
  }
}

/**
 * 探索を実行する。
 */
void Player::_evaluate() {
  std::vector<Node*> nodes = {_root};
  float value = 0.0f;
  bool search_equally = _searchEqually;
  int32_t search_width = _searchWidth;
  bool search_use_ucb1 = _searchUseUcb1;

  while (true) {
    NodeResult result = nodes.back()->evaluate(
        search_equally, search_width, search_use_ucb1);

    if (result.getNode() != nullptr) {
      nodes.push_back(result.getNode());
    } else {
      value = result.getValue();
      break;
    }

    search_equally = 0;
    search_use_ucb1 = false;
    search_width = 0;
  }

  for (Node* node : nodes) {
    node->update(value);
  }
}

/**
 * ルートノード以外のノードオブジェクトを返却する。
 * @param node 返却するノードオブジェクト
 */
void Player::_releaseNode(Node* node) {
  std::vector<Node*> stack = {node};

  while (!stack.empty()) {
    Node* current = stack.back();
    stack.pop_back();

    if (current == _root) {
      continue;
    }

    for (Node* child : current->getChildren()) {
      stack.push_back(child);
    }

    _nodeManager.releaseNode(current);
  }
}

}  // namespace deepgo
