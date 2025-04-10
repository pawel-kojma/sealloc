/*!
 * @file bin.h
 * @brief Bin utils for manipulating bins and runs that it aggregates.
 *
 * Bins holds information which describes runs and what type of regions they
 * allocate.
 */

#ifndef SEALLOC_BIN_H_
#define SEALLOC_BIN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "container_ll.h"

struct run_state;

typedef struct run_state run_t;

/*!
 * @brief Holds metadata of a bin.
 */
struct bin_state {
  ll_head_t run_list_inactive; /*!< List of runs that no longer can service
                                  allocations */
  ll_head_t
      run_list_active; /*!< List of runs currently servicing allocations */
  unsigned run_list_active_cnt; /*!< Number of active runs */
  unsigned avail_regs;     /*!< Number of available regions for allocation */
  unsigned reg_size;       /*!< Size of single region */
  unsigned run_size_pages; /*!< Number of pages that each run spans */
  unsigned reg_mask_size_bits; /*!< Number of bits in region bitmask */
};
typedef struct bin_state bin_t;

/*!
 * @brief Initializes an uninitialized bin structure.
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in] reg_size Region size form which entire bin content is derived.
 * @pre reg_size is aligned in its size class
 * @pre 1<= reg_size <= LARGE_SIZE_MAX_REGION
 */
void bin_init(bin_t *bin, unsigned reg_size);

/*!
 * @brief Selects run from which next allocation will take place
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @pre bin is initialized
 * @post bin structure is updated
 * @warning Allocation must take place from specified run after calling this
 * function
 */
run_t *bin_get_run_for_allocation(bin_t *bin);

/*!
 * @brief Adds fresh run to the bin
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in,out] run Run pointer structure inside this bin.
 * @pre bin is initialized
 * @pre run is initialized and fresh
 */
void bin_add_run(bin_t *bin, run_t *run);

/*!
 * @brief Deletes inactive run from inactive list.
 *
 * Does not delete run metadata.
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in,out] run Run pointer structure inside this bin.
 * @pre bin is initialized
 * @pre run is initialized
 * @pre run must be on the inactive list
 */
void bin_delete_run(bin_t *bin, run_t *run);

/*!
 * @brief Put run on inactive list.
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @pre bin is initialized
 * @pre run is initialized
 * @pre run is on the list of active runs
 * @pre run cannot service more allocations 
 */
void bin_retire_run(bin_t *bin, run_t *run);

/*!
 * @brief Finds run metadata
 *
 * Searches active and inactive lists to find run metadata.
 *
 * @param[in,out] bin Pointer to the allocated bin structure.
 * @param[in] run_ptr Pointer that possibly matches run metadata.
 * @return NULL if run_ptr doesn't match anything, run metadata otherwise.
 * @pre bin is initialized
 */
run_t *bin_get_run_by_addr(bin_t *bin, const void *run_ptr);

#endif /* SEALLOC_BIN_H_ */
