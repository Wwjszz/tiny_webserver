#ifndef SQL_CONNPOOL_H
#define SQL_CONNPOOL_H

#include <assert.h>
#include <mysql/mysql.h>

#include <atomic>
#include <memory>
#include <semaphore>

#include "block_queue.hpp"

using mysql = struct MYSQL_WARRPER {
  MYSQL* mysql_conn_;
  MYSQL_WARRPER(MYSQL* conn) : mysql_conn_(conn) {}
  ~MYSQL_WARRPER() { mysql_close(mysql_conn_); }
};

class sql_connpool {
 public:
  static sql_connpool* instance();
  std::unique_ptr<mysql> get_conn();
  void free_conn(std::unique_ptr<mysql>);
  int get_free_conn_count();

  void init(const char* host, int port, const char* user, const char* pwd,
            const char* db_name, int conn_size = 10);

 private:
  // static void sql_deleter_(MYSQL* conn) { mysql_close(conn); }
  // using sql_d_ = decltype(sql_deleter_);
  sql_connpool() = default;
  ~sql_connpool() { mysql_library_end(); }
  int max_conn_;
  block_queue<std::unique_ptr<mysql>> conn_que_;
};

struct sql_RAII {
  std::unique_ptr<mysql> sql_conn_;
  sql_connpool* sql_pool_;
  sql_RAII(MYSQL** sql, sql_connpool* pool) {
    assert(pool);
    sql_conn_ = pool->get_conn();
    *sql = sql_conn_.get()->mysql_conn_;
    sql_pool_ = pool;
  }
  ~sql_RAII() {
    if (sql_pool_ && sql_conn_) sql_pool_->free_conn(std::move(sql_conn_));
  }
};

#endif