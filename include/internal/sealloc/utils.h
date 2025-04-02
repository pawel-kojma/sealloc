/* Misc utils */

#ifndef SEALLOC_UTILS_H_
#define SEALLOC_UTILS_H_

#define PAGE_SIZE 4096
#define ALIGNUP_8(n) ((n + 7) & ~7)
#define ALIGNUP_16(n) ((n + 15) & ~15)
#define ALIGNUP_PAGE(n) ((n + PAGE_SIZE) & ~PAGE_SIZE)
#define BITS2BYTES_CEIL(bits) (ALIGNUP_8(bits) / 8)

#define IS_ALIGNED(X, Y) (((X) % (Y)) == 0)
#define CONTAINER_OF(ptr, type, member) \
  ((type *)((char *)ptr - offsetof(type, member)))

#endif /* SEALLOC_UTILS_H_ */
