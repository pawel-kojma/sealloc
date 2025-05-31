/*!
 * @file run.h
 * @brief Run utils API for manipulating run structures
 *
 * Runs are the bottom-level structures that store single user allocations.
 */

#ifndef SEALLOC_RUN_H_
#define SEALLOC_RUN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "container_ll.h"
#include "utils.h"

struct bin_state;

/*!
 * @brief Size of run for allocations of small size.
 */
#define RUN_SIZE_SMALL_BYTES 16384
/*!
 * @brief Size of run for allocations of medium size.
 */
#define RUN_SIZE_MEDIUM_BYTES 16384
#define RUN_SIZE_SMALL_PAGES (RUN_SIZE_SMALL_BYTES / PAGE_SIZE)
#define RUN_SIZE_MEDIUM_PAGES (RUN_SIZE_MEDIUM_BYTES / PAGE_SIZE)

typedef struct bin_state bin_t;

/*!
 * @brief Region statuses in a bitmap.
 */
typedef enum run_bitmap_state {
  STATE_FREE = 0,        /*!< Region is free */
  STATE_ALLOC = 1,       /*!< Region is allocated */
  STATE_ALLOC_FREE = 2,  /*!< Region is free but was allocated */
} bstate_t;

/*!
 * @brief Holds state of run.
 *
 * Run structure uses random generators of group Z_n with addition to
 * allocate regions randomly.
 */
typedef struct run_state {
  ll_entry_t entry;      /*!< Contains run_heap ptr as key */
  uint16_t navail;       /*!< Number of remaining free regions */
  uint16_t nfreed;       /*!< Number of freed regions */
  uint16_t gen;          /*!< (Z_n, +) Group generator */
  uint16_t current_idx;  /*!< Current index */
  uint8_t reg_bitmap[];  /*!< Region bitmap */
} run_t;

/*!
 * @brief Allocate region from run.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @return Pointer to region. 
 * @pre run is initialized
 * @pre bin is initialized
 */
void *run_allocate(run_t *run, bin_t *bin);

/*!
 * @brief Validate if ptr points to allocated region.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in] ptr Pointer to validate.
 * @return Bitmap index or SIZE_MAX if validation failed.
 * @pre run is initialized
 * @pre bin is initialized
 */
size_t run_validate_ptr(run_t *run, bin_t *bin, void *ptr);

/*!
 * @brief Deallocate region from run.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in] ptr Pointer to region.
 * @return false if ptr was freed. 
 * @pre run is initialized
 * @pre bin is initialized
 */
bool run_deallocate(run_t *run, bin_t *bin, void *ptr);

/*!
 * @brief Check if run metadata can be deallocated.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @return true if run metadata can be deallocated.
 * @pre run is initialized
 * @pre bin is initialized
 */
bool run_is_freeable(run_t *run, bin_t *bin);

/*!
 * @brief Check if run has any regions left to allocate.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @return false if run still has free region left.
 * @pre run is initialized
 */
bool run_is_depleted(run_t *run);

/*!
 * @brief Initialize run.
 *
 * @param[in,out] run Pointer to the allocated run structure.
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in,out] heap Pointer to heap region that the run will manage. 
 * @pre bin is initialized
 */
void run_init(run_t *run, bin_t *bin, void *heap);

#endif /* SEALLOC_RUN_H_ */
