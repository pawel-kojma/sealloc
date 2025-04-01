/* Size class utilities */

#include <sealloc/utils.h>

#ifndef SEALLOC_SIZE_CLASS_H_
#define SEALLOC_SIZE_CLASS_H_

// Small - len(16, ..., 16*i, ..., 512) = 32
// Medium - len(1KB, 2KB, 4KB) = 3
// Large - len(8KB, ..., 8KB * (2 ** (i-1)), 1MB) = 8
#define NO_SMALL_SIZE_CLASSES 32
#define NO_MEDIUM_SIZE_CLASSES 3
#define NO_LARGE_SIZE_CLASSES 8
#define SMALL_SIZE_CLASS_ALIGNMENT 16
#define MEDIUM_SIZE_CLASS_ALIGNMENT 1024
#define SMALL_SIZE_MIN_REGION 16
#define SMALL_SIZE_MAX_REGION 512
#define MEDIUM_SIZE_MIN_REGION 1024
#define MEDIUM_SIZE_MAX_REGION PAGE_SIZE
#define LARGE_SIZE_MIN_REGION (2 * PAGE_SIZE)
#define LARGE_SIZE_MAX_REGION 1048576  // 1MB
#define IS_SIZE_SMALL(size) (1 <= (size) && (size) <= SMALL_SIZE_MAX_REGION)
#define IS_SIZE_MEDIUM(size) \
  (MEDIUM_SIZE_MIN_REGION <= (size) && (size) <= MEDIUM_SIZE_MAX_REGION)
#define IS_SIZE_LARGE(size) \
  (LARGE_SIZE_MIN_REGION <= (size) && (size) <= LARGE_SIZE_MAX_REGION)

#define ALIGNUP_SMALL_SIZE(n) \
  (((n) + (SMALL_SIZE_CLASS_ALIGNMENT - 1)) & ~(SMALL_SIZE_CLASS_ALIGNMENT - 1))
#define ALIGNUP_MEDIUM_SIZE(n)                 \
  (((n) + (MEDIUM_SIZE_CLASS_ALIGNMENT - 1)) & \
   ~(MEDIUM_SIZE_CLASS_ALIGNMENT - 1))

#define SIZE_TO_IDX_SMALL(n) (((n) / SMALL_SIZE_CLASS_ALIGNMENT) - 1)
#define SIZE_TO_IDX_MEDIUM(n) (((n) / MEDIUM_SIZE_CLASS_ALIGNMENT) - 1)

unsigned alignup_large_size(unsigned n);
unsigned size_to_idx_large(unsigned n);
#endif /* SEALLOC_SIZE_CLASS_H_ */
