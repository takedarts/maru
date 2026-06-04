#include "ThreadPool.h"

namespace deepgo {

/**
 * Creates a thread pool object.
 * @param threads number of threads
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
 * Destroys the thread pool object.
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
 * Submits a task for execution.
 * @param task task to execute
 */
void ThreadPool::submit(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(_mutex);
    _tasks.push(task);
  }

  _condition.notify_all();
}

/**
 * Worker function that executes tasks.
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
 * Returns the number of threads.
 * @return number of threads
 */
int32_t ThreadPool::getSize() {
  return static_cast<int32_t>(_threads.size());
}

}  // namespace deepgo
