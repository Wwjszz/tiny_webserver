#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

class epoller {
 public:
  explicit epoller(size_t max_event_ = 1024);
  ~epoller();

  bool add_fd(int fd, uint32_t events);
  bool mod_fd(int fd, uint32_t events);
  bool del_fd(int fd);
  int wait(int timeout_ms = -1);
  int event_fd(size_t idx) const;
  uint32_t events(size_t idx) const;

 private:
  int epoll_fd_;
  std::vector<epoll_event> events_;
};

#endif