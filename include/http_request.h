#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>

class http_request {
 public:
  enum class PARSE_STATE { REQUEST_LINE, HEADERS, BODY, FINISH };

  void init();

 private:
  PARSE_STATE state_;
  std::unordered_map<std::string, std::string> header_;
  std::unordered_map<std::string, std::string> post_;

  static const std::unordered_set<std::string> DEFAULT_HTML;
  static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif
