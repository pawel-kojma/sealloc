/* Misc utils */

#ifndef SEALLOC_UTILS_H_
#define SEALLOC_UTILS_H_

#define ALIGNUP_8(n) ((n + 7) & ~7)
#define ALIGNUP_16(n) ((n + 15) & ~15)
#define BITS2BYTES_CEIL(bits) (ALIGNUP_8(bits) / 8)

#define PAGE_SIZE 4096
#define IS_ALIGNED(X, Y) (((X) % (Y)) == 0)
#define SIZE_CLASS_ALIGNMENT 16
#define MEDIUM_CLASS_ALIGNMENT 1024
#define IS_SIZE_SMALL(size) (16 <= size && size <= 512)
#define IS_SIZE_MEDIUM(size) (512 < size && size <= 4096)
#define MAX_LARGE_SIZE 1048576  // 1MB
#define CONTAINER_OF(ptr, type, member) \
  ((type *)((char *)ptr - offsetof(type, member)))

#endif /* SEALLOC_UTILS_H_ */
