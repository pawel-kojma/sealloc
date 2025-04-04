/* Size class utilities */

#ifndef SEALLOC_SIZE_CLASS_H_
#define SEALLOC_SIZE_CLASS_H_

#include "utils.h"
#include <stdbool.h>

// Small - len(16, ..., 16*i, ..., 512) = 32
// Medium - len(1KB, 2KB, 4KB) = 3
// Large - len(8KB, ..., 8KB * (2 ** (i-1)), 1MB) = 8
#define NO_SMALL_SIZE_CLASSES 32
#define NO_MEDIUM_SIZE_CLASSES 3
#define NO_LARGE_SIZE_CLASSES 8
#define SMALL_SIZE_CLASS_ALIGNMENT 16
#define SMALL_SIZE_MIN_REGION 16
#define SMALL_SIZE_MAX_REGION 512
#define MEDIUM_SIZE_MIN_REGION 1024
#define MEDIUM_SIZE_MAX_REGION PAGE_SIZE
#define LARGE_SIZE_MIN_REGION (2 * PAGE_SIZE)
#define LARGE_SIZE_MAX_REGION 1048576  // 1MB
#define IS_SIZE_SMALL(size) (1 <= (size) && (size) <= SMALL_SIZE_MAX_REGION)
#define IS_SIZE_MEDIUM(size) \
  (SMALL_SIZE_MAX_REGION < (size) && (size) <= MEDIUM_SIZE_MAX_REGION)
#define IS_SIZE_LARGE(size) \
  (MEDIUM_SIZE_MAX_REGION < (size) && (size) <= LARGE_SIZE_MAX_REGION)
#define IS_SIZE_HUGE(size) ((size) > LARGE_SIZE_MAX_REGION)
#define ALIGNUP_SMALL_SIZE(n) \
  (((n) + (SMALL_SIZE_CLASS_ALIGNMENT - 1)) & ~(SMALL_SIZE_CLASS_ALIGNMENT - 1))

#define SIZE_TO_IDX_SMALL(n) (((n) / SMALL_SIZE_CLASS_ALIGNMENT) - 1)

unsigned alignup_large_size(unsigned n);
unsigned size_to_idx_large(unsigned n);

unsigned alignup_medium_size(unsigned n);
unsigned size_to_idx_medium(unsigned n);
bool is_size_aligned(unsigned size);
#endif /* SEALLOC_SIZE_CLASS_H_ */
