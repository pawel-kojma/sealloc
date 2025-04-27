#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * This test tests whether the allocator will detect that overflow happened and
 * abort the program.
 *
 * This is the harder case because no allocator call is made after overflow
 * happens. In order to pass this test, the allocator must prepare the
 * environment somehow during the malloc() call.
 */

int main(void) {
  // Allocate the chunk
  void *a = malloc(ALLOC_SIZE);

  // Overflow the chunk, writing twice as much data
  memset(a, 0x42, 2 * ALLOC_SIZE);

  // If program did not abort at this point, detection failed
  return FAIL;
}
