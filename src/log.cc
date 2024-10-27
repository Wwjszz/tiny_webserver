#include "log.h"

#include <stdarg.h>

#include <cstring>

log::~log() {
  is_thread_close = true;
  if (write_thread_ptr && write_thread_ptr->joinable())
    write_thread_ptr->join();
  if (m_fp_) {
    fclose(m_fp_);
  }
}

// void log::flush() { fflush(m_fp_); }

void log::async_write() {
  std::string str;
  while (!is_thread_close) {
    while (deq_ptr_->pop(str, 1)) {
      fputs(str.c_str(), m_fp_);
    }
  }
}

void log::flush_log_thread() { log::instance()->async_write(); }

log *log::instance() {
  static log lg;
  return &lg;
}

void log::init(int level, const char *path, const char *suffix,
               int max_queue_capacity) {
  is_open_ = true;
  level_ = level;
  path_ = path;
  suffix_ = suffix;

  time_t timer = time(NULL);
  tm *systime = localtime(&timer);
  char filename[LOG_NAME_LEN] = {0};
  snprintf(filename, LOG_NAME_LEN, "%s/%04d_%02d_%02d%s", path_,
           systime->tm_year + 1900, systime->tm_mon + 1, systime->tm_mday,
           suffix_);
  printf("filename : %s\n", filename);
  today_ = systime->tm_mday;

  m_fp_ = fopen(filename, "a");

  assert(m_fp_ != NULL);

  if (max_queue_capacity) {
    is_async = true;
    deq_ptr_ = std::make_unique<block_queue<std::string>>();
    write_thread_ptr = std::make_unique<std::thread>(flush_log_thread);
  } else {
    is_async = false;
  }
}

void log::write(int level, const char *format, ...) {
  timeval now = {0, 0};
  gettimeofday(&now, NULL);
  time_t t_sec = now.tv_sec;
  tm *systime = localtime(&t_sec);
  tm t = *systime;

  // FIXME: FPUTS LEAK
  va_list arg_list;

  std::unique_lock lock(file_mutex_);
  if (today_ != t.tm_mday || (line_count_ && (line_count_ % MAX_LINES == 0))) {
    char new_file[LOG_NAME_LEN];
    char name[36] = {0};

    snprintf(name, sizeof(name), "%04d_%02d_%02d", t.tm_year + 1900,
             t.tm_mon + 1, t.tm_mday);

    if (today_ != t.tm_mday) {
      snprintf(new_file, LOG_NAME_LEN, "%s/%s%s", path_, name, suffix_);
      today_ = t.tm_mday;
      line_count_ = 0;
    } else {
      snprintf(new_file, LOG_NAME_LEN, "%s/%s-%d%s", path_, name,
               line_count_ / MAX_LINES, suffix_);
    }

    deq_ptr_->waiting_empty();
    fclose(m_fp_);
    m_fp_ = fopen(new_file, "a");
  }
  lock.unlock();

  char tmp[MAX_LOG_LEN];
  std::unique_lock lock_(mutex_);
  ++line_count_;
  int n = snprintf(tmp, MAX_LOG_LEN, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                   t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                   t.tm_min, t.tm_sec, now.tv_usec);
  buff_.append(tmp, strlen(tmp));
  append_log_level_title(level);

  va_start(arg_list, format);
  int m = vsnprintf(tmp, MAX_LOG_LEN, format, arg_list);
  va_end(arg_list);

  buff_.append(tmp, strlen(tmp));
  buff_.append("\n", 2);

  if (is_async && deq_ptr_ && !deq_ptr_->full()) {
    deq_ptr_->push_back(buff_.retrieve_all_as_string());
  } else {
    buff_.read(tmp, MAX_LOG_LEN);
    fputs(tmp, m_fp_);
  }
  buff_.retrieve_all();
}

void log::append_log_level_title(int level) {
  switch (level) {
    case 0:
      buff_.append("[debug]: ", 9);
      break;
    case 1:
      buff_.append("[info]: ", 8);
      break;
    case 2:
      buff_.append("[warn]: ", 8);
      break;
    case 3:
      buff_.append("[error]: ", 9);
      break;
    default:
      buff_.append("[info]: ", 9);
      break;
  }
}
