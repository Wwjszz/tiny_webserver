#include "threadpool.h"

void threadpool::init_array_() {
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

threadpool::~threadpool() {
  is_close_.store(true);
  for (auto &ptr : threads_array_) {
    ptr->join();
  }
}
