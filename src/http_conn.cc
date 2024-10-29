#include "http_conn.h"

#include <unistd.h>

#include "log.h"

bool http_conn::ET = false;
std::atomic<int> http_conn::user_count;
const char* http_conn::src_dir = nullptr;

http_conn::~http_conn() {
  if (fd_) {
    close(fd_);
    user_count.fetch_sub(1);
    LOG_DEBUG("client[%d](%s:%d) quit, user_count:%d", fd_, ip(), port(),
              user_count.load());
  }
}

void http_conn::init(int fd, const sockaddr_in& addr) {
  addr_ = addr;
  fd_ = fd;
  user_count.fetch_add(1);
  write_buff_.retrieve_all();
  read_buff_.retrieve_all();
  LOG_INFO("client[%d](%s:%d) in, user_count:%d", fd_, ip(), port(),
           user_count.load());
}

ssize_t http_conn::read(int& save_errno) {
  ssize_t len = -1;
  do {
    len = read_buff_.read_fd(fd_, save_errno);
    if (len <= 0) {
      break;
    }
  } while (ET);
  return len;
}

/*
  TODO: async read, process, write
*/
ssize_t http_conn::write(int& save_errno) {
  ssize_t len = -1;
  do {
    if (write_buff_.readable_bytes())
      len = write_buff_.write_fd(fd_, save_errno);
    else {
      len = ::write(fd_, mm_file, mm_file_len);
    }
    if (len <= 0) break;
  } while (ET);
  return len;
}

bool http_conn::process() {
  request_.init();
  if (!request_.parse(read_buff_)) {
    return false;
  }
  LOG_DEBUG("request %s", request_.path().c_str());
  response_.init(src_dir, request_.path(), request_.is_keep_alive(), 200);

  response_.make_response(write_buff_);

  if (response_.mm_file_len() > 0 && response_.mm_file()) {
    mm_file = response_.mm_file();
    mm_file_len = response_.mm_file_len();
  }
  LOG_DEBUG("file size: %d, %d", mm_file_len, bytes());
  return true;
}