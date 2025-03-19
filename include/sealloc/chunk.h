/* chunk utils API */

#include <sealloc/container_ll.h>
#include <sealloc/utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef _CHUNK_H_
#define _CHUNK_H_

struct run_state;

typedef struct run_state run_t;

#define CHUNK_SIZE_BYTES 4194304  // 2^22 B = 4MB
#define CHUNK_LEAST_REGION_SIZE_BYTES (PAGE_SIZE * 2)
#define CHUNK_NO_NODES_LAST_LAYER \
  (CHUNK_SIZE_BYTES / CHUNK_LEAST_REGION_SIZE_BYTES)
#define CHUNK_NO_NODES \
  (CHUNK_NO_NODES_LAST_LAYER + (CHUNK_NO_NODES_LAST_LAYER - 1))
#define CHUNK_BUDDY_TREE_SIZE_BITS (CHUNK_NO_NODES * 2)

#define CHUNK_BUDDY_TREE_SIZE_BYTES \
  ((((CHUNK_BUDDY_TREE_SIZE_BITS) + 7) & ~7) / 8)

typedef struct chunk_state {
  ll_entry_t entry;
  size_t free_mem;
  uint8_t
      reg_size_small_medium[CHUNK_NO_NODES_LAST_LAYER];  // Stores reg_size //
                                                         // 16 for small and
                                                         // medium size class
  uint8_t buddy_tree[CHUNK_BUDDY_TREE_SIZE_BYTES];
} chunk_t;

void *chunk_allocate_run(chunk_t *chunk, size_t run_size, size_t reg_size);
void *chunk_deallocate_run(chunk_t *chunk, run_t *run);
bool chunk_is_empty(chunk_t *chunk);
bool chunk_is_full(chunk_t *chunk);
void chunk_init(chunk_t *chunk, void *heap);
// TODO: improve buddy tree so that we can implement that efficiently
/* bool chunk_can_allocate_run(chunk_t *chunk, size_t run_size); */
void chunk_get_run_ptr(chunk_t *chunk, void *ptr, void **run_ptr,
                       size_t *run_size, size_t *reg_size);

#endif /* _CHUNK_H_ */
