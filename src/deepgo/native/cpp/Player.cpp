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
 * @param superko スーパーコウルールを適用するならtrue
 * @param evalLeafOnly 葉ノードのみを評価対象とするならtrue
 */
Player::Player(
    Processor* processor, int32_t threads,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    bool evalLeafOnly)
    : _mutex(),
      _condition(),
      _nodeManager(processor, width, height, komi, rule, superko),
      _threadPool(threads),
      _thread(),
      _root(_nodeManager.createNode()),
      _evalLeafOnly(evalLeafOnly),
      _searchVisits(0),
      _searchPlayouts(0),
      _searchEqually(false),
      _searchUseUcb1(false),
      _searchWidth(0),
      _searchTemperature(1.0f),
      _searchNoise(0.0f),
      _runnings(0),
      _paused(false),
      _stopped(true),
      _terminated(false) {
  _thread.reset(new std::thread([this]() { this->_run(); }));
  _root->initialize();
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
void Player::initialize() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 現在のルートノードを保存する
  Node* old_root = _root;

  // 初期ノードをルートノードに設定する
  _root = _nodeManager.createNode();
  _root->initialize();

  // ルートノード以外のノードを解放する
  _releaseNode(old_root);

  // スレッドを再開する
  _paused = false;
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
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 現在のルートノードを保存する
  Node* old_root = _root;

  // 新しいルートノードを設定する
  _root = old_root->getChild(x, y);

  // ルートノード以外のノードを解放する
  _releaseNode(old_root);

  // スレッドを再開する
  _paused = false;
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
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 候補手を作成する
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      -1, -1, OPPOSITE(_root->getColor()),
      0, 0, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // スレッドを再開する
  _paused = false;
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
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 候補手を作成する
  std::pair<int32_t, int32_t> move = _root->getRandomMove(temperature);
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      move.first, move.second, OPPOSITE(_root->getColor()),
      1, 1, 1.0f, _root->getValue(),
      std::vector<std::pair<int32_t, int32_t>>());

  // スレッドを再開する
  _paused = false;
  _condition.notify_all();

  return candidates;
}

/**
 * 盤面評価を開始する。
 * 探索処理は別スレッドで実行される。
 * @param equally 探索回数を均等にするならばtrue、UCB1かPUCBを使用するならばfalse
 * @param useUcb1 探索先の基準としてUCB1を使用するならばtrue、PUCBを使用するならばfalse
 * @param width 探索幅(0の場合は探索幅を自動で調整する)
 * @param temperature 探索の温度パラメータ
 * @param noise 探索のガンベルノイズの強さ
 */
void Player::startEvaluation(
    bool equally, bool useUcb1, int32_t width, float temperature, float noise) {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // 探索の設定を変更する
  _searchVisits = _root->getVisits();
  _searchPlayouts = _root->getPlayouts();
  _searchEqually = equally;
  _searchUseUcb1 = useUcb1;
  _searchWidth = width;
  _searchTemperature = temperature;
  _searchNoise = noise;

  // 実行状態に設定する
  _stopped = false;

  // スレッドを再開する
  _paused = false;
  _condition.notify_all();
}

/**
 * 指定された訪問数とプレイアウト数になるまで待機する。
 * @param visits 訪問数
 * @param playouts プレイアウト数
 * @param timelimit 時間制限
 * @param stop 探索を停止するならばtrue
 */
void Player::waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop) {
  std::unique_lock<std::mutex> lock(_mutex);

  // 指定された訪問数とプレイアウト数になるまで待機する
  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));
  _condition.wait_for(lock, timeout, [this, visits, playouts]() {
    return _searchVisits >= visits && _searchPlayouts >= playouts;
  });

  // 探索を停止する
  _stopped = _stopped || stop;
}

/**
 * 候補手の一覧を取得する。
 * @return 候補手の一覧
 */
std::vector<Candidate> Player::getCandidates() {
  std::unique_lock<std::mutex> lock(_mutex);

  // スレッドを一時停止する
  _paused = true;
  _condition.wait(lock, [this]() { return _runnings == 0; });

  // ルートノードの子ノードを候補手として設定する
  std::vector<Candidate> candidates;

  for (Node* node : _root->getChildren()) {
    candidates.emplace_back(
        node->getX(), node->getY(), node->getColor(),
        node->getVisits(), node->getPlayouts(),
        node->getPolicy(), node->getValue(), node->getVariations());
  }

  // 候補手がない場合はPolicyNetworkによる着手を追加する
  if (candidates.empty()) {
    std::pair<int32_t, int32_t> move = _root->getPolicyMove();

    candidates.emplace_back(
        move.first, move.second, OPPOSITE(_root->getColor()),
        1, 1, 1.0f, _root->getValue(),
        std::vector<std::pair<int32_t, int32_t>>());
  }

  // スレッドを再開する
  _paused = false;
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
        } else if (!_stopped && !_paused && _runnings < _threadPool.getSize()) {
          return true;
        } else {
          return false;
        }
      });

      if (_terminated) {
        break;
      }

      _searchVisits += 1;
      _runnings += 1;
      _condition.notify_all();
    }

    _threadPool.submit([this]() {
      int32_t playouts = _evaluate();
      std::unique_lock<std::mutex> lock(_mutex);
      _searchPlayouts += playouts;
      _runnings -= 1;
      _condition.notify_all();
    });
  }
}

/**
 * 探索を実行する。
 * @return 探索のプレイアウト数
 */
int32_t Player::_evaluate() {
  std::vector<Node*> nodes = {_root};
  bool search_equally = _searchEqually;
  int32_t search_width = _searchWidth;
  bool search_use_ucb1 = _searchUseUcb1;
  float search_temperature = _searchTemperature;
  float search_noise = _searchNoise;
  int32_t playouts = 0;

  while (true) {
    NodeResult result = nodes.back()->evaluate(
        search_equally, search_width, search_use_ucb1, search_temperature, search_noise);

    // ノードの評価値を更新する
    if (result.getPlayouts() == 1) {
      for (Node* node : nodes) {
        node->updateValue(result.getValue());
      }
    } else if (result.getPlayouts() == -1 && _evalLeafOnly) {
      for (Node* node : nodes) {
        node->cancelValue(result.getValue());
      }
    }

    // ノードのプレイアウト数を更新する
    for (Node* node : nodes) {
      node->setPlayouts(node->getPlayouts() + result.getPlayouts());
    }

    // この探索のプレイアウト数を更新する
    playouts += result.getPlayouts();

    // 子ノードが存在する場合は次のノードとして設定する
    if (result.getNode() != nullptr) {
      nodes.push_back(result.getNode());
    } else {
      break;
    }

    // ルートノードだけに適用する設定項目をリセットする
    search_equally = 0;
    search_use_ucb1 = false;
    search_width = 0;
    search_temperature = 1.0f;
    search_noise = 0.0f;
  }

  // プレイアウト数を返す
  return playouts;
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
