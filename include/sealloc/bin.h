/* Bin management API */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct run_state;

typedef struct run_state run_t;

typedef struct bin_state {
  run_t *runcur;             // Current active run servicing allocations
  run_t *run_list_inactive;  // List of dead runs
  uint32_t
      run_list_inactive_cnt;    // Number of runs currently in the inactive list
  uint16_t reg_size;            // Size of single region
  uint16_t run_size_pages;      // Number of pages that each run spans
  uint32_t reg_mask_size_bits;  // Number of bits in region bitmask
} bin_t;

// Initialize bin
void bin_init(bin_t *bin, uint16_t reg_size);

// Put depleted run
void bin_del_dead_run(bin_t *bin, run_t *run);

// Put run on inactive list
void bin_retire_run(bin_t *bin, run_t *run);

// Get run's metadata by its address
run_t *bin_get_run_by_addr(bin_t *bin, void *run_ptr);
