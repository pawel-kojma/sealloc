#include <sealloc/size_class.h>

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

unsigned alignup_large_size(unsigned n) {
  unsigned d = ceil_div(n, LARGE_SIZE_MIN_REGION);

  // Compute next highest power of 2 (32-bit)
  // Source:
  // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
  d--;
  d |= d >> 1;
  d |= d >> 2;
  d |= d >> 4;
  d |= d >> 8;
  d |= d >> 16;
  d++;
  return d * LARGE_SIZE_MIN_REGION;
}

unsigned size_to_idx_large(unsigned n) {
  return ctz(n / LARGE_SIZE_MIN_REGION);
}
