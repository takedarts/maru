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
 * Thread management class.
 */
class ThreadPool {
 public:
  /**
   * Create thread management object.
   * @param threads Number of threads
   */
  ThreadPool(int32_t threads);

  /**
   * Destroy thread management object.
   */
  virtual ~ThreadPool();

  /**
   * Register a task to execute.
   * @param task Task
   */
  void submit(std::function<void()> task);

  /**
   * Return the number of threads.
   * @return Number of threads
   */
  int32_t getSize();

 private:
  /**
   * Mutex for synchronization.
   */
  std::mutex _mutex;

  /**
   * Condition variable for synchronization.
   */
  std::condition_variable _condition;

  /**
   * List of thread objects.
   */
  std::vector<std::thread> _threads;

  /**
   * List of waiting tasks.
   */
  std::queue<std::function<void()>> _tasks;

  /**
   * True to stop operation.
   */
  bool _terminated;

  /**
   * Execute search.
   */
  void _run();
};

}  // namespace deepgo
