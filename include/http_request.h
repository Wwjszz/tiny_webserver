#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

class http_request {
 public:
  enum class PARSE_STATE { REQUEST_LINE, HEADERS, BODY, FINISH };
};

#endif
