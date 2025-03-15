/* Bin management API */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct run_state;

typedef struct run_state run_t;

typedef struct bin_state {
  run_t *run_list_active;        // List of runs with active regions
  run_t *run_list_inactive;      // List of dead runs
  uint32_t run_list_active_cnt;  // Number of runs currently in the active list
  uint32_t run_list_inactive_cnt;    // Number of runs currently in the dead list
  uint16_t reg_size;             // Size of single region
  uint16_t run_size_pages;       // Number of pages that each run spans
  uint32_t reg_mask_size_bits;   // Number of bits in region bitmask
} bin_t;

// Initialize bin
void bin_init(bin_t *bin, size_t reg_size);

// Add run to bin
void bin_add_fresh_run(bin_t *bin, run_t *run);

// Delete run from bin
void bin_del_dead_run(bin_t *bin, run_t *run);

// Get run for allocation
run_t *bin_get_non_full_run(bin_t *bin);

// Get run metadate by address
run_t *bin_get_run_by_addr(bin_t *bin, void *run_ptr);
