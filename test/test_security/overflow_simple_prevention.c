#include <stdlib.h>
#include <string.h>

#include "common.h"

/*
 * This test tests whether the allocator can prevent data corruption in the
 * simple case.
 *
 * In this case two chunks, of the same size, A and B are requested.
 * We place a cookie value in the first field of B.
 * It's integrity will be checked after the overflow.
 *
 * We corrupt the data using chunk A, by writing 4 times the amount of data it
 * can hold.
 *
 * The allocator may store metadata between chunks, so we account for it by
 * overwriting more.
 *
 * If the heap layout is not randomized at all, A and B will be next to each
 * other on the heap.
 *
 * Example heap:
 *
 * **************************
 * *           A            *
 * **************************
 * *           B            *
 * **************************
 * *     not allocated      *
 * **************************
 *
 * In this case the test should fail as cookie value will be corrupted by the
 * overflow.
 */

struct data {
  unsigned cookie;
};

int main(void) {
  void *A = malloc(ALLOC_SIZE);
  struct data *B = malloc(ALLOC_SIZE);

  // Place the cookie in the first word
  B->cookie = UCOOKIE;

  // Perform the overflow using A
  memset(A, 0, 4 * ALLOC_SIZE);

  // If the data is not corrupted, success
  if (B->cookie == UCOOKIE) return SUCCESS;

  // Data is corrupted, fail
  return FAIL;
}
