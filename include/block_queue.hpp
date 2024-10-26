#ifndef BLOCK_QUEUE_HPP
#define BLOCK_QUEUE_HPP

#include <chrono>
#include <deque>
#include <mutex>
#include <semaphore>
#include <shared_mutex>

template <class T>
class block_queue {
 public:
  block_queue(int maxsize = 1000);
  ~block_queue();
  bool empty();
  bool full();
  void push_back(T&& item);
  void push_front(T&& item);
  bool pop(T& item);
  bool pop(T& item, int timeout);
  void clear();
  bool front(T& value);
  bool back(T& value);
  size_t capacity();
  size_t size();

 private:
  std::counting_semaphore<> s_empty_, s_full_;
  std::shared_mutex s_mutex_;
  std::deque<T> deq_;
  // bool is_close_;
  size_t m_size_;
};

template <class T>
block_queue<T>::block_queue(int maxsize)
    : m_size_(maxsize), s_empty_(maxsize), s_full_(0) {}

template <class T>
block_queue<T>::~block_queue() {
  clear();
}

template <class T>
bool block_queue<T>::empty() {
  std::shared_lock s_lock_(s_mutex_);
  return deq_.empty();
}

template <class T>
bool block_queue<T>::full() {
  std::shared_lock s_lock_(s_mutex_);
  return deq_.size() >= m_size_;
}

template <class T>
void block_queue<T>::push_back(T&& item) {
  s_empty_.acquire();
  std::unique_lock u_lock_(s_mutex_);
  deq_.push_back(std::forward<T>(item));
  u_lock_.unlock();
  s_full_.release();
}

template <class T>
void block_queue<T>::push_front(T&& item) {
  s_empty_.acquire();
  std::unique_lock u_lock_(s_mutex_);
  deq_.push_front(std::forward<T>(item));
  u_lock_.unlock();
  s_full_.release();
}

template <class T>
bool block_queue<T>::pop(T& item) {
  s_full_.acquire();
  std::unique_lock u_lock_(s_mutex_);
  item = std::move(deq_.front());
  deq_.pop_front();
  u_lock_.unlock();
  s_empty_.release();
  return true;
}

template <class T>
bool block_queue<T>::pop(T& item, int timeout) {
  if (s_full_.try_acquire_for(std::chrono::seconds(timeout))) {
    std::unique_lock u_lock_(s_mutex_);
    item = std::move(deq_.front());
    deq_.pop_front();
    u_lock_.unlock();
    s_empty_.release();
    return true;
  }
  return false;
}

template <class T>
void block_queue<T>::clear() {
  std::lock_guard u_lock_s(s_mutex_);
  deq_.clear();
}

template <class T>
bool block_queue<T>::front(T& value) {
  std::shared_lock s_lock_(s_mutex_);
  if (deq_.empty()) return false;
  value = std::move(deq_.front());
  return true;
}

template <class T>
bool block_queue<T>::back(T& value) {
  std::shared_lock s_lock_(s_mutex_);
  if (deq_.empty()) return false;
  value = std::move(deq_.back());
  return true;
}

template <class T>
size_t block_queue<T>::capacity() {
  return m_size_;
}

template <class T>
size_t block_queue<T>::size() {
  std::shared_lock s_lock_(s_mutex_);
  return deq_.size();
}

// void close();
// void flush();

#endif