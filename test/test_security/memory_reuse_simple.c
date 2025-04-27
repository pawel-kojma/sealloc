#include <stdlib.h>

#include "common.h"

/*
 * This test tests whether the allocator reuses memory in the simplest case.
 *
 * Two allocations, A and B, are requested. A is freed before B is allocated.
 * If allocator does not implement any delayed reuse mechanisms, B will be
 * allocated in place of A.
 *
 * Layout in case memory is reused:
 *
 * **************************      **************************      **************************
 * *           A            *      *           FREE         *      *           B            *
 * ************************** ---> ************************** ---> **************************
 * *         FREE           *      *           FREE         *      *          FREE          *
 * **************************      **************************      ************************** 
 *
 * Otherwise, B will be allocated somewhere else and test will pass.
 */

int main(void) {
  void *a = malloc(ALLOC_SIZE);
  free(a);
  void *b = malloc(ALLOC_SIZE);
  // Check if pointers are the same
  if (a != b) return SUCCESS;
  return FAIL;
}
