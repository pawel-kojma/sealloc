#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * This test tests whether the allocator will detect that overflow happened and
 * abort the program.
 *
 * This is the simpler case because we free the chunk afterwards and the
 * allocator may react during the call.
 */

int main(void) {
  // Allocate the chunk
  void *a = malloc(ALLOC_SIZE);

  // Overflow the chunk, writing twice as much data
  memset(a, 0x42, 2 * ALLOC_SIZE);

  // Let the allocator react
  free(a);

  // If program did not abort at this point, detection failed
  return FAIL;
}
