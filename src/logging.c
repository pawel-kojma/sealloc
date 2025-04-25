#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "sealloc/utils.h"

#define NIL_STR "(nil)"
#define BUFSIZE 201

static const char* hex = "0123456789abcdef";

static size_t zu2str(size_t n, char buf[BUFSIZE], char** begin) {
  int idx = BUFSIZE - 1;
  if (n == 0) {
    buf[idx] = '0';
    *begin = &buf[idx];
    return BUFSIZE - idx;
  }
  while (idx > 0 && n > 0) {
    buf[idx] = ((n % 10) + '0');
    idx--;
    n /= 10;
  }
  *begin = &buf[idx + 1];
  return (BUFSIZE - (idx + 1));
}

static size_t u2str(unsigned n, char buf[BUFSIZE], char** begin) {
  return zu2str(n, buf, begin);
}

static size_t p2str(void* ptr, char buf[BUFSIZE], char** begin) {
  size_t n = (size_t)ptr;
  if (ptr == NULL) {
    memcpy(buf, NIL_STR, sizeof(NIL_STR) - 1);
    *begin = buf;
    return sizeof(NIL_STR) - 1;
  }
  if (n == 0) {
    memcpy(buf, "0x0", 3);
    *begin = buf;
    return 3;
  }
  int idx = BUFSIZE - 1;
  while (idx > 1 && n > 0) {
    buf[idx] = hex[n % 16];
    idx--;
    n /= 16;
  }
  idx -= 1;
  buf[idx] = '0';
  buf[idx + 1] = 'x';
  *begin = &buf[idx];
  return (BUFSIZE - idx);
}

void vfse_log(int fd, const char* msg, va_list args) {
  const char* begin = msg;
  char c, next, *buf_begin, *str_va;
  char buf[BUFSIZE];
  size_t len;
  if (msg == NULL) return;
  while ((c = *msg) != '\0') {
    if (c == '%') {
      write(fd, begin, (size_t)(msg - begin));
      next = msg[1];
      if (next == '%') {
        write(fd, "%", 1);
        msg += 2;
        begin = msg;
        continue;
      }
      switch (next) {
        case 'u':
          len = u2str(va_arg(args, unsigned), buf, &buf_begin);
          write(fd, buf_begin, len);
          msg += 2;
          begin = msg;
          break;
        case 'p':
          len = p2str(va_arg(args, void*), buf, &buf_begin);
          write(fd, buf_begin, len);
          msg += 2;
          begin = msg;
          break;
        case 'z':
          if (msg[2] == 'u') {
            len = zu2str(va_arg(args, size_t), buf, &buf_begin);
            write(fd, buf_begin, len);
            msg += 3;
            begin = msg;
          } else {
            msg += 2;
            begin = msg;
          }
          break;
        case 's':
          str_va = va_arg(args, char*);
          write(fd, str_va, msg_len(str_va));
          msg += 2;
          begin = msg;
          break;
        default:
          (void)va_arg(args, void*);
          msg++;
          break;
      }
    } else {
      msg++;
    }
  }
  if (begin < msg) {
    write(fd, begin, (size_t)(msg - begin));
  }
}

void se_log(const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  vfse_log(STDERR_FILENO, msg, args);
  va_end(args);
}

void fse_log(int fd, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  vfse_log(fd, msg, args);
  va_end(args);
}
