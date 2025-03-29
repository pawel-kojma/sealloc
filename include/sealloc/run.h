/* Run management API */

#include <sealloc/container_ll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifndef SEALLOC_RUN_H_
#define SEALLOC_RUN_H_

struct bin_state;

typedef struct bin_state bin_t;

typedef enum run_bitmap_state {
  STATE_FREE = 0,        // region is free
  STATE_ALLOC = 1,       // region is allocated
  STATE_ALLOC_FREE = 2,  // region is free but was allocated
} bstate_t;

typedef struct run_state {
  ll_entry_t entry;      // Contains run_heap ptr as key
  uint16_t navail;       // Number of remaining free regions
  uint16_t nfreed;       // Number of freed regions
  uint16_t gen;           // Generator
  uint16_t current_idx;   // Current index
  uint8_t reg_bitmap[];  // Region bitmap
} run_t;

// Allocate region from run
void *run_allocate(run_t *run, bin_t *bin);

// Deallocate region from run
void run_deallocate(run_t *run, bin_t *bin, void *ptr);

// Returns true if run is fully deallocated and can be collected
bool run_is_freeable(run_t *run, bin_t *bin);

// Returns false if run can still allocate, false otherwise
bool run_is_depleted(run_t *run);

// Initialize run
void run_init(run_t *run, bin_t *bin, void *heap);

#endif /* SEALLOC_RUN_H_ */
