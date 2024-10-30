#include "webserver.h"

int main() {
  webserver(1316, 3, 10000, false, 3306, "root", "1", "webserver", 12, 6, true,
            1, 1024)
      .start();
  return 0;
}