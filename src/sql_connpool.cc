#include "sql_connpool.h"

#include "log.h"

// sql_connpool::sql_connpool() : sem_(0), mutex_(0) {}

sql_connpool* sql_connpool::instance() {
  static sql_connpool sql_coonpool_;
  return &sql_coonpool_;
}

std::unique_ptr<mysql> sql_connpool::get_conn() {
  std::unique_ptr<mysql> mysql_;
  conn_que_.pop(mysql_);
  return mysql_;
}

void sql_connpool::free_conn(std::unique_ptr<mysql> item) {
  conn_que_.push_back(std::move(item));
}

int sql_connpool::get_free_conn_count() { return conn_que_.size(); }

void sql_connpool::init(const char* host, int port, const char* user,
                        const char* pwd, const char* db_name, int conn_size) {
  if (conn_size > 1000) conn_size = 1000;
  for (int i = 0; i < conn_size; ++i) {
    MYSQL* conn = nullptr;
    conn = mysql_init(conn);
    if (!conn) {
      LOG_ERROR("Mysql init error");
      assert(conn);
    }
    conn = mysql_real_connect(conn, host, user, pwd, db_name, port, nullptr, 0);
    if (!conn) {
      LOG_ERROR("Mysql connect error");
      assert(conn);
    }
    conn_que_.push_back(std::make_unique<mysql>(conn));
  }
  max_conn_ = conn_size;
}
