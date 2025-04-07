
#include <stdint.h>

static uint32_t state32;
static uint64_t state64;

void init_splitmix32(uint32_t seed) { state32 = seed; }
void init_splitmix64(uint64_t seed) { state64 = seed; }

/*
 * Source: https://github.com/bryc/code/blob/master/jshash/PRNGs.md
 */
uint32_t splitmix32(void) {
  uint32_t z = (state32 += 0x9e3779b9);
  z = (state32 ^ (state32 >> 16)) * 0x21f0aaad;
  z = (z ^ (z >> 15)) * 0x735a2d97;
  return z ^ (z >> 15);
}

/*
 * Source: https://rosettacode.org/wiki/Pseudo-random_numbers/Splitmix64
 */
uint64_t splitmix64(void) {
  state64 += 0x9e3779b97f4a7c15;
  uint64_t z = state64;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}
