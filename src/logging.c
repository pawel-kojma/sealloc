#include <sealloc/logging.h>
#include <string.h>
#include <unistd.h>

static const char *log_info_prefix = "[INFO]";

void sealloc_log(const char *fmt) {
#ifdef LOG_INFO
  write(STDOUT_FILENO, log_info_prefix, strlen(log_info_prefix));
  write(STDOUT_FILENO, " ", 1);
  write(STDOUT_FILENO, fmt, strlen(fmt));
  write(STDOUT_FILENO, '\n', 1);
#endif
}
