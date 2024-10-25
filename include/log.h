#ifndef LOG_H
#define LOG_H

#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "block_queue.hpp"
#include "buffer.h"

class log {
 public:
  void init(int level, const char *path = "./log", const char *suffix = ".log",
            int max_queue_capacity = 1024);

  static log *Instance();
  static void flush_log_thread();

  void write(int level, const char *format, ...);
  void flush();

  bool is_open() { return is_open_; }

  // FIXME: THREAD PROBLEM
  int get_level() { return level_; }
  void set_level(int level) { level_ = level; }

 private:
  log() = default;
  void append_log_level_title(int level);
  virtual ~log();
  void async_write();

 private:
  static const int LOG_PATH_LEN = 256;
  static const int LOG_NAME_LEN = 256;
  static const int MAX_LOG_LEN = 256;
  static const int MAX_LINES = 50000;

  const char *path_;
  const char *suffix_;

  size_t max_lines_;
  size_t line_count_;
  size_t today_;
  bool is_open_;

  buffer buff_;
  size_t level_;
  bool is_async;

  FILE *m_fp_;
  char *m_buf_;
  std::unique_ptr<block_queue<std::string>> deq_ptr_;
  bool is_thread_close;
  size_t timeout{1};
  std::unique_ptr<std::thread> write_thread_ptr;
  std::mutex mutex_;
};

#endif