#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "block_queue.hpp"

class threadpool {
 public:
  threadpool(int threads_num, int max_queue_size = 1024, int timeout = 1)
      : threads_array_(threads_num), tasks_(max_queue_size), timeout_(timeout) {
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
  std::vector<std::unique_ptr<std::thread>> threads_array_;
  // std::array<std::unique_ptr<std::thread>, N> threads_array_;
  void init_array_();
};

#endif