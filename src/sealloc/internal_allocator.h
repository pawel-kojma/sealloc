/* Internal allocator used for storing metadata */

#ifndef SEALLOC_INTERNAL_ALLOCATOR_H_
#define SEALLOC_INTERNAL_ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>
/*
 * Least size chunk is 16 bytes
 * Entire buffer to partition is 8MB
 * Number of nodes in binary tree = (8MB / 16B) + (8MB / 16B) - 1
 * Each node takes 2 bits
 */

#define INTERNAL_ALLOC_CHUNK_SIZE_BYTES 8388608  // 2^23 B = 8MB
#define INTERNAL_ALLOC_LEAST_REGION_SIZE_BYTES 16
#define INTERNAL_ALLOC_NO_NODES_LAST_LAYER \
  (INTERNAL_ALLOC_CHUNK_SIZE_BYTES / INTERNAL_ALLOC_LEAST_REGION_SIZE_BYTES)
#define INTERNAL_ALLOC_NO_NODES         \
  (INTERNAL_ALLOC_NO_NODES_LAST_LAYER + \
   (INTERNAL_ALLOC_NO_NODES_LAST_LAYER - 1))
#define INTERNAL_ALLOC_BUDDY_TREE_SIZE_BITS (INTERNAL_ALLOC_NO_NODES * 2)
#define INTERNAL_ALLOC_BUDDY_TREE_SIZE_BYTES \
  (((INTERNAL_ALLOC_BUDDY_TREE_SIZE_BITS + 7) & ~7) / 8)

struct internal_allocator_data {
  struct internal_allocator_data *fd;
  struct internal_allocator_data *bk;
  size_t free_mem;
  uint8_t buddy_tree[INTERNAL_ALLOC_BUDDY_TREE_SIZE_BYTES];
  uint8_t memory[INTERNAL_ALLOC_CHUNK_SIZE_BYTES];
};

/* Initialize allocator */
int internal_allocator_init(void);

/* allocate chunk for metadata */
void *internal_alloc(size_t);

/* free chunk */
void internal_free(void *);

#endif /* SEALLOC_INTERNAL_ALLOCATOR_H_ */
