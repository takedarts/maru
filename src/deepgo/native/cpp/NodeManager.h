#pragma once

#include <mutex>
#include <vector>

#include "Node.h"
#include "NodeParameter.h"

namespace deepgo {

/**
 * ノードオブジェクトを管理するためのクラス。
 */
class NodeManager {
 public:
  /**
   * ノードオブジェクトの管理クラスを作成する。
   * @param processor 推論を実行するオブジェクト
   * @param width 盤面の幅
   * @param height 盤面の高さ
   * @param komi コミの目数
   * @param rule 勝敗判定ルール
   * @param superko スーパーコウルールを適用するならtrue
   */
  NodeManager(
      Processor* processor, int32_t width, int32_t height,
      float komi, int32_t rule, bool superko);

  /**
   * 初期ノードとして使用できるノードオブジェクトを作成する。
   * 必ず新しいノードオブジェクトを作成する。
   * @return 初期ノードオブジェクト
   */
  Node* createInitNode();


  /**
   * ノードオブジェクトを作成する。
   * 未使用のノードオブジェクトがあればそれを返し、なければ新規作成する。
   * @return ノードオブジェクト
   */
  Node* createNode();

  /**
   * ノードオブジェクトを未使用状態にする。
   * @param node ノードオブジェクト
   */
  void releaseNode(Node* node);

  /**
   * このノード管理オブジェクトのの情報を出力する。
   * @param os 出力先
   */
  void print(std::ostream& os = std::cout);

 private:
  /**
   * 同期オブジェクト。
   */
  std::mutex _mutex;

  /**
   * ノードオブジェクトを作成するときに使用するパラメータ。
   */
  NodeParameter _parameter;

  /**
   * ノードオブジェクトの一覧。
   */
  std::vector<std::unique_ptr<Node>> _nodes;

  /**
   * 未使用のノードオブジェクトの一覧。
   */
  std::vector<Node*> _poolNodes;

  /**
   * 使用中のノードオブジェクトの一覧。
   */
  std::set<Node*> _usedNodes;
};

}  // namespace deepgo
