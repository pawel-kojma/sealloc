/* arena utils API */

#include <sealloc/bin.h>
#include <sealloc/utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct chunk_state;
typedef struct chunk_state chunk_t;

// Each bin describes different size class
// Small - len(16, ..., 16*i, ..., 512) = 32
// Medium - len(1KB, 2KB, 4KB) = 3
// Large - len(8KB, ..., 4KB * (2 ** i), 1MB) = 8
#define ARENA_NO_BINS 43

typedef struct arena_state {
  size_t secret;
  chunk_t *chunk_list;
  bin_t bins[ARENA_NO_BINS];
  // TODO: huge allocations
} arena_t;

void arena_init(arena_t *arena);
chunk_t *arena_allocate_chunk(arena_t *arena);
void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk);
chunk_t *arena_get_chunk_from_ptr(arena_t *arena, void *ptr);
/*
 * arena_find_huge_mapping(arena_t *arena, void *chunk_ptr);
 * arena_create_huge_mapping(arena_t *arena, size_t huge_size);
 * arena_delete_huge_mapping(arena_t *arena, void *huge);
 */
