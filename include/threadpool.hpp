#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include "block_queue.hpp"

template <size_t N = 8>
class threadpool {
 public:
  threadpool(int timeout = 1) : timeout_(timeout) { init_array_(); }

  threadpool(int max_queue_size, int timeout = 1)
      : tasks_(max_queue_size), timeout_(timeout) {
    init_array_();
  }

  ~threadpool();

  void add_task(std::function<void()> task) {
    tasks_.push_back(std::move(task));
  }

 private:
  block_queue<std::function<void()>> tasks_;
  // TODO: CHANGE ATOMIC
  std::atomic<bool> is_close_;
  int timeout_;
  std::array<std::unique_ptr<std::thread>, N> threads_array_;
  void init_array_();
};

template <size_t N>
void threadpool<N>::init_array_() {
  for (auto &t_ptr_ : threads_array_) {
    t_ptr_ = std::make_unique<std::thread>([this]() {
      while (!is_close_.load()) {
        std::function<void()> task;
        if (tasks_.pop(task, 1)) {
          task();
        }
      }
    });
  }
}

template <size_t N>
threadpool<N>::~threadpool() {
  is_close_.store(true);
  for (auto &ptr : threads_array_) {
    ptr->join();
  }
}

#endif