/*!
 * @file chunk.h
 * @brief Chunk utils API for managing runs within them.
 *
 * On each run allocation, 2 guard pages are allocated right after the run to
 * prevent linear overflows between runs.
 */

#ifndef SEALLOC_CHUNK_H_
#define SEALLOC_CHUNK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "container_ll.h"
#include "utils.h"
struct run_state;

typedef struct run_state run_t;

/*!
 * @brief Size of chunk_node_t in bits
 */
#define NODE_STATE_BITS 2
/*!
 * @brief Each chunk is 32MB in size
 */
#define CHUNK_SIZE_BYTES 33554432

/*!
 * @brief Size of the leaf in a tree spans 4*PAGE_SIZE == 16384 bytes
 */
#define CHUNK_LEAST_REGION_SIZE_BYTES (PAGE_SIZE * 4)
#define CHUNK_NO_NODES_LAST_LAYER \
  (CHUNK_SIZE_BYTES / CHUNK_LEAST_REGION_SIZE_BYTES)
#define CHUNK_NO_NODES \
  (CHUNK_NO_NODES_LAST_LAYER + (CHUNK_NO_NODES_LAST_LAYER - 1))
#define CHUNK_BUDDY_TREE_SIZE_BITS (CHUNK_NO_NODES * NODE_STATE_BITS)
#define CHUNK_BUDDY_TREE_DEPTH 11
#define CHUNK_JUMP_NODE_SIZE_BYTES 4
#define CHUNK_JUMP_TREE_SIZE_BYTES (CHUNK_JUMP_NODE_SIZE_BYTES * CHUNK_NO_NODES)
/*!
 * @brief Arbitrary point where we unmap part of the chunk
 *
 * Right now we are unmapping at least 32 pages
 */
#define CHUNK_UNMAP_THRESHOLD 0

#define CHUNK_BUDDY_TREE_SIZE_BYTES \
  ((((CHUNK_BUDDY_TREE_SIZE_BITS) + 7) & ~7) / 8)

/*!
 * @brief Indicates that reg_size_small_medium field does not contain valid data
 */
#define REG_MARK_BAD_VALUE 65535

#define RANDOM_LOOKUP_TRESHOLD_PERCENTAGE 25
#define RANDOM_LOOKUP_TRIES 4

typedef struct jump_node {
  unsigned short prev;
  unsigned short next;
} jump_node_t;

/*!
 * @brief Describes chunk metadata.
 */
struct chunk_state {
  ll_entry_t
      entry; /*!< Linkage field that links together more chunk metadata. */
  unsigned short
      avail_nodes_count[CHUNK_BUDDY_TREE_DEPTH + 1]; /*!< i-th element tells how
                                                    many nodes are available for
                                                    allocation in i-th row of
                                                    the tree, if 0 then corresponding jump_tree_first_index is also 0*/
  uint16_t
      reg_size_small_medium[CHUNK_NO_NODES_LAST_LAYER]; /*!<  Stores (reg_size /
                                                           16) for small and
                                                           medium size class
                                                           ONLY */
  unsigned jump_tree_first_index[CHUNK_BUDDY_TREE_DEPTH +
                                 1]; /*!< Array of starting global indexes of
                                        free nodes in each level, 0 if level
                                        does not have any free nodes */
  jump_node_t
      jump_tree[CHUNK_NO_NODES]; /*!< Tree where each node has prev
              and next pointing at prev/next free element in the same row */
  uint8_t buddy_tree[CHUNK_BUDDY_TREE_SIZE_BYTES]; /*<! Segment tree state for
                                                      binary buddy algorithm */
};
typedef struct chunk_state chunk_t;

/*!
 * @brief Initializes an uninitialized chunk structure.
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @param[in] heap Pointer to memory mapping which chunk uses to allocate memory
 * for runs
 * @pre heap != NULL
 */
void chunk_init(chunk_t *chunk, void *heap);

/*!
 * @brief Tries to allocate run memory given run_size.
 *
 * reg_size is also passed to supply more information about the run when given a
 * run pointer.
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @param[in] run_size How much memory to allocate
 * @param[in] reg_size Additional run info to link with allocated run pointer
 * @return NULL if not enough memory, pointer to at least run_size bytes
 * otherwise
 * @pre chunk is initialized
 * @pre CHUNK_LEAST_REGION_SIZE_BYTES <= run_size <= CHUNK_SIZE_BYTES
 * @pre reg_size is aligned to size within its size class
 */
void *chunk_allocate_run(chunk_t *chunk, unsigned run_size, unsigned reg_size);

/*!
 * @brief Invalidates memory under run_ptr for particular run.
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @param[in] run_ptr Pointer to a valid run in a chunk.
 * @returns True if chunk was fully unmapped during this deallocation, false
 * otherwise
 * @pre chunk is initialized
 * @pre run_ptr is a valid pointer acquired from chunk_allocate_run
 */
bool chunk_deallocate_run(chunk_t *chunk, void *run_ptr);

/*!
 * @brief check if chunk is unmapped
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @return true if unmapped
 * @pre chunk is initialized
 */
bool chunk_is_unmapped(chunk_t *chunk);

/*!
 * @brief check if chunk is full
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @return true if chunk->free_mem == 0
 * @pre chunk is initialized
 */
bool chunk_is_full(chunk_t *chunk);

// TODO: improve buddy tree so that we can implement that efficiently
/* bool chunk_can_allocate_run(chunk_t *chunk, size_t run_size); */

/*!
 * @brief Get run information based on ptr inside some run
 *
 * Fills in run_ptr, run_size and reg_size based on ptr possibly inside some
 * run. If ptr does not correspond to any run, run_ptr is untouched.
 *
 * @param[in,out] chunk Pointer to the allocated chunk structure.
 * @param[in,out] ptr Target pointer to find run for.
 * @param[out] run_ptr Pointer to run memory.
 * @param[out] run_size Run size of found run.
 * @param[out] reg_size Reg size of found run. Aligned to size in its size
 * class.
 * @pre chunk is initialized
 * @pre *run_ptr is NULL
 * @pre *run_size is 0
 * @pre *reg_size is 0
 */
void chunk_get_run_ptr(chunk_t *chunk, void *ptr, void **run_ptr,
                       unsigned *run_size, unsigned *reg_size);

#endif /* SEALLOC_CHUNK_H_ */
