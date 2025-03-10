/* Sealloc logging library */

#ifndef LOGGING_H_
#define LOGGING_H_

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef LOGGING
#define pr_fmt "%s:%d (%s) ", __FILE__, __LINE__, __func__
#define se_debug(...)          \
  {                            \
    printf("[DEBUG] " pr_fmt); \
    printf(__VA_ARGS__);       \
    putchar('\n');             \
  }
#define se_error(...)          \
  {                            \
    printf("[ERROR] " pr_fmt); \
    printf(__VA_ARGS__);       \
    putchar('\n');             \
    exit(1);                   \
  }
#define se_error_with_errno(...) \
  {                              \
    printf("[ERROR] " pr_fmt);   \
    printf(__VA_ARGS__);         \
    putchar('\n');               \
    exit(errno);                 \
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

#endif
