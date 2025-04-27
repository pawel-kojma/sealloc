#include <stdlib.h>

#include "common.h"

#define NALLOCS 2000

/*
 * This test tests if the allocator can detect double free in the harder case.
 *
 * Here we allocate NALLOCS chunks and free all of them.
 * Then, a chunk in the middle is freed once again.
 *
 * This is to test if the allocator can spot double free regardless of how many
 * free() calls were made in between.
 */

int main(void) {
  void *chunks[NALLOCS];
  for (int i = 0; i < NALLOCS; i++) {
    chunks[i] = malloc(ALLOC_SIZE);
  }
  for (int i = 0; i < NALLOCS; i++) {
    free(chunks[i]);
  }

  // Perform the second free
  free(chunks[NALLOCS / 2]);

  // If we got here, it means the allocator failed to detect the double free
  return FAIL;
}
