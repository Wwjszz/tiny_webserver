#include "buffer.h"

#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

buffer::buffer()
    : buffer_(prepend_ + initial_size_),
      read_index_(prepend_),
      write_index_(prepend_) {
  assert(readable_bytes() == 0);
  assert(writeable_bytes() == initial_size_);
  assert(prepend_bytes() == prepend_);
}

ssize_t buffer::read(char* dest, size_t len) {
  if (len < 0) return -1;
  size_t a_len = std::min(readable_bytes(), len);
  strncpy(dest, peek(), a_len);
  retrieve(a_len);
  return a_len;
}

void buffer::append(const char* data, size_t len) {
  assert(data);
  ensure_writeable(len);
  std::copy(data, data + len, write_begin_());
  has_writen(len);
}

void buffer::append(const std::string& str) { append(str.c_str(), str.size()); }

void buffer::shrink_(size_t reserve) {
  buffer other;
  other.ensure_writeable(readable_bytes() + reserve);
  other.append(peek(), readable_bytes());
  *this = std::move(other);
}

void buffer::make_space_(size_t len) {
  if (writeable_bytes() + prepend_bytes() < len + prepend_) {
    buffer_.resize(len + write_index_ + 1);
  } else {
    size_t readable = readable_bytes();
    std::copy(begin_() + read_index_, begin_() + write_index_,
              begin_() + prepend_);
    read_index_ = prepend_;
    write_index_ = read_index_ + readable;
    assert(readable == readable_bytes());
  }
}

ssize_t buffer::read_fd(int fd, int& Errno) {
  char extra[65536];
  iovec iov[2];
  size_t writeable = writeable_bytes();
  iov[0].iov_base = write_begin_();
  iov[0].iov_len = writeable;
  iov[1].iov_base = extra;
  iov[1].iov_len = sizeof(extra);

  ssize_t len = readv(fd, iov, 2);
  if (len < 0) {
    Errno = errno;
  } else if (len <= writeable) {
    has_writen(len);
  } else {
    has_writen(writeable);
    append(extra, len - writeable);
  }
  return len;
}

ssize_t buffer::write_fd(int fd, int& Errno) {
  ssize_t len = write(fd, peek(), readable_bytes());
  if (len < 0) {
    Errno = errno;
    return len;
  }
  retrieve(len);
  return len;
}

std::pair<bool, std::string> buffer::search(const char* start, size_t len) {
  const char* end =
      std::search(peek(), write_begin_const_(), start, start + len);
  if (end == write_begin_const_()) {
    return {false, ""};
  }
  return {true, retrieve_as_string(end - peek())};
}
