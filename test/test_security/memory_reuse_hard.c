#include <stddef.h>
#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether the allocator reuses memory in the harder case.
 *
 * The test is performed as follows:
 * 1. Allocate NALLOCS chunks
 * 2. Free all previously allocated chunks.
 * 3. Allocate NALLOCS chunks again.
 * 4. Check if any returned pointer during the second allocation round appeared
 * in the first one.
 *
 * Test passes if each returned pointer is distinct.
 */

#define NALLOCS 3000

int cmp(const void *a, const void *b) {
  ptrdiff_t aa = *(ptrdiff_t *)a, bb = *(ptrdiff_t *)b;
  if (aa > bb)
    return 1;
  else if (aa < bb)
    return -1;
  return 0;
}

int main(void) {
  void *allocs[2 * NALLOCS];

  for (int i = 0; i < NALLOCS; i++) {
    allocs[i] = malloc(ALLOC_SIZE);
  }
  for (int i = 0; i < NALLOCS; i++) {
    free(allocs[i]);
  }
  for (int i = NALLOCS; i < 2 * NALLOCS; i++) {
    allocs[i] = malloc(ALLOC_SIZE);
  }
  qsort(allocs, 2 * NALLOCS, sizeof(void *), cmp);
  for (int i = 1; i < 2 * NALLOCS; i++) {
    if (allocs[i - 1] == allocs[i]) return FAIL;
  }
  return SUCCESS;
}
