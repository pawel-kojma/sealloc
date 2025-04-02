/* Bin management API */

#include <sealloc/container_ll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef SEALLOC_BIN_H_
#define SEALLOC_BIN_H_

struct run_state;

typedef struct run_state run_t;

typedef struct bin_state {
  ll_head_t run_list_inactive;  // List of inactive runs
  unsigned
      run_list_inactive_cnt;    // Number of runs currently in the inactive list
  run_t *runcur;                // Current active run servicing allocations
  unsigned reg_size;            // Size of single region
  unsigned run_size_pages;      // Number of pages that each run spans
  unsigned reg_mask_size_bits;  // Number of bits in region bitmask
} bin_t;

// Initialize bin
void bin_init(bin_t *bin, unsigned reg_size);

// Delete inactive run
void bin_delete_run(bin_t *bin, run_t *run);

// Delete current run by putting on inactive list
void bin_retire_current_run(bin_t *bin);

// Get run's metadata by its address
run_t *bin_get_run_by_addr(bin_t *bin, void *run_ptr);

#endif /* SEALLOC_BIN_H_ */
