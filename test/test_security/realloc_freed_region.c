#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether realloc reacts to a freed pointer being passed.
 *
 * The test is performed as follows:
 * 1. Allocate two chunks.
 * 2. free the first one.
 * 3. realloc the first one.
 * 4. Fail if realloc did not abort the program.
 */

int main(void) {
  // Allocate two chunks.
  void *a = malloc(ALLOC_SIZE);
  void *b = malloc(ALLOC_SIZE);

  // Free first chunk.
  free(a);

  // Reallocate freed pointer.
  void *c = realloc(a, 2 * ALLOC_SIZE);

  // If program did not abort at this point, detection failed.
  return FAIL;
}
