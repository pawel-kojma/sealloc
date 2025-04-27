#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether the allocator has randomized heap layout feature.
 *
 * The test is performed as follows:
 * 1. Allocate NALLOCS chunks.
 * 2. Check if the returned addresses are sorted.
 * 3. Check if space gap between each allocation is constant.
 * 4. Tests succeds when (2) or (3) is not satisfied.
 */

#define NALLOCS 100

int is_sorted(volatile uintptr_t allocs[NALLOCS]) {
  for (int i = 1; i < NALLOCS; i++) {
    if (allocs[i - 1] >= allocs[i]) return SUCCESS;
  }
  return FAIL;
}

ptrdiff_t gap(uintptr_t ptr1, uintptr_t ptr2) { return ptr2 - ptr1; }
int is_gap_constant(volatile uintptr_t allocs[NALLOCS]) {
  ptrdiff_t initial_gap = gap(allocs[0], allocs[1]);
  for (int i = 2; i < NALLOCS; i++) {
    if (initial_gap != gap(allocs[i - 1], allocs[i])) return SUCCESS;
  }
  return FAIL;
}

int main(void) {
  volatile uintptr_t allocs[NALLOCS];
  for (int i = 0; i < NALLOCS; i++) {
    allocs[i] = (uintptr_t)malloc(ALLOC_SIZE);
  }

  return is_sorted(allocs) || is_gap_constant(allocs);
}
