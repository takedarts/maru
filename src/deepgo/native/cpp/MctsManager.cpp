#include "MctsManager.h"

#include <sstream>

#include "MctsNode.h"

namespace deepgo {

/**
 * ノード管理オブジェクトを作成する。
 * @param parameter ノード生成パラメータ
 */
MctsManager::MctsManager(const MctsParameter& parameter)
    : _mutex(),
      _parameter(parameter),
      _nodes(),
      _poolNodes(),
      _usedNodes() {
}

/**
 * ノードオブジェクトを作成する。
 * @return ノードオブジェクト
 */
MctsNode* MctsManager::createNode() {
  std::lock_guard<std::mutex> lock(_mutex);

  // ノードオブジェクトを作成する
  // 未使用のノードオブジェクトがあればそれを利用する
  MctsNode* node;

  if (_poolNodes.empty()) {
    _nodes.emplace_back(std::make_unique<MctsNode>(this));
    node = _nodes.back().get();
  } else {
    node = _poolNodes.back();
    _poolNodes.pop_back();
  }

  // 使用中のノードオブジェクトとして登録する
  _usedNodes.insert(node);

  // ノードオブジェクトを返す
  return node;
}

/**
 * ノードオブジェクトを未使用状態にする。
 * @param node ノードオブジェクト
 */
void MctsManager::releaseNode(MctsNode* node) {
  std::lock_guard<std::mutex> lock(_mutex);

  // すでに未使用状態なら何もしない
  if (_usedNodes.find(node) == _usedNodes.end()) {
    return;
  }

  // ノードオブジェクトを未使用状態にする
  _usedNodes.erase(node);
  _poolNodes.push_back(node);
}

/**
 * ノードとその子孫を未使用状態にする。
 * @param root 解放する探索木のルート
 */
void MctsManager::releaseTree(MctsNode* root) {
  std::vector<MctsNode*> stack = {root};

  // 深さ優先で探索して、ノードを未使用状態にする
  while (!stack.empty()) {
    MctsNode* current = stack.back();
    stack.pop_back();

    for (MctsNode* child : current->getChildren()) {
      stack.push_back(child);
    }

    releaseNode(current);
  }
}

/**
 * MCTSの探索ノード管理オブジェクトの状態を文字列として取得する。
 * @return 状態を表す文字列
 */
std::string MctsManager::toString() {
  std::lock_guard<std::mutex> lock(_mutex);
  std::ostringstream ss;

  ss << "NodeManager: nodes=" << _nodes.size();
  ss << "(" << _usedNodes.size() << "/" << _poolNodes.size() << ")";
  ss << std::endl;

  return ss.str();
}

}  // namespace deepgo
