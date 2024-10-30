#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <memory>
#include <unordered_map>

#include "epoller.h"
#include "heap_timer.h"
#include "http_conn.h"
#include "threadpool.h"

class webserver {
 public:
  webserver(int port, int trig_mode, int timeout_ms, bool opt_linger,
            int sql_port, const char *sql_user, const char *sql_pwd,
            const char *db_name, int connect_pool_num, int threads_num,
            bool open_log, int log_level, int log_que_size);
  ~webserver();

  void start();

 private:
  bool init_socket_();
  void init_event_mode_(int trig_mode);
  void add_client_(int fd, sockaddr_in addr);

  void deal_listen_();
  void deal_write_(http_conn *client);
  void deal_read_(http_conn *client);

  void send_error_(int fd, const char *info);
  void extent_time_(http_conn *client);
  void close_conn_(http_conn *client);

  void read_(http_conn *client);
  void write_(http_conn *client);
  void process_(http_conn *client);

  static const int MAX_FD_ = 65536;

  static int setnonblock(int fd);

  int port_;
  bool open_linger_;
  int timeout_ms_;
  bool is_close_;
  int listen_fd_;
  char *src_dir_;

  uint32_t listen_event_;
  uint32_t conn_event_;

  std::unique_ptr<heap_timer> timer_;
  std::unique_ptr<threadpool> threadpool_;
  std::unique_ptr<epoller> epoller_;
  std::unordered_map<int, http_conn> users_;
};

#endif