/* Run management API */

#ifndef SEALLOC_RUN_H_
#define SEALLOC_RUN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "container_ll.h"
#include "utils.h"

struct bin_state;

#define RUN_SIZE_SMALL_BYTES 16384
#define RUN_SIZE_MEDIUM_BYTES 16384
#define RUN_SIZE_SMALL_PAGES (RUN_SIZE_SMALL_BYTES / PAGE_SIZE)
#define RUN_SIZE_MEDIUM_PAGES (RUN_SIZE_MEDIUM_BYTES / PAGE_SIZE)

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
  uint16_t gen;          // Generator
  uint16_t current_idx;  // Current index
  uint8_t reg_bitmap[];  // Region bitmap
} run_t;

// Allocate region from run
void *run_allocate(run_t *run, bin_t *bin);

// Validate if ptr points to allocated region
size_t run_validate_ptr(run_t *run, bin_t *bin, void *ptr);

// Deallocate region from run
bool run_deallocate(run_t *run, bin_t *bin, void *ptr);

// Returns true if run is fully deallocated and can be collected
bool run_is_freeable(run_t *run, bin_t *bin);

// Returns false if run can still allocate, false otherwise
bool run_is_depleted(run_t *run);

// Initialize run
void run_init(run_t *run, bin_t *bin, void *heap);

#endif /* SEALLOC_RUN_H_ */
