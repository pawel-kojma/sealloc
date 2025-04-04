/*!
 * @file arena.h
 * @brief Arena utils API for manipulating arena structures
 *
 * Arenas are the top-level structures that organizes chunks, bins and huge
 * allocations.
 */

#ifndef SEALLOC_ARENA_H_
#define SEALLOC_ARENA_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bin.h"
#include "container_ll.h"
#include "size_class.h"
#include "utils.h"

struct chunk_state;
typedef struct chunk_state chunk_t;

/*!
 * @brief Number of bins arena holds.
 */
#define ARENA_NO_BINS \
  (NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES + NO_LARGE_SIZE_CLASSES)

/*!
 * @brief Indicate how many chunks will be placed inside one mapping.
 */
#define CHUNKS_PER_MAPPING 4

/*!
 * @brief Holds metadata of huge allocations.
 */
struct huge_chunk {
  ll_entry_t entry; /*!< Linkage field that links together more huge allocations
                       metadata. */
  size_t len;       /*!< Size of allocation, ALIGNED to PAGE_SIZE */
};
typedef struct huge_chunk huge_chunk_t;

/*!
 * @brief Holds state of arena
 */
struct arena_state {
  int is_initialized;   /*!< Holds 1 if arena was initialized, 0 otherwise. */
  uint32_t secret;      /*!< 32-bit PRNG seed used to randomize allocation of
                           structures or      user allocations. */
  ll_head_t chunk_list; /*!< Head to list of linkage entries within chunk_t
                           structures. */
  uintptr_t alloc_ptr;  /*!< If not NULL, a pointer to place within a mapping
                           from where next chunk will be allocated. If alloc_ptr
                           is NULL, then chunks_left is 0. */
  unsigned chunks_left; /*!< Indicate how many chunks are left in current
                           mapping servicing chunks. If chunks_left is 0, then
                           alloc_ptr is NULL. */
  ll_head_t huge_mappings;   /*!< Head to list of linkage entries within
                                huge_chunk_t structures. */
  bin_t bins[ARENA_NO_BINS]; /*!< Array of bin_t structures for SMALL, MEDIUM or
                                LARGE size classes. */
};
typedef struct arena_state arena_t;

/*!
 * @brief Initializes an uninitialized arena structure.
 *
 * @param[in,out] arena Pointer to the allocated arena structure.
 * @sideeffect Terminates if random value for seed couldn't be acquired.
 */
void arena_init(arena_t *arena);

/*!
 * @brief Allocates a run within some chunk assigned to arena.
 *
 * It uses bin metadata to allocate correct amount of memory for run metadata.
 * It allocates a chunk if one is needed.
 *
 * @param[in,out] arena Pointer to the allocated arena structure.
 * @param[in,out] bin Pointer to the allocated arena structure.
 * @return NULL if EOM or pointer to initialized run metadata for specified bin.
 * @pre arena is initialized
 * @pre bin is initialized
 */
run_t *arena_allocate_run(arena_t *arena, bin_t *bin);

/*!
 * @brief Allocates a chunk and assigns it to arena.
 *
 * It allocates chunk metadata and mapping and stores it in arena.
 *
 * @param[in,out] arena Pointer to the allocated arena structure
 * @return NULL if EOM or pointer to initialized chunk metadata.
 * @pre arena is initialized
 * @sideeffect fails if chunk could not be allocated
 */
chunk_t *arena_allocate_chunk(arena_t *arena);

/*!
 * @brief Deallocates a chunk
 *
 * It deallocates chunk metadata and a guard page after the chunk mapping.
 * Info about chunk in arena is deleted.
 * It assumes that the chunk is fully unmapped.
 *
 * @param[in,out] arena Pointer to the allocated arena structure
 * @param[in,out] chunk Pointer to the fully unmapped chunk metadata.
 * @pre arena is initialized
 * @pre chunk is fully unmapped
 * @sideeffect fails if guard page could not be unmapped
 */
void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk);

/*!
 * @brief Finds chunk metadata.
 *
 * Searches through chunks assigned to arena and returns one where ptr matches
 * chunk's mapping.
 *
 * It deallocates chunk metadata and a guard page after the chunk mapping.
 * It assumes that the chunk is fully unmapped.
 *
 * @param[in] arena Pointer to the allocated arena structure
 * @param[in] ptr Pointer to possibly a chunk in this arena
 * @return Pointer to chunk metadata which corresponds to ptr or NULL if no
 * chunk was found
 * @pre arena is initialized
 */
chunk_t *arena_get_chunk_from_ptr(const arena_t *arena, const void *const ptr);

/*!
 * @brief Returns bin for reg_size
 *
 * It deallocates chunk metadata and a guard page after the chunk mapping.
 * It assumes that the chunk is fully unmapped.
 *
 * @param[in, out] arena Pointer to the allocated arena structure
 * @param[in] reg_size Region size which identifies which bin to return
 * @return Pointer to bin metadata for reg_size
 * @pre 1 <= reg_size <= LARGE_SIZE_MAX_REGION
 * @pre reg_size is initialied to size in its class
 * @pre arena is initialized
 */
bin_t *arena_get_bin_by_reg_size(arena_t *arena, unsigned reg_size);

/*!
 * @brief Finds huge allocation
 *
 * Searches through arena huge allocations and returns huge allocation metadata
 * if one was found.
 *
 * @param[in] arena Pointer to the allocated arena structure
 * @param[in] huge_map Pointer to possibly a huge allocation assigned to this
 * arena
 * @return Pointer to huge allocation metadata.
 * @pre arena is initialized
 */
huge_chunk_t *arena_find_huge_mapping(const arena_t *arena,
                                      const void *const huge_map);

/*!
 * @brief Allocates huge allocation without allocating metadata.
 *
 * @param[in, out] arena Pointer to the allocated arena structure
 * @param[in] len size of requested allocation, page aligned.
 * @return Pointer to huge allocation mapping.
 * @pre len is page aligned
 * @pre arena is initialized
 * @sideeffect fails if mapping could not be allocated
 */
void *arena_allocate_huge_mapping(arena_t *arena, size_t len);

/*!
 * @brief Deallocates huge allocation without deallocating metadata.
 *
 * @param[in, out] arena Pointer to the allocated arena structure
 * @param[in] huge_map Pointer to valid huge mapping.
 * @param[in] len size of requested allocation, page aligned.
 * @pre len is page aligned
 * @pre arena is initialized
 * @sideeffect fails if mapping could not be deallocated 
 */
void arena_deallocate_huge_mapping(arena_t *arena, void *const huge_map,
                                   size_t len);

/*!
 * @brief Stores huge allocation metadata in arena.
 *
 * Does not deallocate metadata itself.
 *
 * @param[in, out] arena Pointer to the allocated arena structure
 * @param[in, out] huge Pointer to valid huge mapping.
 * @pre huge is initialized
 * @pre arena is initialized
 */
void arena_store_huge_meta(arena_t *arena, huge_chunk_t *huge);

/*!
 * @brief Deletes huge allocation info from arena.
 *
 * Does not deallocate metadata itself.
 *
 * @param[in, out] arena Pointer to the allocated arena structure
 * @param[in, out] huge_map Pointer to valid huge mapping.
 * @pre huge is initialized
 * @pre arena is initialized
 */
void arena_delete_huge_meta(arena_t *arena, huge_chunk_t *huge);
#endif /* SEALLOC_ARENA_H_ */
