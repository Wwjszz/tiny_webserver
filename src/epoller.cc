#include "epoller.h"

epoller::epoller(size_t max_event_)
    : epoll_fd_(epoll_create(512)), events_(max_event_) {}

epoller::~epoller() { close(epoll_fd_); }

bool epoller::add_fd(int fd, uint32_t events) {
  epoll_event ee = {0};
  ee.data.fd = fd;
  ee.events = events;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ee) == 0;
}

bool epoller::mod_fd(int fd, uint32_t events) {
  epoll_event ee = {0};
  ee.data.fd = fd;
  ee.events = events;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ee) == 0;
}

bool epoller::del_fd(int fd) {
  return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == 0;
}

int epoller::wait(int timeout_ms) {
  return epoll_wait(epoll_fd_, &events_[0], events_.size(), timeout_ms);
}

int epoller::event_fd(size_t idx) const { return events_[idx].data.fd; }

uint32_t epoller::events(size_t idx) const { return events_[idx].events; }