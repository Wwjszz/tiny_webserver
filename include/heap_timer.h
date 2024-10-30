#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

class heap_timer {
 public:
  using timeout_callback = std::function<void()>;
  heap_timer() {
    heap_.reserve(64);
    heap_.emplace_back();
  }
  ~heap_timer() = default;

  void adjust(size_t idx, int new_expire);
  void add(size_t id, int timeout, const timeout_callback& cb);
  void do_work(size_t id);
  void clear() {
    heap_.clear();
    ref_.clear();
  }
  void tick();
  void pop() { del_(1); }
  int get_next_tick();

 private:
  using chrono_clock = std::chrono::high_resolution_clock;
  using ms = std::chrono::milliseconds;
  using time_stamp = chrono_clock::time_point;

  template <class T>
  auto duration_cast(const T& dur) {
    return std::chrono::duration_cast<ms>(dur);
  }

  struct timer_node {
    int id_;
    time_stamp expire_;
    timeout_callback cb_;
    bool operator<(const timer_node& other) { return expire_ < other.expire_; }
    timer_node() = default;
    timer_node(int id, time_stamp expire, timeout_callback cb)
        : id_(id), expire_(expire), cb_(cb) {}
  };

  void del_(size_t idx);
  void up_(size_t idx);
  void down_(size_t idx);
  void swap_node_(size_t lhs, size_t rhs);

  std::vector<timer_node> heap_;
  std::unordered_map<int, size_t> ref_;
};

#endif