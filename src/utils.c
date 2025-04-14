#include <stdint.h>

unsigned msg_len(const char* msg) {
  unsigned cnt = 0;
  while (*msg != '\0') {
    cnt++;
    msg++;
  }
  return cnt;
}

unsigned ctz(unsigned x) {
  unsigned cnt = 0;
  while (x > 1) {
    cnt++;
    x >>= 1;
  }
  return cnt;
}

uint32_t str2u32(const char* str) {
  uint32_t res = 0, base = 1, idx = msg_len(str);
  while (idx > 0) {
    res += (str[idx - 1] - '0') * base;
    base *= 10;
    idx--;
  }
  return res;
}
