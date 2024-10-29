#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <atomic>

#include "buffer.h"
#include "http_request.h"
#include "http_response.h"

class http_conn {
 public:
  http_conn() = default;
  ~http_conn();

  void init(int sock_fd, const sockaddr_in &addr);

  ssize_t read(int &save_errno);
  ssize_t write(int &save_errno);

  int fd() const { return fd_; }
  int port() const { return addr_.sin_port; }

  const char *ip() const { return inet_ntoa(addr_.sin_addr); }
  sockaddr_in addr() const { return addr_; }
  bool process();

  size_t bytes() const { return write_buff_.readable_bytes() + mm_file_len; }
  bool is_keep_alive() const { request_.is_keep_alive(); }

  static bool ET;
  static const char *src_dir;
  static std::atomic<int> user_count;

 private:
  int fd_ = -1;
  sockaddr_in addr_;

  bool is_close_ = true;

  char *mm_file;
  size_t mm_file_len;

  buffer read_buff_;
  buffer write_buff_;

  http_request request_;
  http_response response_;
};

#endif