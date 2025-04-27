#include <stdlib.h>

#include "common.h"

/*
 * This test tests if the allocator can detect double free in the simple case.
 *
 * We allocate two chunks, A and B.
 * We free them in order A -> B -> A
 *
 * Chunk B is freed in between to make it a harder for allocators, that use
 * simple free list, for storing freed chunks.
 */

int main(void) {
  void *a = malloc(ALLOC_SIZE);
  void *b = malloc(ALLOC_SIZE);
  free(a);
  free(b);
  free(a);

  // If we got here, it means the allocator failed to detect second free on
  // chunk A
  return FAIL;
}
