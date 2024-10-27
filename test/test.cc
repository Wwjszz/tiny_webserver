#include <features.h>

#include "log.h"
#include "threadpool.hpp"

#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif

void Testlog() {
  int cnt = 0, level = 0;
  log::instance()->init(level, "./testlog1", ".log", 0);
  for (level = 3; level >= 0; level--) {
    log::instance()->set_level(level);
    for (int j = 0; j < 10000; j++) {
      for (int i = 0; i < 4; i++) {
        LOG_BASE(i, "%s 111111111 %d ============= ", "Test", cnt++);
      }
    }
  }
  cnt = 0;
  log::instance()->init(level, "./testlog2", ".log", 5000);
  for (level = 0; level < 4; level++) {
    log::instance()->set_level(level);
    for (int j = 0; j < 10000; j++) {
      for (int i = 0; i < 4; i++) {
        LOG_BASE(i, "%s 222222222 %d ============= ", "Test", cnt++);
      }
    }
  }
}

void ThreadlogTask(int i, int cnt) {
  for (int j = 0; j < 10000; j++) {
    LOG_BASE(i, "PID:[%04d]======= %05d ========= ", gettid(), cnt++);
  }
}

void TestThreadPool() {
  log::instance()->init(0, "./testThreadpool", ".log", 5000);
  threadpool<8> threadpool;
  for (int i = 0; i < 18; i++) {
    threadpool.add_task(std::bind(ThreadlogTask, i % 4, i * 10000));
  }
  getchar();
}

int main() {
  // Testlog();
  TestThreadPool();
}