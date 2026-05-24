#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "MctsParameter.h"

namespace deepgo {

class MctsNode;

/**
 * ノードオブジェクトを管理するためのクラス。
 */
class MctsManager {
 public:
  /**
   * ノード管理オブジェクトを作成する。
   * @param parameter ノード生成パラメータ
   */
  explicit MctsManager(const MctsParameter& parameter);

  /**
   * ノードオブジェクトを作成する。
   * @return ノードオブジェクト
   */
  MctsNode* createNode();

  /**
   * ノードオブジェクトを未使用状態にする。
   * @param node ノードオブジェクト
   */
  void releaseNode(MctsNode* node);

  /**
   * ノードとその子孫を未使用状態にする。
   * @param root 解放する探索木のルート
   */
  void releaseTree(MctsNode* root);

  /**
   * MCTSの探索ノード管理オブジェクトの状態を文字列として取得する。
   * @return 状態を表す文字列
   */
  std::string toString();

  /**
   * ノード生成パラメータを取得する。
   * @return ノード生成パラメータ
   */
  inline const MctsParameter& getParameter() const {
    return _parameter;
  }

 private:
  /**
   * 同期オブジェクト。
   */
  std::mutex _mutex;

  /**
   * ノード生成パラメータ。
   */
  MctsParameter _parameter;

  /**
   * ノードオブジェクトの一覧。
   */
  std::vector<std::unique_ptr<MctsNode>> _nodes;

  /**
   * 未使用のノードオブジェクトの一覧。
   */
  std::vector<MctsNode*> _poolNodes;

  /**
   * 使用中のノードオブジェクトの一覧。
   */
  std::set<MctsNode*> _usedNodes;
};

}  // namespace deepgo
