#include "heap_timer.h"

void heap_timer::swap_node_(size_t lhs, size_t rhs) {
  std::swap(heap_[lhs], heap_[rhs]);
  ref_[heap_[lhs].id_] = lhs;
  ref_[heap_[rhs].id_] = rhs;
}

void heap_timer::up_(size_t idx) {
  while (idx > 1 && heap_[idx] < heap_[idx >> 1]) {
    swap_node_(idx, idx >> 1);
    idx >>= 1;
  }
}

void heap_timer::down_(size_t idx) {
  size_t heap_size_ = heap_.size() - 1;
  size_t child = idx << 1;
  for (; child <= heap_size_; child <<= 1) {
    if (child + 1 <= heap_size_ && heap_[child + 1] < heap_[child]) {
      ++child;
    }
    if (heap_[child] < heap_[idx]) {
      swap_node_(idx, child);
      idx = child;
    } else {
      break;
    }
  }
}

void heap_timer::del_(size_t idx) {
  size_t heap_size_ = heap_.size() - 1;
  if (idx < heap_size_) {
    swap_node_(idx, heap_size_);
    down_(idx);
    up_(idx);
  }
  ref_.erase(heap_.back().id_);
  heap_.pop_back();
}

void heap_timer::adjust(size_t id, int new_expire) {
  size_t idx = ref_[id];
  heap_[idx].expire_ = chrono_clock::now() + ms(new_expire);
  down_(idx);
  up_(idx);
}

void heap_timer::add(size_t id, int timeout, const timeout_callback& cb) {
  time_stamp expire = chrono_clock::now() + ms(timeout);
  if (ref_.contains(id)) {
    size_t idx = ref_[id];
    heap_[idx].expire_ = expire;
    heap_[idx].cb_ = cb;
    down_(idx);
    up_(idx);
  } else {
    heap_.emplace_back(id, expire, cb);
    size_t idx = heap_.size() - 1;
    ref_[id] = idx;
    up_(idx);
  }
}

void heap_timer::do_work(size_t id) {
  size_t idx = ref_[id];
  heap_[idx].cb_();
  del_(idx);
}

void heap_timer::tick() {
  while (heap_.size() > 1) {
    timer_node node = heap_[1];
    if (duration_cast(node.expire_ - chrono_clock::now()).count() > 0) {
      break;
    }
    node.cb_();
    pop();
  }
}

int heap_timer::get_next_tick() {
  tick();
  size_t res = -1;
  if (heap_.size() > 1) {
    res = duration_cast(heap_[1].expire_ - chrono_clock::now()).count();
    if (res < 0) {
      res = 0;
    }
  }
  return res;
}