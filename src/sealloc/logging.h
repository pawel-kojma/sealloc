/* Sealloc logging library */

#ifndef SEALLOC_LOGGING_H_
#define SEALLOC_LOGGING_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#define pr_fmt "%s:%u (%s) ", __FILE__, __LINE__, __func__
#ifdef LOGGING
#define se_debug(...)          \
  {                            \
    se_log("[DEBUG] " pr_fmt); \
    se_log(__VA_ARGS__);       \
    se_log("\n");              \
  }
#else
#define se_debug(...) \
  {                   \
  }
#endif

#define se_error(...)          \
  {                            \
    se_log("[ERROR] " pr_fmt); \
    se_log(__VA_ARGS__);       \
    se_log("\n");              \
    exit(1);                   \
  }
#define se_error_with_errno(...) \
  {                              \
    se_log("[ERROR] " pr_fmt);   \
    se_log(__VA_ARGS__);         \
    se_log("\n");                \
    exit(errno);                 \
  }

void se_log(const char* msg, ...);
#endif /* SEALLOC_LOGGING_H_ */
