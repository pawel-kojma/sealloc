#include <unistd.h>
#include "sealloc/utils.h"

void logE(const char* msg) {
  unsigned len = msg_len(msg);
  if (len == 0) return;
  write(STDERR_FILENO, msg, msg_len(msg));
  write(STDERR_FILENO, "\n", 1);
}
