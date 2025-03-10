/*
 * Source: https://github.com/bryc/code/blob/master/jshash/PRNGs.md
 */

#include <stdint.h>

static uint64_t state;

void init_splitmix32(uint64_t seed) { state = seed; }

uint32_t splitmix32(void) {
  uint32_t z = (state += 0x9e3779b9);
  z = (state ^ (state >> 16)) * 0x21f0aaad;
  z = (z ^ (z >> 15)) * 0x735a2d97;
  return z ^ (z >> 15);
}
