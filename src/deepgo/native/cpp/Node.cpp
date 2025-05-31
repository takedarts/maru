#include "Node.h"

#include <cmath>
#include <random>

#include "NodeManager.h"

namespace deepgo {

// 乱数生成器
static std::random_device random_seed_gen;
static std::default_random_engine random_engine(random_seed_gen());

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
Node::Node(
    NodeManager* manager, Processor* processor,
    int32_t width, int32_t height, float komi, int32_t rule, bool superko)
    : _evalMutex(),
      _valueMutex(),
      _manager(manager),
      _board(width, height),
      _x(-1),
      _y(-1),
      _color(WHITE),
      _captured(0),
      _policy(0.0f),
      _evaluator(processor, komi, rule, superko),
      _children(),
      _priorityQueue(),
      _waitingQueue(),
      _waitingSet(),
      _visits(0),
      _value(0.0f),
      _count(0) {
}

/**
 * ノードの評価情報を初期化する。
 */
void Node::reset() {
  _evaluator.clear();
  _children.clear();
  _priorityQueue = std::priority_queue<Policy>();
  _waitingQueue = std::queue<Policy>();
  _waitingSet.clear();
  _visits = 0;
  _value = 0.0f;
  _count = 0;
}

/**
 * 探索ノードを評価して、次に評価するノードオブジェクトを取得する。
 * 次に評価するノードオブジェクトが存在しない場合はnullptrを返す。
 * @param equally 探索回数を均等にする場合はtrue
 * @param width 探索幅(0の場合は探索幅を自動で調整する)
 * @param useUcb1 UCB1を使用する場合はtrue・PUCBを使用する場合はfalse
 * @return 次に評価するノードオブジェクト
 */
NodeResult Node::evaluate(bool equally, int32_t width, bool useUcb1) {
  std::unique_lock<std::shared_mutex> lock(_evalMutex);

  // ノードの評価を実行する
  _evaluate();

  // 訪問数を増やす
  _visits += 1;

  // 最初の訪問ならばこのノードの評価結果を返す
  if (_visits == 1) {
    return NodeResult(nullptr, _evaluator.getValue());
  }

  // 候補手がない場合はこのノードの評価値を返す
  if (_priorityQueue.empty()) {
    return NodeResult(nullptr, _evaluator.getValue());
  }

  // キューから評価に追加する候補手を取得する
  if (!_priorityQueue.empty()) {
    Policy policy = _priorityQueue.top();
    int32_t policy_index = policy.y * _board.getWidth() + policy.x;

    _priorityQueue.pop();

    // 探索幅が指定されていて、子ノードと待機リストの合計が探索幅より少ない場合は
    // 新しい候補手が取得されるまでキューから候補手を取り出す
    if (width > 0 && _children.size() + _waitingSet.size() < width) {
      std::vector<Policy> temp_policies;

      while (!_priorityQueue.empty() &&
             (_children.find(policy_index) != _children.end() ||
              _waitingSet.find(policy_index) != _waitingSet.end())) {
        // 一時的に取り出した候補手を保存しておく
        temp_policies.push_back(policy);

        // 探索対象とする新しい候補手を取得する
        policy = _priorityQueue.top();
        policy_index = policy.y * _board.getWidth() + policy.x;
        _priorityQueue.pop();
      }

      // 一時的に取り出した候補手をキューに戻す
      for (Policy temp_policy : temp_policies) {
        _priorityQueue.push(temp_policy);
      }
    }

    // 探索対象とした候補手の訪問数を増やしてキューに再度追加する
    _priorityQueue.emplace(policy.x, policy.y, policy.policy, policy.visits + 1);

    // 評価に追加する候補手が未登録状態であれば新たに待機リストに登録する
    if (_children.find(policy_index) == _children.end() &&
        _waitingSet.find(policy_index) == _waitingSet.end()) {
      _waitingQueue.push(policy);
      _waitingSet.insert(policy_index);
    }
  }

  // 探索幅が指定されていない場合と子ノードの数が指定された探索幅に達していない場合、
  // 待機リストに候補手が存在する場合は新しい子ノードを作成して次の探索先として返す
  if (_waitingQueue.size() > 0 && (width <= 0 || _children.size() < width)) {
    // 最初に登録された待機中の候補手を取得する
    Policy policy = _waitingQueue.front();
    int32_t policy_index = policy.y * _board.getWidth() + policy.x;

    _waitingQueue.pop();
    _waitingSet.erase(policy_index);

    // 未登録の候補手であれば新しい子ノードを作成して次の探索先として返す
    if (_children.find(policy_index) == _children.end()) {
      Node* node = _manager->createNode();

      node->_setAsNextNode(this, policy.x, policy.y, policy.policy);
      _children[policy_index] = node;

      return NodeResult(node, 0.0);
    }
  }

  // 探索対象とする子ノードの一覧を作成する
  std::vector<std::pair<Node*, float>> children;

  for (std::pair<int32_t, Node*> child : _children) {
    children.push_back(std::make_pair(
        child.second, child.second->getValueLCB() * child.second->getColor()));
  }

  // 探索幅が指定されている場合は探索対象とする子ノードの数を制限する
  if (width > 0 && children.size() > width) {
    std::sort(children.begin(), children.end(), [](auto a, auto b) {
      return a.second > b.second;
    });

    children.resize(width);
  }

  // 最も優先度が高いノードを次の探索先として返す
  Node* max_node = children[0].first;
  float max_priority = -1.0;

  for (std::pair<Node*, float> child : children) {
    float priority;

    if (equally) {
      float visits = child.first->getVisits();
      float value = child.first->getValue() * child.first->getColor();
      priority = 1.0 / (visits + 1 - value * 0.5);
    } else if (useUcb1) {
      priority = child.first->getPriorityByUCB1(_visits);
    } else {
      priority = child.first->getPriorityByPUCB(_visits);
    }

    if (max_priority < priority) {
      max_node = child.first;
      max_priority = priority;
    }
  }

  return NodeResult(max_node, 0.0);
}

/**
 * 探索ノードを更新する。
 * @param value 反映する予想勝率
 */
void Node::update(float value) {
  std::unique_lock<std::shared_mutex> lock(_valueMutex);
  _value += value;
  _count += 1;
}

/**
 * PolicyNetworkの評価値をもとに候補手をランダムに取得する。
 * @param temperature 温度
 * @return 候補手
 */
std::pair<int32_t, int32_t> Node::getRandomMove(float temperature) {
  // 温度パラメータを補正する
  temperature = std::max(temperature, 0.1f);

  // ノードの評価を実行する
  {
    std::unique_lock<std::shared_mutex> lock(_evalMutex);
    _evaluate();
  }

  // 候補手の一覧を取得する
  std::vector<Policy> policies;
  std::vector<float> probs;

  {
    std::shared_lock<std::shared_mutex> lock(_evalMutex);
    for (Policy policy : _evaluator.getPolicies()) {
      policies.push_back(policy);
      probs.push_back(std::pow(policy.policy, 1.0 / temperature));
    }
  }

  // 候補手がない場合はパスを返す
  if (policies.empty()) {
    return std::make_pair(-1, -1);
  }

  // 着手確率に基づいてランダムに選択する
  std::discrete_distribution<int32_t> dist(probs.begin(), probs.end());
  int32_t index = dist(random_engine);
  Policy policy = policies[index];

  return std::make_pair(policy.x, policy.y);
}

/**
 * PolicyNetworkの評価値が最も高い候補手を取得する。
 * @return 候補手
 */
std::pair<int32_t, int32_t> Node::getPolicyMove() {
  {
    // ノードの評価を実行する
    std::unique_lock<std::shared_mutex> lock(_evalMutex);
    _evaluate();
  }

  // 候補手の一覧を取得する
  std::vector<Policy> policies;

  {
    std::shared_lock<std::shared_mutex> lock(_evalMutex);
    for (Policy policy : _evaluator.getPolicies()) {
      policies.push_back(policy);
    }
  }

  // 候補手がない場合はパスを返す
  if (policies.empty()) {
    return std::make_pair(-1, -1);
  }

  // 最も着手確率が高い候補手を取得する
  Policy max_policy = policies[0];

  for (Policy policy : policies) {
    if (max_policy.policy < policy.policy) {
      max_policy = policy;
    }
  }

  // 最も着手確率が高い候補手を返す
  return std::make_pair(max_policy.x, max_policy.y);
}

/**
 * 着手座標のx座標を取得する。
 * @return x座標
 */
int32_t Node::getX() {
  return _x;
}

/**
 * 着手座標のy座標を取得する。
 * @return y座標
 */
int32_t Node::getY() {
  return _y;
}

/**
 * 着手した石の色を取得する。
 * @return 石の色
 */
int32_t Node::getColor() {
  return _color;
}

/**
 * このノードで打ち上げた石の数を取得する。
 * @return 打ち上げた石の数
 */
int32_t Node::getCaptured() {
  return _captured;
}

/**
 * このノードの予想着手確率を取得する。
 * @return 予想着手確率
 */
float Node::getPolicy() {
  return _policy;
}

/**
 * 子ノードの一覧を取得する。
 * @return ノードオブジェクトの一覧
 */
std::vector<Node*> Node::getChildren() {
  std::shared_lock<std::shared_mutex> lock(_evalMutex);
  std::vector<Node*> children;

  for (std::pair<int32_t, Node*> item : _children) {
    children.push_back(item.second);
  }

  return children;
}

/**
 * 指定した座標に着手したときのノードオブジェクトを取得する。
 * ノードオブジェクトが存在しない場合は新しく作成したオブジェクトを返す。
 * 作成したノードオブジェクトはこのノードオブジェクトの子ノードとしては登録されない。
 * @param x 着手するX座標
 * @param y 着手するY座標
 * @return ノードオブジェクトへのポインタ
 */
Node* Node::getChild(int32_t x, int32_t y) {
  std::unique_lock<std::shared_mutex> lock(_evalMutex);

  // 子ノードが存在する場合はそのノードを返す
  int32_t index = y * _board.getWidth() + x;

  if (_children.find(index) != _children.end()) {
    return _children[index];
  }

  // 子ノードが存在しない場合は新しいノードを作成する
  Node* node = _manager->createNode();

  node->_setAsNextNode(this, x, y, 1.0);

  return node;
}

/**
 * このノードの探索回数を取得する。
 * @return 探索回数
 */
int32_t Node::getVisits() {
  std::shared_lock<std::shared_mutex> lock(_evalMutex);
  return _visits;
}

/**
 * このノードの評価値を取得する。
 * @return 評価値
 */
float Node::getValue() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return 0.0f;
  } else {
    return _value / _count;
  }
}

/**
 * このノードの評価値の信頼区間の下限を取得する。
 * @return 信頼区間の下限
 */
float Node::getValueLCB() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return 0.0f;
  } else {
    float value = _value / _count;
    float lower = 1.96 * 0.5 / std::sqrt(_visits + 1);
    return value - (lower * _color);
  }
}

/**
 * PUCBに基づいてこのノードの優先度を取得する。
 * @param totalVisits 探索回数の合計
 * @return 優先度
 */
float Node::getPriorityByPUCB(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return -99.0f;
  } else {
    float c_puct = std::log((1 + totalVisits + 19652.0) / 19652.0) + 1.25;
    float value = (_value / _count) * _color;
    float upper = c_puct * _policy * std::sqrt(totalVisits) / (1 + _visits);
    return value + 2 * upper;
  }
}

/**
 * UCB1に基づいてこのノードの優先度を取得する。
 * @param totalVisits 探索回数の合計
 * @return 優先度
 */
float Node::getPriorityByUCB1(int32_t totalVisits) {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  if (_count == 0) {
    return -99.0f;
  } else {
    float value = (_value / _count) * _color;
    float upper = 0.5 * std::sqrt(std::log(totalVisits) / (_visits + 1));
    return value + upper;
  }
}

/**
 * このノードの予想進行を取得する。
 * @return 予想進行
 */
std::vector<std::pair<int32_t, int32_t>> Node::getVariations() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  std::vector<std::pair<int32_t, int32_t>> variations;
  int32_t max_visits = 0;
  Node* max_child = nullptr;

  for (auto child : _children) {
    if (child.second->_visits > max_visits) {
      max_visits = child.second->_visits;
      max_child = child.second;
    }
  }

  variations.push_back(std::make_pair(_x, _y));

  if (max_child != nullptr) {
    std::vector<std::pair<int32_t, int32_t>> child_variations = max_child->getVariations();
    variations.insert(variations.end(), child_variations.begin(), child_variations.end());
  }

  return variations;
}

/**
 * 盤面の状態を取得する。
 * @return 盤面の状態
 */
std::vector<int32_t> Node::getBoardState() {
  std::shared_lock<std::shared_mutex> lock(_valueMutex);
  return _board.getState();
}

/**
 * このノードの情報を出力する。
 * @param os 出力先
 */
void Node::print(std::ostream& os) {
  _board.print(os);
  os << "color:" << _color << std::endl;
  os << "visits:" << _visits << std::endl;
  os << "value:" << getValue() << std::endl;
}

/**
 * このノードの評価を実行する。
 */
void Node::_evaluate() {
  if (_evaluator.isEvaluated()) {
    return;
  }

  _evaluator.evaluate(&_board, OPPOSITE(_color));

  for (Policy policy : _evaluator.getPolicies()) {
    _priorityQueue.push(policy);
  }
}

/**
 * 指定されたノードの継続ノードとしての値を設定する。
 * @param prevNode 前のノード
 * @param x 着手するX座標
 * @param y 着手するY座標
 * @param policy 予想着手確率
 */
void Node::_setAsNextNode(Node* prevNode, int32_t x, int32_t y, float policy) {
  _x = x;
  _y = y;
  _color = OPPOSITE(prevNode->_color);
  _policy = policy;

  _board.copyFrom(&prevNode->_board);
  _captured = _board.play(_x, _y, _color);
}

}  // namespace deepgo
