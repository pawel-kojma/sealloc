/* Bin management API */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct run_state;

typedef struct run_state run_t;

typedef struct bin_state {
    run_t *runcur;
    run_t *run_list;
    uint32_t reg_size;
    uint32_t run_size;
    uint32_t reg_mask_size;
} bin_t;
