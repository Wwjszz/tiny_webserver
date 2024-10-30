#include "webserver.h"

#include <fcntl.h>
#include <string.h>

#include "log.h"
#include "sql_connpool.h"

webserver::webserver(int port, int trig_mode, int timeout_ms, bool opt_linger,
                     int sql_port, const char *sql_user, const char *sql_pwd,
                     const char *db_name, int connpool_num, int threads_num,
                     bool open_log, int log_level, int log_que_size)
    : port_(port),
      open_linger_(opt_linger),
      timeout_ms_(timeout_ms),
      timer_(std::make_unique<heap_timer>()),
      threadpool_(std::make_unique<threadpool>(threads_num)),
      epoller_(std::make_unique<epoller>()) {
  src_dir_ = getcwd(nullptr, 256);
  strcat(src_dir_, "/resources/");
  http_conn::user_count = 0;
  http_conn::src_dir = src_dir_;

  sql_connpool::instance()->init("0.0.0.0", sql_port, sql_user, sql_pwd,
                                 db_name, connpool_num);

  init_event_mode_(trig_mode);
  is_close_ = !init_socket_();
  if (open_log) {
    log::instance()->init(log_level, "./log", ".log", log_que_size);
    if (is_close_) {
      LOG_ERROR("========== server init error ==========");
    } else {
      LOG_INFO("========== server init ==========");
      LOG_INFO("port:%d, open_linger: %s", port_,
               opt_linger ? "true" : "false");
      LOG_INFO("listen mode: %s, open_conn mode: %s",
               (listen_event_ & EPOLLET ? "ET" : "LT"),
               (conn_event_ & EPOLLET ? "ET" : "LT"));
      LOG_INFO("log_sys level: %d", log_level);
      LOG_INFO("src_dir: %s", http_conn::src_dir);
      LOG_INFO("sql_connpool num: %d, threadpool num: %d", connpool_num,
               threads_num);
    }
    printf("log init success\n");
  }
}

webserver::~webserver() {
  close(listen_fd_);
  free(src_dir_);
}

void webserver::init_event_mode_(int trig_mode) {
  listen_event_ |= EPOLLRDHUP;
  conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
  switch (trig_mode) {
    case 0:
      break;
    default:
    case 1:
      conn_event_ |= EPOLLET;
    case 3:
      listen_event_ |= EPOLLET;
      break;
    case 2:
      listen_event_ |= EPOLLET;
    case 4:
      conn_event_ |= EPOLLET;
      break;
  }
}

bool webserver::init_socket_() {
  int ret;
  sockaddr_in addr;

  if (port_ > 65535 || port_ < 1024) {
    LOG_ERROR("port:%d error", port_);
    return false;
  }

  addr.sin_family = PF_INET;
  addr.sin_port = htons(port_);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  {
    linger opt_linger = {0};
    if (open_linger_) {
      opt_linger.l_linger = 1;
      opt_linger.l_onoff = 1;
    }

    listen_fd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
      LOG_ERROR("create socket error port:%d", port_);
      return false;
    }

    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &opt_linger,
                     sizeof(opt_linger));
    if (ret < 0) {
      close(listen_fd_);
      LOG_ERROR("init linger error port:%d", port_);
      return false;
    }
  }

  int optval = 1;
  ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                   sizeof(optval));
  if (ret < 0) {
    LOG_ERROR("set socket opt error");
    close(listen_fd_);
    return false;
  }

  ret = bind(listen_fd_, (sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERROR("bind port:%d error", port_);
    close(listen_fd_);
    return false;
  }

  ret = listen(listen_fd_, 6);
  if (ret < 0) {
    LOG_ERROR("listen port:%d error", port_);
    close(listen_fd_);
    return false;
  }

  ret = epoller_->add_fd(listen_fd_, listen_event_ | EPOLLIN);
  if (ret == 0) {
    LOG_ERROR("add listen error");
    close(listen_fd_);
    return false;
  }
  setnonblock(listen_fd_);
  LOG_INFO("server port:%d", port_);
  return true;
}

int webserver::setnonblock(int fd) {
  return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}

void webserver::send_error_(int fd, const char *info) {
  int ret = send(fd, info, strlen(info), 0);
  if (ret < 0) {
    LOG_WARN("send error to client[%d] error", fd);
  }
  close(fd);
}

void webserver::close_conn_(http_conn *client) {
  LOG_INFO("client[%d] quit", client->fd());
  epoller_->del_fd(client->fd());
  client->close_();
}

void webserver::add_client_(int fd, sockaddr_in addr) {
  users_[fd].init(fd, addr);
  if (timeout_ms_ > 0) {
    timer_->add(fd, timeout_ms_, [this, fd]() { close_conn_(&users_[fd]); });
  }
  epoller_->add_fd(fd, EPOLLIN | conn_event_);
  setnonblock(fd);
  LOG_INFO("client[%d] in", users_[fd].fd());
}

void webserver::deal_listen_() {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  do {
    int fd = accept(listen_fd_, (sockaddr *)&addr, &len);
    if (fd <= 0) {
      return;
    } else if (http_conn::user_count >= MAX_FD_) {
      send_error_(fd, "server busy");
      LOG_WARN("client is full");
      return;
    }
    add_client_(fd, addr);
  } while (listen_event_ & EPOLLET);
}

void webserver::deal_read_(http_conn *client) {
  extent_time_(client);
  threadpool_->add_task([this, client]() { read_(client); });
}

void webserver::deal_write_(http_conn *client) {
  extent_time_(client);
  threadpool_->add_task([this, client]() { write_(client); });
}

void webserver::extent_time_(http_conn *client) {
  if (timeout_ms_ > 0) {
    timer_->adjust(client->fd(), timeout_ms_);
  }
}

void webserver::read_(http_conn *client) {
  int ret = -1;
  int read_errno = 0;
  ret = client->read(read_errno);
  if (ret <= 0 && read_errno != EAGAIN) {
    close_conn_(client);
    return;
  }
  process_(client);
}

void webserver::process_(http_conn *client) {
  if (client->process()) {
    epoller_->mod_fd(client->fd(), conn_event_ | EPOLLOUT);
  } else {
    epoller_->mod_fd(client->fd(), conn_event_ | EPOLLIN);
  }
}

void webserver::write_(http_conn *client) {
  int ret = -1;
  int write_errno = 0;
  ret = client->write(write_errno);
  if (client->bytes() == 0) {
    if (client->is_keep_alive()) {
      epoller_->mod_fd(client->fd(), conn_event_ | EPOLLIN);
      return;
    }
  } else if (ret < 0) {
    if (write_errno == EAGAIN) {
      epoller_->mod_fd(client->fd(), conn_event_ | EPOLLOUT);
      return;
    }
  }
  close_conn_(client);
}

void webserver::start() {
  int time_ms = -1;
  if (is_close_) {
    LOG_INFO("========== server start ==========");
  }
  while (!is_close_) {
    if (timeout_ms_ > 0) {
      time_ms = timer_->get_next_tick();
    }
    int event_cnt = epoller_->wait(time_ms);
    for (int i = 0; i < event_cnt; ++i) {
      int fd = epoller_->event_fd(i);
      uint32_t events = epoller_->events(i);
      if (fd == listen_fd_) {
        deal_listen_();
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        close_conn_(&users_[fd]);
      } else if (events & EPOLLIN) {
        deal_read_(&users_[fd]);
      } else if (events & EPOLLOUT) {
        deal_write_(&users_[fd]);
      } else {
        LOG_ERROR("unexpected event");
      }
    }
  }
}