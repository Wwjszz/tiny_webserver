#ifndef BUFFER_H
#define BUFFER_H

/*
  buffer:
    pre read write

*/

#include <assert.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class buffer {
 public:
  buffer();

  ~buffer() = default;

  buffer &operator=(buffer &&other) {
    buffer_ = std::move(other.buffer_);
    read_index_ = other.read_index_;
    write_index_ = other.write_index_;
    return *this;
  }

  size_t readable_bytes() { return write_index_ - read_index_; }
  size_t writeable_bytes() { return buffer_.size() - write_index_; }
  size_t prepend_bytes() { return read_index_; }

  const char *peek() const { return begin_() + read_index_; }

  void ensure_writeable(size_t len) {
    if (len > writeable_bytes()) {
      make_space_(len);
    }
    assert(len <= writeable_bytes());
  }
  void has_writen(size_t len) { write_index_ += len; }

  void retrieve(size_t len) {
    assert(len <= readable_bytes());
    if (len < readable_bytes()) {
      read_index_ += len;
    } else {
      retrieve_all();
    }
  }

  void retrieve_until(const char *end) {
    assert(end <= peek());
    assert(end <= write_begin_());
    retrieve(end - peek());
  }

  void retrieve_all() { write_index_ = read_index_ = prepend_; }

  std::string retrieve_all_as_string() {
    return retrieve_as_string(readable_bytes());
  }

  std::string retrieve_as_string(size_t len) {
    assert(len <= readable_bytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  void append(const char *data, size_t len);
  void append(const std::string &str);

  void shrink_(size_t reserve);
  size_t internal_capacity_() const { return buffer_.capacity(); }

  ssize_t read_fd(int fd, int &Errno);
  ssize_t write_fd(int fd, int &Errno);

  char *write_begin_() { return begin_() + write_index_; }
  const char *write_begin_() const {
    return static_cast<const char *>(
        const_cast<buffer *>(this)->write_begin_());
  }

  void output() {
    printf("%s\n", std::string(peek(), readable_bytes()).c_str());
  }

 private:
  static const size_t prepend_ = 8;
  static const size_t initial_size_ = 1024;
  std::vector<char> buffer_;
  size_t read_index_;
  size_t write_index_;

  char *begin_() { return &*buffer_.begin(); }
  const char *begin_() const {
    return static_cast<const char *>(const_cast<buffer *>(this)->begin_());
  }

  void make_space_(size_t len);
};

#endif