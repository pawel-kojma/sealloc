#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether realloc reacts to a freed pointer being passed.
 *
 * The test is performed as follows:
 * 1. Allocate one chunk.
 * 2. Reallocate the chunk with the same size.
 * 3. Succeed if different pointer was returned.
 */

int main(void) {
  // Allocate two chunks.
  char *a = malloc(ALLOC_SIZE);

  // Reallocate freed pointer, with the same size.
  char *c = realloc(a, ALLOC_SIZE);

  // Return success if different pointer was returned.
  if (a != c)
    return SUCCESS;

  // Pointers are the same, test failed.
  return FAIL;
}
