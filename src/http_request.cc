#include "http_request.h"

#include <assert.h>
#include <mysql/mysql.h>
#include <sql_connpool.h>

#include <algorithm>
#include <regex>

#include "log.h"

const std::unordered_set<std::string> http_request::DEFAULT_HTML{
    "/index", "/register", "/login", "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> http_request::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void http_request::init() {
  method_ = path_ = version_ = body_ = "";
  state_ = PARSE_STATE::REQUEST_LINE;
  header_.clear();
  post_.clear();
}

bool http_request::is_keep_alive() const {
  auto c_iter = header_.find("Connection");
  return c_iter != header_.end() && c_iter->second == "keep-alive" &&
         version_ == "1.1";
}

void http_request::parse_path_() {
  if (path_ == "/") {
    path_.append("/index.html");
  } else {
    for (auto &item : DEFAULT_HTML) {
      if (item == path_) {
        path_.append(".html");
        break;
      }
    }
  }
}

bool http_request::parse_request_line_(const std::string &line) {
  std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
  std::smatch match;
  if (std::regex_match(line, match, pattern)) {
    method_ = match[1];
    path_ = match[2];
    version_ = match[3];
    state_ = PARSE_STATE::HEADERS;
    return true;
  }
  LOG_ERROR("request_line error");
  return false;
}

void http_request::parse_header_(const std::string &line) {
  std::regex pattern("^([^:]*): ?(.*)$");
  std::smatch match;
  if (std::regex_match(line, match, pattern)) {
    header_[match[1]] = match[2];
  } else {
    state_ = PARSE_STATE::BODY;
  }
}

void http_request::parse_body_(const std::string &line) {
  body_ = line;
  parse_post_();
  state_ = PARSE_STATE::FINISH;
  LOG_DEBUG("body:%s, len:%d", line.c_str(), line.size());
}

int http_request::conver_hex(char ch) {
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return ch;
}

void http_request::parse_post_() {
  if (method_ == "POST" &&
      header_["Content-Type"] == "application/x-www-form-urlencoded") {
    parse_from_url_();
    auto iter = DEFAULT_HTML_TAG.find(path_);
    if (iter != DEFAULT_HTML_TAG.end()) {
      int tag = iter->second;
      if (tag == 0 || tag == 1) {
        bool is_login = tag == 1;
        if (user_verify(post_["username"], post_["password"], is_login)) {
          path_ = "/welcome.html";
        } else {
          path_ = "/error.html";
        }
      }
    }
  }
}

std::string http_request::path() const { return path_; }

std::string &http_request::path() { return path_; }

std::string http_request::method() const { return method_; }

std::string http_request::version() const { return version_; }

std::string http_request::get_post(const std::string &key) const {
  assert(key != "");
  auto iter = post_.find(key);
  if (iter != post_.end()) {
    return iter->second;
  }
  return "";
}

std::string http_request::get_post(const char *key) const {
  assert(key != "");
  auto iter = post_.find(key);
  if (iter != post_.end()) {
    return iter->second;
  }
  return "";
}

bool http_request::parse(buffer &buff) {
  const char CRLF[] = "\r\n";
  if (buff.readable_bytes() <= 0) {
    return false;
  }

  // FIXME: MAYBE BUG
  while (buff.readable_bytes() && state_ != PARSE_STATE::FINISH) {
    std::string line;
    if (state_ != PARSE_STATE::BODY) {
      auto res = buff.search(CRLF, 2);
      if (res.first == false) break;
      line = std::move(res.second);
    } else {
      size_t content_length = std::stoi(header_["Content-length"]);
      if (buff.readable_bytes() < content_length) {
        break;
      }
      line = buff.retrieve_as_string(content_length);
    }

    switch (state_) {
      case PARSE_STATE::REQUEST_LINE:
        if (!parse_request_line_(line)) {
          return false;
        }
        parse_path_();
        break;
        break;
      case PARSE_STATE::HEADERS:
        parse_header_(line);
        break;
      case PARSE_STATE::BODY:
        parse_body_(line);
        break;
      default:
        break;
    }

    if (state_ != PARSE_STATE::FINISH) {
      buff.retrieve(2);
    }

    if (state_ == PARSE_STATE::BODY && !header_.contains("Content-length")) {
      state_ = PARSE_STATE::FINISH;
    }
  }

  if (state_ != PARSE_STATE::FINISH) {
    return false;
  }

  LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(),
            version_.c_str());
  return true;
}

void http_request::parse_from_url_() {
  if (body_.size() == 0) {
    return;
  }
  std::regex pattern("(([^&=]+)=([^&=]+))");
  std::smatch match;
  auto search_start = body_.cbegin();
  while (std::regex_search(search_start, body_.cend(), match, pattern)) {
    std::string value = std::move(match[2]), key = std::move(match[1]);
    size_t j = 0, n = value.size();
    for (size_t i = 0; i < n; ++i) {
      char c = value[i], tmp = c;
      if (c == '+') {
        tmp = ' ';
      } else if (c == '%') {
        tmp = conver_hex(value[i + 1]) * 16 + conver_hex(value[i + 2]);
        i += 2;
      }
      value[j++] = tmp;
    }
    value.erase(j);
    LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
    post_[key] = value;
  }
}

bool http_request::user_verify(const std::string &name, const std::string &pwd,
                               bool is_login) {
  LOG_INFO("verify name:%s pwd:%s", name.c_str(), pwd.c_str());
  MYSQL *sql;
  sql_RAII sql_r(&sql, sql_connpool::instance());

  char sql_order[256] = {0};
  MYSQL_RES *sql_res = nullptr;

  snprintf(sql_order, 256,
           "SELECT username, password FROM user WHERE username = '%s' LIMIT 1",
           name.c_str());
  LOG_DEBUG("%s", sql_order);

  if (mysql_query(sql, sql_order)) {
    // mysql_free_result(sql_res);
    return false;
  }

  sql_res = mysql_store_result(sql);

  MYSQL_ROW row = mysql_fetch_row(sql_res);
  std::string failed_str("undefine");
  bool flag = false;

  if (row) {
    LOG_DEBUG("Mysql row: %s %s", row[0], row[1]);
    std::string real_password(row[1]);
    if (is_login) {
      if (pwd == real_password) {
        flag = true;
      } else {
        failed_str = "password error";
        flag = false;
      }
    } else {
      failed_str = "user exist";
      flag = false;
    }
  } else if (is_login) {
    failed_str = "user doesn't exist";
    flag = false;
  } else {
    flag = true;
    LOG_DEBUG("user registering");
    snprintf(sql_order, 256,
             "INSERT INTO user(username, password) VALUES('%s', '%s')",
             name.c_str(), pwd.c_str());
    LOG_DEBUG("%s", sql_order);
    if (mysql_query(sql, sql_order)) {
      failed_str = "insert error";
      flag = false;
    }
  }

  if (flag) {
    LOG_DEBUG("user verify success");
  } else {
    LOG_DEBUG("user verify failed: %s", failed_str.c_str());
  }
  mysql_free_result(sql_res);
  return flag;
}