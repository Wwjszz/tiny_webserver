#include "http_response.h"

#include <fcntl.h>

#include "log.h"

void http_response::init(const std::string& dir, const std::string& path,
                         bool is_keep_alive, int code) {
  if (mm_file) {
    unmap_file();
  }
  dir_ = dir;
  path_ = path;
  is_keep_alive_ = is_keep_alive;
  code_ = code;
  mm_file_ = nullptr;
  mm_file_stat_ = {0};
}

void http_response::unmap_file() {
  if (mm_file) {
    munmap(mm_file_, mm_file_stat_.st_size);
    mm_file_ = nullptr;
  }
}

std::string http_response::file_type_() {
  std::string::size_type pos = path_.find_last_of('.');
  if (pos != std::string::npos) {
    std::string suffix = path_.substr(pos);
    auto iter = SUFFIX_TYPE_.find(suffix);
    if (iter != SUFFIX_TYPE_.end()) {
      return iter->second;
    }
  }
failed:
  return "text/plain";
}

void http_response::error_html() {
  if (CODE_PATH_.contains(code_)) {
    path_ = CODE_PATH_.at(code_);
    stat((dir_ + path_).c_str(), &mm_file_stat_);
  }
}

void http_response::error_content(buffer& buff, std::string message) {
  std::string body, status;
  body += "<html><title>Error</title>";
  body += "<body bgcolor=\"ffffff\">";
  if (CODE_STATUS_.contains(code_)) {
    status = CODE_STATUS_.at(code_);
  } else {
    status = CODE_STATUS_.at(400);
  }
  body += std::to_string(code_) + " : " + status + "\n";
  body += "<p>" + message + "</p>";
  body += "<hr><em>TinyWebserver</em></body></html>";

  buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
  buff.append(body);
}

void http_response::add_response_status_line_(buffer& buff) {
  std::string status;
  if (CODE_STATUS_.contains(code_)) {
    status = CODE_STATUS_.at(code_);
  } else {
    status = CODE_STATUS_.at(400);
  }
  buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void http_response::add_response_header_(buffer& buff) {
  buff.append("Connection: ");
  if (is_keep_alive_) {
    buff.append("keep-alive\r\n");
    buff.append("keep-alive: max=6, timeout=120\r\n");
  } else {
    buff.append("close\r\n");
  }
  buff.append("Content-type: " + file_type_() + "\r\n");
}

void http_response::add_response_content_(buffer& buff) {
  int src_fd = open((dir_ + path_).c_str(), O_RDONLY);
  if (src_fd == -1) {
    error_content(buff, "Not found");
    return;
  }

  LOG_DEBUG("file path: %s", (dir_ + path_).c_str());
  void* mm_src =
      mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
  if (*static_cast<int*>(mm_src) == -1) {
    error_content(buff, "mmap error");
    return;
  }

  mm_file_ = (char*)mm_src;
  close(src_fd);
  buff.append("Content-length: " + std::to_string(mm_file_stat_.st_size) +
              "\r\n\r\n");
}

void http_response::make_response(buffer& buff) {
  if (stat((dir_ + path_).c_str(), &mm_file_stat_) < 0 ||
      S_ISDIR(mm_file_stat_.st_mode)) {
    code_ = 404;
  } else if (!(mm_file_stat_.st_mode & S_IROTH)) {
    code_ = 403;
  } else {
    code_ = 200;
  }
  error_html();
  add_response_status_line_(buff);
  add_response_header_(buff);
  add_response_content_(buff);
}