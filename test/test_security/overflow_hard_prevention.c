#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * This test tests whether the allocator can prevent data corruption in the
 * more complicated case.
 *
 * In this case 300 chunks are requested.
 *
 * We request out data chunk at the end and place cookie value inside it.
 * Then, those 300 chunks are used to overflow into the subsequent chunks.
 *
 * The reason for allocating many chunks is that we want to increase
 * the odds of allocating data chunk after the one being overflowed,
 * because the heap layout may be randomized.
 *
 * To visualize, our heap may look like the one below after allocation of 300
 * chunks. (N in A_N means chunk allocation number)
 *
 * **************************
 * *           A_1          *
 * **************************
 * *                        *
 * **************************
 * *           A_5          *
 * **************************
 * *                        * -
 * ************************** |
 * *           ...          * | many non-allocated chunks
 * ************************** |
 * *                        * -
 * **************************
 * *         A_299          *
 * **************************
 * *         A_187          *
 * **************************
 * *         A_14           *
 * **************************
 *
 * After that data chunk B is requested.
 *
 * If it is placed after at least one of the chunks being overflowed, we will be
 * able to corrupt the data.
 */

#define CHUNKS 300

struct data {
  unsigned cookie;
};

int main(void) {
  void *A[CHUNKS];

  // Allocate chunks used for overflow
  for (int i = 0; i < CHUNKS; i++) {
    A[i] = malloc(ALLOC_SIZE);
  }

  // Allocate 1 data chunk
  struct data *B = malloc(ALLOC_SIZE);

  // Place the cookie in the first word
  B->cookie = UCOOKIE;

  // Perform the overflow on each chunk A
  for (int i = 0; i < CHUNKS; i++) {
    memset(A[i], 0, 4 * ALLOC_SIZE);
  }

  // If the data is not corrupted, success
  if (B->cookie == UCOOKIE) return SUCCESS;

  // Data is corrupted, fail
  return FAIL;
}
