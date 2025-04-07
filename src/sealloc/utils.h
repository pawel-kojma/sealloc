/* Misc utils */

#ifndef SEALLOC_UTILS_H_
#define SEALLOC_UTILS_H_

#include <stdint.h>

#define USERSPACE_ADDRESS_MASK 0x7fffffffffffULL
#define PAGE_SIZE 4096
#define PAGE_MASK 0xfffULL
#define ALIGNUP_8(n) ((n + 7) & ~7)
#define ALIGNUP_16(n) ((n + 15) & ~15)
#define ALIGNUP_PAGE(n) ((n + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))
#define BITS2BYTES_CEIL(bits) (ALIGNUP_8(bits) / 8)

#define IS_ALIGNED(X, Y) (((X) % (Y)) == 0)
#define CONTAINER_OF(ptr, type, member) \
  ((type *)((char *)ptr - offsetof(type, member)))

uint32_t str2u32(const char *str);
unsigned msg_len(const char* msg);
#endif /* SEALLOC_UTILS_H_ */
