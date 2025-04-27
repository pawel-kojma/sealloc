#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether the allocator reuses memory in the harder case.
 *
 */

int main(void) {
  void *a = malloc(ALLOC_SIZE);
  free(a);
  void *b = malloc(ALLOC_SIZE);
  // Check if pointers are the same
  if (a != b) return SUCCESS;
  return FAIL;
}
