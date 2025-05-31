#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace deepgo {

/**
 * スレッド管理クラス。
 */
class ThreadPool {
 public:
  /**
   * スレッド管理オブジェクトを作成する。
   * @param threads スレッド数
   */
  ThreadPool(int32_t threads);

  /**
   * スレッド管理オブジェクトを破棄する。
   */
  virtual ~ThreadPool();

  /**
   * 実行対象のタスクを登録する。
   * @param task タスク
   */
  void submit(std::function<void()> task);

  /**
   * スレッド数を返す。
   * @return スレッド数
   */
  int32_t getSize();

 private:
  /**
   * 同期用のミューテックス。
   */
  std::mutex _mutex;

  /**
   * 同期用の条件変数。
   */
  std::condition_variable _condition;

  /**
   * スレッドオブジェクトの一覧。
   */
  std::vector<std::thread> _threads;

  /**
   * 待機中のタスクの一覧。
   */
  std::queue<std::function<void()>> _tasks;

  /**
   * 動作を停止するならtrue。
   */
  bool _terminated;

  /**
   * 探索を実行する。
   */
  void _run();
};

}  // namespace deepgo
