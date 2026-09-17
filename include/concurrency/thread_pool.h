#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <memory>

namespace openshard {
namespace concurrency {

class ThreadPool {
 public:
  explicit ThreadPool(size_t num_threads);
  ~ThreadPool();

  // Submit task
  template <typename Func>
  void Submit(Func&& func) {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      tasks_.push(std::forward<Func>(func));
    }
    cv_.notify_one();
  }

  // Wait for all tasks to complete
  void WaitAll();

  // Get worker count
  size_t NumWorkers() const { return workers_.size(); }

 private:
  std::queue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  bool stop_;

  void Worker();
};

}  // namespace concurrency
}  // namespace openshard
