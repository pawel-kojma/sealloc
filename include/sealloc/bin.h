/* Bin management API */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct run_state;

typedef struct run_state run_t;

typedef struct bin_state {
    run_t *runcur;               // Current run servicing allocations
    run_t *run_list;             // List of active runs
    uint16_t reg_size;           // Size of single region
    uint16_t run_size_pages;     // Number of pages that each run spans
    uint32_t reg_mask_size_bits; // Number of bits in region bitmask
} bin_t;
