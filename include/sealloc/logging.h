/* Sealloc logging library */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef SEALLOC_LOGGING_H_
#define SEALLOC_LOGGING_H_

#ifdef LOGGING
#define pr_fmt "%s:%d (%s) ", __FILE__, __LINE__, __func__
#define se_debug(...)                   \
  {                                     \
    fprintf(stderr, "[DEBUG] " pr_fmt); \
    fprintf(stderr, __VA_ARGS__);       \
    fprintf(stderr, "\n");              \
  }
#define se_error(...)                   \
  {                                     \
    fprintf(stderr, "[ERROR] " pr_fmt); \
    fprintf(stderr, __VA_ARGS__);       \
    fprintf(stderr, "\n");              \
    exit(1);                            \
  }
#define se_error_with_errno(...)        \
  {                                     \
    fprintf(stderr, "[ERROR] " pr_fmt); \
    fprintf(stderr, __VA_ARGS__);       \
    fprintf(stderr, "\n");              \
    exit(errno);                        \
  }
#else
#define se_debug(...) \
  {                   \
  }
#define se_error(...) \
  {                   \
    exit(1);          \
  }
#define se_error_with_errno(...) \
  {                              \
    exit(errno);                 \
  }
#endif

#endif /* SEALLOC_LOGGING_H_ */
