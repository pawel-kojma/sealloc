#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * This test is similar to overflow prevention test, but uses free to
 * try to force the allocator into reusing the freed memory.
 *
 * The test is performed as follows:
 * 1. Allocate NALLOCS chunks
 * 2. Free every second chunk to make holes in between
 * 3. Allocate target chunk
 * 4. Perform overflow on every chunk that was not freed before
 * 5. Check if data in target chunk is still intact
 */

#define NALLOCS 300

struct asd {
  unsigned a;
  unsigned b;
};

int main(void) {
  void *spray_chunks[NALLOCS];

  // Spray the heap
  for (int i = 0; i < NALLOCS; i++) {
    spray_chunks[i] = malloc(ALLOC_SIZE);
  }

  // Make holes for application data
  for (int i = 1; i < NALLOCS; i += 2) {
    free(spray_chunks[i]);
  }

  // Allocate target chunk
  struct asd *application_data = malloc(ALLOC_SIZE);

  // Application places some data we want to overwrite
  application_data->a = UCOOKIE;

  // Try to overflow the data
  for (int i = 0; i < NALLOCS; i += 2) {
    memset(spray_chunks[i], 0, 2 * ALLOC_SIZE);
  }

  // If data is still intact no impactful overflow happened
  if (application_data->a == UCOOKIE) return SUCCESS;

  return FAIL;
}
