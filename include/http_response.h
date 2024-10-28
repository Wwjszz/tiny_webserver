#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <sys/mman.h>
#include <sys/stat.h>

#include <string>
#include <unordered_map>

#include "buffer.h"

class http_response {
 public:
  http_response() = default;
  ~http_response() { unmap_file(); }

  void init(const std::string &dir, const std::string &path,
            bool is_keep_alive = false, int code = -1);
  void make_response(buffer &buff);
  char *mm_file() const { return mm_file_; }
  size_t mm_file_len() const { return mm_file_stat_.st_size; }
  int code() const { return code_; }

 private:
  void unmap_file();

  void add_response_status_line_(buffer &buff);
  void add_response_header_(buffer &buff);
  void add_response_content_(buffer &buff);

  void error_content(buffer &buff, std::string message);
  void error_html();

  std::string file_type_();

  std::string path_;
  std::string dir_;
  const std::string real_path() { return path_ + dir_; }

  int code_ = -1;
  bool is_keep_alive_;

  char *mm_file_;
  struct stat mm_file_stat_;

  static const std::unordered_map<std::string, std::string> SUFFIX_TYPE_;
  static const std::unordered_map<int, std::string> CODE_STATUS_;
  static const std::unordered_map<int, std::string> CODE_PATH_;

  // static const std::string CRLF;
};

#endif