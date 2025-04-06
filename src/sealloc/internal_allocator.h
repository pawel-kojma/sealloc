/*!
 * @file internal_allocator.h
 * @brief Internal allocator used for storing metadata.
 *
 * Least size chunk is 16 bytes
 * Entire buffer to partition is 8MB
 */

#ifndef SEALLOC_INTERNAL_ALLOCATOR_H_
#define SEALLOC_INTERNAL_ALLOCATOR_H_

#include <stddef.h>
#include <stdint.h>

#include "container_ll.h"

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

/*!
 * @brief Hold information about particular allocation unit
 */
struct internal_allocator_state {
  ll_entry_t entry; /*!< Next allocation unit */
  size_t free_mem;  /*!< Non-contiguous amount of memory left in this unit */
  uint8_t
      buddy_tree[INTERNAL_ALLOC_BUDDY_TREE_SIZE_BYTES]; /*<! Segment tree state
                                                 for binary buddy algorithm */
  uint8_t memory[INTERNAL_ALLOC_CHUNK_SIZE_BYTES];      /*<! Memory on which
                                                           buddy_tree operates */
};
typedef struct internal_allocator_state int_alloc_t;

/*!
 * @brief Initializes an internal allocator state.
 *
 * @param[in,out] ia internal allocator state.
 * @warning Fails if cannot allocate space for structures.
 */
void internal_allocator_init(int_alloc_t *ia);

/*!
 * @brief Allocate memory for metadata.
 *
 * @param[in,out] ia internal allocator state. 
 * @param[in] size
 * @pre allocator must be initialized.
 */
void *internal_alloc(int_alloc_t *ia, size_t size);

/*!
 * @brief Free metadata
 *
 * @param[in,out] ia internal allocator state.
 * @param[in] ptr ptr to metadata to free.
 * @pre ptr must come from internal_alloc();
 */
void internal_free(int_alloc_t *ia, void *ptr);

#endif /* SEALLOC_INTERNAL_ALLOCATOR_H_ */
