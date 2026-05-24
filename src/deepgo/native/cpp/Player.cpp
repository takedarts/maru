#include "Player.h"

#include <iomanip>
#include <sstream>

namespace deepgo {

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
Player::Player(
    InferenceProcessor* processor, int32_t threads, int32_t maxVisits,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko,
    float pucbConstantInit, float pucbConstantBase)
    : _mutex(),
      _searchCondition(),
      _updateCondition(),
      _stopCondition(),
      _processor(processor),
      _threadPool(threads),
      _searchThread(),
      _updateThread(),
      _nodeManager(MctsParameter(
          width, height, komi, rule, superko, pucbConstantInit, pucbConstantBase)),
      _root(_nodeManager.createNode()),
      _maxVisits(maxVisits),
      _searchEqually(false),
      _searchCandidateWidth(0),
      _searchTemperature(1.0f),
      _searchNoise(0.0f),
      _runnings(0),
      _paused(false),
      _stopped(true),
      _terminated(false),
      _evaluatingNodes() {
  _root->initialize();
  _searchThread = std::thread(&Player::_runSearch, this);
  _updateThread = std::thread(&Player::_runUpdate, this);
}

/**
 * プレイヤオブジェクトを破棄する。
 */
Player::~Player() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    _terminated = true;
  }

  _searchCondition.notify_one();
  _updateCondition.notify_one();
  _searchThread.join();
  _updateThread.join();
}

/**
 * プレイヤオブジェクトの状態を初期化する。
 */
void Player::initialize() {
  std::unique_lock<std::mutex> lock(_mutex);

  // 探索スレッドを一時停止する
  _paused = true;
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // 現在の探索木を退避して、新しい初期局面のルートを作成する
  MctsNode* old_root = _root;

  _root = _nodeManager.createNode();
  _root->initialize();

  // 古い探索木はノードプールへ戻して再利用できる状態にする
  _nodeManager.releaseTree(old_root);

  // 探索スレッドを再開する
  _paused = false;
  _searchCondition.notify_one();
}

/**
 * 盤面に石を置く。
 * @param move 着手
 * @return 打ち上げた石の数
 */
int32_t Player::play(Move move) {
  std::unique_lock<std::mutex> lock(_mutex);

  // 探索スレッドを一時停止する
  _paused = true;
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // 着手先の子ノードを新しいルートにする
  MctsNode* old_root = _root;

  _root = old_root->getChild(move);
  _root->setAsRootNode();

  // 新しいルートを古い探索木から切り離して、それ以外を解放する
  old_root->removeChild(move);
  _nodeManager.releaseTree(old_root);

  // 探索スレッドを再開する
  _paused = false;
  _searchCondition.notify_one();

  return _root->getCaptured();
}

/**
 * パスの候補手を取得する。
 * @return パスの候補手
 */
std::vector<Candidate> Player::getPass() {
  std::unique_lock<std::mutex> lock(_mutex);

  // 探索スレッドを一時停止する
  _paused = true;
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // パスの候補手を作成する
  std::vector<Candidate> candidates;

  candidates.emplace_back(
      Move::createPassMove(_root->getNextColor()), 0, 0, 1.0f,
      _root->getMctsValue(), std::vector<Move>(), _root->getTerritories());

  // 探索スレッドを再開する
  _paused = false;
  _searchCondition.notify_one();

  return candidates;
}

/**
 * 盤面評価を開始する。
 * @param equally 探索回数を均等にするならtrue
 * @param width 候補手の探索幅
 * @param temperature 探索の温度パラメータ
 * @param noise ガンベルノイズの強さ
 */
void Player::startEvaluation(
    bool equally, int32_t width, float temperature, float noise) {
  std::unique_lock<std::mutex> lock(_mutex);

  // 探索条件をまとめて変更するため、実行中の探索をいったん止める
  _paused = true;
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // 以降の探索で使用する条件を更新する
  _searchEqually = equally;
  _searchCandidateWidth = width;
  _searchTemperature = temperature;
  _searchNoise = noise;

  // 探索スレッドを動作状態にする
  _stopped = false;

  // 探索スレッドを再開する
  _paused = false;
  _searchCondition.notify_one();
}

/**
 * 指定された訪問数とプレイアウト数になるまで待機する。
 * @param visits 訪問数
 * @param playouts プレイアウト数
 * @param timelimit 時間制限
 * @param stop 探索を停止するならtrue
 */
void Player::waitEvaluation(int32_t visits, int32_t playouts, float timelimit, bool stop) {
  std::unique_lock<std::mutex> lock(_mutex);

  // 最初の評価を待機する
  if (visits > 0 || playouts > 0) {
    _stopCondition.wait(lock, [this]() {
      return _root->getVisits() > 0;
    });
  }

  std::chrono::milliseconds timeout(static_cast<int32_t>(timelimit * 1000.0f));

  // 指定回数に到達するか、時間制限に達するまで待機する
  _stopCondition.wait_for(lock, timeout, [this, visits, playouts]() {
    return _root->getVisits() >= visits && _root->getPlayouts() >= playouts;
  });

  // 停止状態が要求されている場合は停止フラグを設定する
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
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // 候補手の一覧を作成する
  std::vector<Candidate> candidates;

  for (MctsNode* node : _root->getChildren()) {
    candidates.emplace_back(node);
  }

  // 候補手がない場合はPolicyNetworkによる着手を追加する
  if (candidates.empty()) {
    Move move = _root->getPolicyMove();

    if (!move.isPass()) {
      candidates.emplace_back(
          move, 0, 0, 1.0f, _root->getMctsValue(),
          std::vector<Move>(), _root->getTerritories());
    }
  }

  _paused = false;
  _searchCondition.notify_one();

  return candidates;
}

/**
 * 次の石の色を取得する。
 * @return 石の色
 */
int32_t Player::getColor() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _root->getNextColor();
}

/**
 * 盤面の状態を取得する。
 * @return 盤面の状態
 */
std::vector<int32_t> Player::getBoardState() {
  return _root->getBoardState();
}

/**
 * プレイヤオブジェクトの文字列表現を取得する。
 * @return プレイヤオブジェクトの文字列表現
 */
std::string Player::toString() {
  std::unique_lock<std::mutex> lock(_mutex);
  std::stringstream ss;

  // スレッドを一時停止する
  _paused = true;
  _stopCondition.wait(lock, [this]() {
    return _runnings == 0 && _evaluatingNodes.empty();
  });

  // 盤面の状態を文字列に変換する
  ss << "--- Board ---" << std::endl
     << _root->getBoard() << std::endl;

  // 探索木を深さ優先で辿りながら現在の状態を文字列に変換する
  std::vector<std::pair<MctsNode*, std::string>> stack = {{_root, ""}};

  while (!stack.empty()) {
    MctsNode* current = stack.back().first;
    std::string prefix = stack.back().second;
    stack.pop_back();

    ss << prefix
       << "Move=(" << current->getMove() << ")"
       << ", Visits=" << current->getVisits()
       << ", Playouts=" << current->getPlayouts()
       << ", Value=" << std::setprecision(4) << current->getMctsValue()
       << ", Policy=" << std::setprecision(4) << current->getProbability()
       << ", PUCB=" << std::setprecision(4) << current->getPriorityByPUCB(_root->getVisits())
       << std::endl;

    std::vector<MctsNode*> children = current->getChildren();

    // 深さ優先で出力し、子ノードはインデントで親子関係を表現する
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
      stack.emplace_back(*it, prefix + "  ");
    }
  }

  // スレッドを再開する
  _paused = false;
  _searchCondition.notify_one();

  return ss.str();
}

/**
 * 探索処理を起動する。
 */
void Player::_runSearch() {
  // 評価ノード数の最大数を計算する
  const int32_t max_evaluating_size =
      _processor->getBatchSize() * _processor->getThreadSize() * 10;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mutex);

      // 探索処理が実行可能になるまで待機する
      // 探索処理が実行可能になる条件は以下のいずれか
      // - [停止] 終了が要求されていて、実行中のスレッドがなくて、評価中のノードがない
      // - [手順探索] 終了が要求、探索が停止要求、一時停止要求のいずれもなくて、
      //   実行スレッド数がスレッドプールのスレッド数未満で、
      //   評価中のノードの数が最大評価ノード数未満で、探索回数が最大訪問回数未満
      _searchCondition.wait(lock, [this, max_evaluating_size]() {
        if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
          return true;
        } else if (
            !_terminated && !_stopped && !_paused &&
            _runnings < _threadPool.getSize() &&
            _evaluatingNodes.size() < static_cast<size_t>(max_evaluating_size) &&
            _root->getVisits() < _maxVisits) {
          return true;
        } else {
          return false;
        }
      });

      // 停止条件を満たしているならばループを抜ける
      if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
        break;
      }

      // そうでない場合は探索処理を実行する
      _runnings += 1;
    }

    // 探索木の展開処理をスレッドプールに登録する
    _threadPool.submit([this]() {
      _runExpand();

      {
        std::unique_lock<std::mutex> lock(_mutex);
        _runnings -= 1;
      }

      _searchCondition.notify_one();
      _updateCondition.notify_one();
      _stopCondition.notify_all();
    });
  }
}

/**
 * 探索木を展開する。
 */
void Player::_runExpand() {
  // 探索の設定をローカル変数にコピーする
  bool search_equally = _searchEqually;
  int32_t search_width = _searchCandidateWidth;
  float search_temperature = _searchTemperature;
  float search_noise = _searchNoise;

  // ルートノードから探索を開始する
  // 次に評価するノードを取得しながら探索木を辿る
  MctsNode* node = nullptr;
  MctsNode* next_node = _root;

  while (true) {
    // 次に評価するノードを取得する
    node = next_node;
    next_node = node->pickupNextNode(
        search_equally, search_width, search_temperature, search_noise);

    // 次に評価するノードが存在しない場合は探索を終了する
    if (next_node == nullptr) {
      break;
    }

    // 探索の設定を更新する
    search_equally = false;
    search_width = 0;
    search_temperature = 1.0f;
    search_noise = 0.0f;
  }

  // 未評価の場合
  if (!node->isEvaluated()) {
    // 盤面評価の推論モデルに評価対象としてノードを登録する
    _processor->submit(node, [this](MctsNode*) {
      std::unique_lock<std::mutex> lock(_mutex);
      _updateCondition.notify_one();
    });
  }

  // 評価中のノードの一覧にノードを追加する
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _evaluatingNodes.push(node);
    _updateCondition.notify_one();
  }
}

/**
 * ノードの状態を更新する。
 */
void Player::_runUpdate() {
  while (true) {
    std::vector<MctsNode*> finished_nodes;

    {
      std::unique_lock<std::mutex> lock(_mutex);

      // 更新処理が実行可能になるまで待機する
      // 更新処理が実行可能になる条件は以下のいずれか
      // - [停止] 終了が要求されていて、実行中のスレッドがなくて、評価中のノードがない。
      // - [評価] 評価中のノードがあって、そのノードの評価が完了している
      _updateCondition.wait(lock, [this]() {
        if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
          return true;
        } else if (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
          return true;
        } else {
          return false;
        }
      });

      // 停止条件を満たしているならばループを抜ける
      if (_terminated && _runnings == 0 && _evaluatingNodes.empty()) {
        break;
      }

      // 評価済みのノードを取り出す
      while (!_evaluatingNodes.empty() && _evaluatingNodes.front()->isEvaluated()) {
        finished_nodes.push_back(_evaluatingNodes.front());
        _evaluatingNodes.pop();
      }
    }

    // 評価済みのノードの統計情報を更新する
    // 詰み手順が見つかっているノードで評価値をNodeValueに設定する
    for (MctsNode* node : finished_nodes) {
      float mcts_value = node->getNodeValue();
      MctsNode* current_node = node;

      while (current_node != nullptr) {
        current_node->updateMctsValue(mcts_value);
        current_node = current_node->getParent();
      }
    }

    // 探索処理に通知する
    _searchCondition.notify_one();
    _stopCondition.notify_all();
  }
}

}  // namespace deepgo
