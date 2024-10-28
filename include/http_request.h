#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "buffer.h"

class http_request {
 public:
  enum class PARSE_STATE { REQUEST_LINE, HEADERS, BODY, FINISH };

  http_request() {}
  ~http_request() = default;

  void init();
  bool parse(buffer &buff);

  std::string path() const;
  std::string &path();
  std::string method() const;
  std::string version() const;
  std::string get_post(const std::string &key) const;
  std::string get_post(const char *key) const;

  bool is_keep_alive() const;

 private:
  bool parse_request_line_(const std::string &line);
  void parse_header_(const std::string &line);
  void parse_body_(const std::string &line);

  void parse_path_();
  void parse_post_();
  void parse_from_url_();

  static bool user_verify(const std::string &name, const std::string &pwd,
                          bool is_login);

  static int conver_hex(char ch);

  PARSE_STATE state_;
  std::string method_, path_, version_, body_;
  std::unordered_map<std::string, std::string> header_;
  std::unordered_map<std::string, std::string> post_;

  static const std::unordered_set<std::string> DEFAULT_HTML;
  static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif
