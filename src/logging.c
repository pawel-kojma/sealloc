#include <unistd.h>

unsigned msg_len(const char* msg) {
  unsigned cnt = 0;
  while (*msg != '\0') {
    cnt++;
    msg++;
  }
  return cnt;
}

void logE(const char* msg) {
  unsigned len = msg_len(msg);
  if (len == 0) return;
  write(STDERR_FILENO, msg, msg_len(msg));
  write(STDERR_FILENO, "\n", 1);
}
