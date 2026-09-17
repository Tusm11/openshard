#include "concurrency/thread_pool.h"
#include <chrono>
#include <thread>

namespace openshard {
namespace concurrency {

ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this] { Worker(); });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  cv_.notify_all();

  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::Worker() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      cv_.wait(lock, [this] { return !tasks_.empty() || stop_; });

      if (stop_ && tasks_.empty()) {
        break;
      }

      if (tasks_.empty()) {
        continue;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
    }

    if (task) {
      task();
    }
  }
}

void ThreadPool::WaitAll() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  cv_.wait(lock, [this] { return tasks_.empty(); });
  lock.unlock();
  
  // Give worker threads time to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

}  // namespace concurrency
}  // namespace openshard
