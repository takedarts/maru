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
 * Thread pool management class.
 */
class ThreadPool {
 public:
  /**
   * Creates a thread pool object.
   * @param threads number of threads
   */
  ThreadPool(int32_t threads);

  /**
   * Destroys the thread pool object.
   */
  virtual ~ThreadPool();

  /**
   * Submits a task for execution.
   * @param task task to execute
   */
  void submit(std::function<void()> task);

  /**
   * Returns the number of threads.
   * @return number of threads
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
   * Queue of pending tasks.
   */
  std::queue<std::function<void()>> _tasks;

  /**
   * True if the pool should stop running.
   */
  bool _terminated;

  /**
   * Worker function that executes tasks.
   */
  void _run();
};

}  // namespace deepgo
