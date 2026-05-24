#include "MctsNode.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "MctsManager.h"

namespace deepgo {

// 探索時のPolicyサンプリングとガンベルノイズで使用する
// スレッドローカル乱数生成器
thread_local static std::random_device random_seed_gen;
thread_local static std::default_random_engine random_engine(random_seed_gen());

/**
 * 探索ノードオブジェクトを作成する。
 * @param manager ノード管理オブジェクト
 */
MctsNode::MctsNode(MctsManager* manager)
    : _mutex(),
      _condition(),
      _manager(manager),
      _board(
          manager->getParameter().getWidth(),
          manager->getParameter().getHeight()),
      _move(Move::createPassMove(WHITE)),
      _captured(0),
      _probability(0.0f),
      _firstChild(false),
      _evaluating(false),
      _evaluated(false),
      _nodeValue(0.0f),
      _policies(),
      _parent(nullptr),
      _children(),
      _visits(0),
      _playouts(0),
      _mctsValue(),
      _waitingPolicies(),
      _waitingMoves() {
}

/**
 * 初期盤面ノードとして設定する。
 */
void MctsNode::initialize() {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _resetNode();
  _board.clear();
  _move = Move::createPassMove(WHITE);
  _captured = 0;
}

/**
 * 初期盤面ノードとして設定する。
 * @param board 盤面
 * @param x 着手座標のX座標
 * @param y 着手座標のY座標
 * @param previousColor 直前に打った石の色
 * @param captured 打ち上げた石の数
 */
void MctsNode::initialize(
    const Board* board, int x, int y, int32_t previousColor, int32_t captured) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _resetNode();
  _board.copyFrom(board);
  _move = Move(x, y, previousColor);
  _captured = captured;
}

/**
 * 推論結果を適用する。
 * @param result 推論結果
 */
void MctsNode::applyInferenceResult(const InferenceResult& result) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // 盤面評価値を更新する
  _nodeValue = result.getValue();

  // 予想着手確率の一覧を更新する
  _policies = result.getPolicies();

  // 予測領域の確率を更新する
  _territories = result.getTerritories();

  // 評価済みとする
  _evaluating = false;
  _evaluated = true;

  // 評価の完了を待機しているスレッドに通知する
  _condition.notify_all();
}

/**
 * 次に評価するノードを取得する。
 * @param equally 探索回数を均等にする場合はtrue
 * @param width 探索幅
 * @param temperature 探索の温度パラメータ
 * @param noise ガンベルノイズの強さ
 * @return 次に評価するノード
 */
MctsNode* MctsNode::pickupNextNode(bool equally, int32_t width, float temperature, float noise) {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  // このノードに到達した探索回数を増やす
  _visits += 1;

  // このノードが評価中ならば、評価の完了を待機する
  if (_evaluating) {
    _condition.wait(lock, [this] { return !_evaluating; });
  }

  // すでに評価済みの場合は次に評価するノードを返す
  if (_evaluated) {
    // 次に評価するノードが存在する場合は次に評価するノードを返す
    // プレイアウト数は末端ノードによって増やされる
    if (!_policies.empty()) {
      return _pickupNextNode(equally, width, temperature, noise);
    }
    // そうでない場合はプレイアウト数だけ増やしてこのノードを返す
    else {
      MctsNode* current_node = this;

      while (current_node != nullptr) {
        current_node->_playouts.fetch_add(1, std::memory_order_relaxed);
        current_node = current_node->_parent;
      }

      return nullptr;
    }
  }

  // 未評価ノードに到達したのでプレイアウト数を増やす
  _playouts.fetch_add(1, std::memory_order_relaxed);

  if (!_firstChild) {
    MctsNode* parent = _parent;

    while (parent) {
      parent->_playouts.fetch_add(1, std::memory_order_relaxed);
      parent = parent->_parent;
    }
  }

  // このノードの状態を評価中にする
  _evaluating = true;

  return nullptr;
}

/**
 * ルートノードとして設定する。
 */
void MctsNode::setAsRootNode() {
  std::unique_lock<std::shared_mutex> lock(_mutex);

  _parent = nullptr;
  _probability = 1.0f;
}

/**
 * 評価済みならtrueを返す。
 * @return 評価済みならtrue
 */
bool MctsNode::isEvaluated() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _evaluated;
}

/**
 * 盤面評価値を取得する。
 * @return 盤面評価値
 */
float MctsNode::getNodeValue() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _nodeValue;
}

/**
 * 直前に打った石の色を設定する。
 * @param color 直前に打った石の色
 */
void MctsNode::setPreviousColor(int32_t color) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _move = Move(_move.getX(), _move.getY(), color);
}

/**
 * コミを取得する。
 * @return コミ
 */
float MctsNode::getKomi() const {
  return _manager->getParameter().getKomi();
}

/**
 * ルールを取得する。
 * @return ルール
 */
int32_t MctsNode::getRule() const {
  return _manager->getParameter().getRule();
}

/**
 * スーパーコウルールを適用するならtrueを返す。
 * @return スーパーコウルールを適用するならtrue
 */
bool MctsNode::getSuperko() const {
  return _manager->getParameter().getSuperko();
}

/**
 * PolicyNetworkの評価値が最も高い候補手を取得する。
 * @return 候補手
 */
Move MctsNode::getPolicyMove() {
  std::shared_lock<std::shared_mutex> lock(_mutex);

  // 候補手がない場合はパスを返す
  if (_policies.empty()) {
    return Move::createPassMove(getNextColor());
  }

  // 最も着手確率が高い候補手を取得する
  Policy max_policy = _policies[0];

  for (const Policy& policy : _policies) {
    if (max_policy.getProbability() < policy.getProbability()) {
      max_policy = policy;
    }
  }

  // 最も着手確率が高い候補手を返す
  return max_policy.getMove();
}

/**
 * 子ノードの一覧を取得する。
 * @return 子ノードの一覧
 */
std::vector<MctsNode*> MctsNode::getChildren() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  std::vector<MctsNode*> children;

  for (const auto& item : _children) {
    children.push_back(item.second);
  }

  return children;
}

/**
 * 親ノードを取得する。
 * @return 親ノード
 */
MctsNode* MctsNode::getParent() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _parent;
}

/**
 * 指定した着手に対応するノードを取得する。
 * @param move 着手
 * @return ノード
 */
MctsNode* MctsNode::getChild(Move move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  int32_t index = _getMoveIndex(move);

  // 子ノードが存在する場合はそのノードを返す
  if (_children.find(index) != _children.end()) {
    return _children[index];
  }

  // 子ノードが存在しない場合は新しくノードオブジェクトを作成して返す
  // 作成したノードオブジェクトはこのノードオブジェクトの子ノードとしては登録しない
  MctsNode* node = _manager->createNode();

  node->_resetNode();
  node->_board.copyFrom(&_board);
  node->_captured = std::max(node->_board.play(move), 0);
  node->_move = move;
  node->_probability = 0.0f;

  return node;
}

/**
 * 指定した着手に対応する子ノードを削除する。
 * @param move 着手
 */
void MctsNode::removeChild(Move move) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  int32_t index = _getMoveIndex(move);
  _children.erase(index);
}

/**
 * このノードの探索回数を取得する。
 * @return 探索回数
 */
int32_t MctsNode::getVisits() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _visits;
}

/**
 * プレイアウト数を取得する。
 * @return プレイアウト数
 */
int32_t MctsNode::getPlayouts() {
  return _playouts.load(std::memory_order_relaxed);
}

/**
 * MCTS評価値を更新する。
 * @param value 評価値
 */
void MctsNode::updateMctsValue(float value) {
  _mctsValue.update(value);
}

/**
 * MCTS評価値を取得する。
 * @return MCTS評価値
 */
float MctsNode::getMctsValue() {
  return _mctsValue.getValue(_nodeValue);
}

/**
 * MCTS評価値の信頼区間の下限を取得する。
 * @return 信頼区間の下限
 */
float MctsNode::getMctsValueLCB() {
  return _mctsValue.getValueLCB(_move.getColor(), _nodeValue);
}

/**
 * PUCBに基づいて優先度を取得する。
 * @param totalVisits 探索回数の合計
 * @return 優先度
 */
float MctsNode::getPriorityByPUCB(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  float pucb_constant_base = _manager->getParameter().getPucbConstantBase();
  float pucb_constant_init = _manager->getParameter().getPucbConstantInit();

  // 手番側から見た評価値に、Policy由来の探索ボーナスを足す
  float value = _mctsValue.getValue(_nodeValue) * _move.getColor();
  float c_pucb_inc = std::log((1 + totalVisits + pucb_constant_base) / pucb_constant_base);
  float c_pucb = pucb_constant_init * (1.0f + c_pucb_inc);
  float ucb = _probability * std::sqrt(static_cast<float>(totalVisits)) / (1 + _visits);

  return value + c_pucb * ucb;
}

/**
 * このノードの予想進行を取得する。
 * @return 予想進行
 */
std::vector<Move> MctsNode::getVariations() {
  std::vector<Move> variations;
  MctsNode* max_child = nullptr;

  {
    // 同期用ミューテックスをロックする
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // このノードの着手を予想進行の先頭に追加する
    variations.push_back(_move);

    // 評価値のLCBが最大の子ノードを辿る手順で予想進行を作成する
    float max_lcb = -std::numeric_limits<float>::infinity();

    for (auto child : _children) {
      float child_lcb = child.second->getMctsValueLCB() * getNextColor();

      if (child_lcb > max_lcb) {
        max_lcb = child_lcb;
        max_child = child.second;
      }
    }
  }

  // 子ノードの予想進行を予想進行の末尾に追加する
  if (max_child != nullptr) {
    std::vector<Move> child_variations = max_child->getVariations();
    variations.insert(variations.end(), child_variations.begin(), child_variations.end());
  }

  return variations;
}

/**
 * 領域の予測確率を取得する。
 * @return 領域の予測確率
 */
std::array<float, 3 * MODEL_SIZE * MODEL_SIZE> MctsNode::getTerritories() {
  MctsNode* max_child = nullptr;

  {
    // 同期用ミューテックスをロックする
    std::shared_lock<std::shared_mutex> lock(_mutex);

    // 子ノードが存在しない場合はこのノードの予測領域確率を返す
    if (_children.empty()) {
      const int32_t width = _board.getWidth();
      const int32_t height = _board.getHeight();
      std::vector<int32_t> board_territory(width * height);

      _board.getTerritories(board_territory.data(), BLACK);

      const int32_t x_begin = (MODEL_SIZE - width) / 2;
      const int32_t y_begin = (MODEL_SIZE - height) / 2;

      for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
          int32_t board_index = y * width + x;
          int32_t model_index = (y_begin + y) * MODEL_SIZE + (x_begin + x);

          if (board_territory[board_index] == BLACK) {
            _territories[model_index + 0 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 1 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 2 * MODEL_SIZE * MODEL_SIZE] = 1.0f;
          } else if (board_territory[board_index] == WHITE) {
            _territories[model_index + 0 * MODEL_SIZE * MODEL_SIZE] = 1.0f;
            _territories[model_index + 1 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
            _territories[model_index + 2 * MODEL_SIZE * MODEL_SIZE] = 0.0f;
          }
        }
      }

      return _territories;
    }

    // 評価値のLCBが最大の子ノードを辿る手順で予測領域確率を取得する
    float max_lcb = -std::numeric_limits<float>::infinity();

    for (auto child : _children) {
      float child_lcb = child.second->getMctsValueLCB() * getNextColor();

      if (child_lcb > max_lcb) {
        max_lcb = child_lcb;
        max_child = child.second;
      }
    }
  }

  // 子ノードの予測領域確率を返す
  return max_child->getTerritories();
}

/**
 * 盤面の状態を取得する。
 * @return 盤面の状態
 */
std::vector<int32_t> MctsNode::getBoardState() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _board.getState();
}

/**
 * 盤面以外の状態を初期化する。
 */
void MctsNode::_resetNode() {
  _probability = 0.0f;
  _firstChild = false;
  _evaluating = false;
  _evaluated = false;
  _nodeValue = 0.0f;
  _policies.clear();
  _parent = nullptr;
  _children.clear();
  _visits = 0;
  _playouts.store(0, std::memory_order_relaxed);
  _mctsValue.reset();
  _waitingPolicies = std::queue<Policy>();
  _waitingMoves.clear();
}

/**
 * 次に評価するノードを取得する。
 * @param equally 探索回数を均等にする場合はtrue
 * @param width 探索幅
 * @param temperature 探索の温度パラメータ
 * @param noise ガンベルノイズの強さ
 * @return 次に評価するノード
 */
MctsNode* MctsNode::_pickupNextNode(bool equally, int32_t width, float temperature, float noise) {
  // ルートノードであり、日本ルールであり、子ノードが1個以上であり、
  // パスの子ノードが存在せず、パスの候補手が待機リストに存在しない場合、
  // パスの候補手を着手確率0として待機リストに追加する
  Move pass_move = Move::createPassMove(getNextColor());
  int32_t pass_move_index = _getMoveIndex(pass_move);

  if (_parent == nullptr && _manager->getParameter().getRule() == RULE_JP &&
      !_children.empty() && _children.find(pass_move_index) == _children.end() &&
      _waitingMoves.find(pass_move_index) == _waitingMoves.end()) {
    Policy pass_policy(pass_move, 0.0f, 0);

    _waitingPolicies.push(pass_policy);
    _waitingMoves.insert(pass_move_index);
  }

  // Policyの候補が残っていて探索幅に余裕がある場合は、新しい着手を展開候補にする
  int32_t children_size = static_cast<int32_t>(_children.size() + _waitingMoves.size());

  if (children_size < static_cast<int32_t>(_policies.size()) &&
      (width < 1 || children_size < width)) {
    int32_t max_index = 0;
    int32_t max_priority_type = 0;
    float max_priority = 0.0f;

    // 温度パラメータを計算する
    float win_chance = _mctsValue.getValue(_nodeValue) * getNextColor() * 0.5f + 0.5f;
    float temperature_power =
        win_chance + (1.0f / std::max(temperature, 1e-3f)) * (1 - win_chance);

    // ガンベルノイズの生成オブジェクトを作成する
    // 子ノードの数が4以下の場合はノイズを加えない
    float noise_scale = (children_size <= 4) ? 0.0f : noise;
    std::extreme_value_distribution<float> noise_dist(0.0f, noise_scale);

    // 予測確率、温度、ガンベルノイズ、未展開優先の条件から次の候補を選ぶ
    for (int32_t i = 0; i < static_cast<int32_t>(_policies.size()); i++) {
      Policy& policy = _policies[i];
      float probability = policy.getProbability();

      // 温度パラメータを反映させる
      probability = std::pow(probability, temperature_power);

      // ガンベルノイズを加える
      // ノイズを加算する対象はロジットとなるため、確率に対してはe^noiseを乗算する
      probability *= std::exp(noise_dist(random_engine));

      // 優先度を計算する
      int32_t priority_type = 1;
      float priority = probability / (policy.getVisits() + 1);

      // 探索回数を均等にする設定となっている場合は登録済みの候補手の優先度を下げる
      if (equally) {
        int32_t policy_index = _getMoveIndex(policy.getMove());

        if (_children.find(policy_index) != _children.end() ||
            _waitingMoves.find(policy_index) != _waitingMoves.end()) {
          priority_type = 0;
        }
      }

      // 優先度の高い候補手を残す
      if (priority_type > max_priority_type ||
          (priority_type == max_priority_type && priority > max_priority)) {
        max_index = i;
        max_priority_type = priority_type;
        max_priority = priority;
      }
    }

    // 評価に追加する候補手が未登録状態であれば新たに待機リストに登録する
    Policy& max_policy = _policies[max_index];
    int32_t max_policy_index = _getMoveIndex(max_policy.getMove());

    if (_children.find(max_policy_index) == _children.end() &&
        _waitingMoves.find(max_policy_index) == _waitingMoves.end()) {
      _waitingPolicies.push(max_policy);
      _waitingMoves.insert(max_policy_index);
    }

    _policies[max_index].incrementVisits();
  }

  // 探索幅が指定されていない場合と子ノードの数が指定された探索幅に達していない場合、
  // 待機リストに候補手が存在する場合は新しい子ノードを作成して次の探索先として返す
  if (_waitingPolicies.size() > 0 && (width <= 0 || _children.size() < width)) {
    // 最初に登録された待機中の候補手を取得する
    Policy policy = _waitingPolicies.front();
    int32_t policy_index = _getMoveIndex(policy.getMove());

    _waitingPolicies.pop();
    _waitingMoves.erase(policy_index);

    // 未登録の候補手であれば新しい子ノードを作成して次の探索先として返す
    // 暫定的にノードの評価値には最低評価値を設定する
    if (_children.find(policy_index) == _children.end()) {
      MctsNode* node = _manager->createNode();

      node->_resetNode();
      node->_board.copyFrom(&_board);
      node->_captured = std::max(node->_board.play(policy.getMove()), 0);
      node->_move = policy.getMove();
      node->_probability = policy.getProbability();
      node->_nodeValue = _move.getColor();
      node->_parent = this;
      node->_firstChild = (_children.size() == 0);
      _children[policy_index] = node;

      return node;
    }
  }

  // 探索対象とする子ノードの一覧を作成する
  std::vector<std::pair<MctsNode*, float>> children;

  for (std::pair<int32_t, MctsNode*> child : _children) {
    children.push_back(std::make_pair(
        child.second, child.second->getMctsValueLCB() * getNextColor()));
  }

  if (children.empty()) {
    return nullptr;
  }

  // 探索幅が指定されている場合は探索対象とする子ノードの数を制限する
  if (width > 0 && children.size() > static_cast<size_t>(width)) {
    std::sort(children.begin(), children.end(), [](auto a, auto b) {
      return a.second > b.second;
    });

    children.resize(width);
  }

  // 最も優先度が高いノードを次の探索先として返す
  MctsNode* max_node = nullptr;
  float max_priority = -std::numeric_limits<float>::infinity();

  for (std::pair<MctsNode*, float> child : children) {
    float priority;

    // パスのノードは探索しない
    if (child.first->_move.isPass()) {
      continue;
    }
    // 探索回数を均等にする設定となっている場合は、
    // 訪問回数に基づいて優先度を計算する（訪問回数が同じならば評価値を考慮する）
    else if (equally) {
      float visits = static_cast<float>(child.first->getVisits());
      float value = child.first->getMctsValue() * getNextColor();
      priority = 1.0f / (visits + 1 - value * 0.5f);
    }
    // そうでない場合はPUCBに基づいて優先度を計算する
    else {
      priority = child.first->getPriorityByPUCB(_visits);
    }

    // 優先度の高いノードを残す
    if (max_priority < priority) {
      max_node = child.first;
      max_priority = priority;
    }
  }

  // 最も優先度の高いノードを次の探索先として返す
  return max_node;
}

}  // namespace deepgo
