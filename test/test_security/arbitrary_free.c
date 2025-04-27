#include <stdlib.h>

#include "common.h"

/*
 * This test tests if the allocator can detect free on address that was not
 * returned by the allocator.
 */

int main(void) {
  char *a = (char *)malloc(ALLOC_SIZE);

  // Try to free and invalid region
  free(a + ALLOC_SIZE);

  // Failed to detect
  return FAIL;
}
