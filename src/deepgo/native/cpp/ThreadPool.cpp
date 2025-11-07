#include "ThreadPool.h"

#include <iostream>

namespace deepgo {

/**
 * Create thread management object.
 * @param threads Number of threads
 */
ThreadPool::ThreadPool(int32_t threads)
    : _mutex(),
      _condition(),
      _threads(),
      _tasks(),
      _terminated(false) {
  for (int32_t i = 0; i < threads; i++) {
    _threads.emplace_back([this]() { _run(); });
  }
}

/**
 * Destroy thread management object.
 */
ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _terminated = true;
    _condition.notify_all();
  }

  for (auto& thread : _threads) {
    thread.join();
  }
}

/**
 * Register a task to execute.
 * @param task Task
 */
void ThreadPool::submit(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _tasks.push(task);
  }

  _condition.notify_all();
}

/**
 * Execute search.
 */
void ThreadPool::_run() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(_mutex);
      _condition.wait(lock, [this]() { return _terminated || !_tasks.empty(); });

      if (_terminated) {
        break;
      }

      task = _tasks.front();
      _tasks.pop();
    }

    task();
  }
}

/**
 * Return the number of threads.
 * @return Number of threads
 */
int32_t ThreadPool::getSize() {
  return _threads.size();
}

}  // namespace deepgo
