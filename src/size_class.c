#include "sealloc/size_class.h"

static inline unsigned ceil_div(unsigned a, unsigned b) {
  return (a / b) + (a % b == 0 ? 0 : 1);
}

static unsigned ctz(unsigned x) {
  unsigned cnt = 0;
  while (x > 1) {
    cnt++;
    x >>= 1;
  }
  return cnt;
}

static unsigned npow2(unsigned n) {
  // Compute next highest power of 2 (32-bit)
  // Source:
  // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}

unsigned alignup_large_size(unsigned n) {
  unsigned d = ceil_div(n, LARGE_SIZE_MIN_REGION);
  return npow2(d) * LARGE_SIZE_MIN_REGION;
}

unsigned size_to_idx_large(unsigned n) {
  return ctz(n / LARGE_SIZE_MIN_REGION);
}

unsigned alignup_medium_size(unsigned n) {
  unsigned d = ceil_div(n, MEDIUM_SIZE_MIN_REGION);
  return npow2(d) * MEDIUM_SIZE_MIN_REGION;
}

unsigned size_to_idx_medium(unsigned n) {
  return ctz(n / MEDIUM_SIZE_MIN_REGION);
}
