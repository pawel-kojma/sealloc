/* Run management API */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct bin_state;

typedef struct bin_state bin_t;

typedef struct run_state {
  struct run_state *fd;
  struct run_state *bk;
  void *run_heap;        // Run pointer on user heap
  uint16_t nfree;        // Number of remaining free regions
  uint8_t gen;           // Generator
  uint8_t current_idx;   // Current index
  uint8_t reg_bitmap[];  // Region bitmap
} run_t;

// Allocate region from run
void *run_allocate(run_t *run, bin_t *bin);

// Deallocate region from run
void *run_deallocate(run_t *run, bin_t *bin);

// Validate if ptr is freeable
void run_validate_freeable(run_t *run, bin_t *bin, void *ptr);

// Check if run is empty
bool run_is_empty(run_t *run, bin_t *bin);

// Check if run is full
bool run_is_full(run_t *run, bin_t *bin);

// Initialize run
int run_init(run_t *run, bin_t *bin, uint64_t secret, void* heap);
