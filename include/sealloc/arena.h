/* arena utils API */

#include <sealloc/bin.h>
#include <sealloc/container_ll.h>
#include <sealloc/utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef SEALLOC_ARENA_H_
#define SEALLOC_ARENA_H_

struct chunk_state;
typedef struct chunk_state chunk_t;

// Each bin describes different size class
// Small - len(16, ..., 16*i, ..., 512) = 32
// Medium - len(1KB, 2KB, 4KB) = 3
// Large - len(8KB, ..., 8KB * (2 ** (i-1)), 1MB) = 8
#define ARENA_NO_SMALL_BINS 32
#define ARENA_NO_MEDIUM_BINS 3
#define ARENA_NO_LARGE_BINS 8
#define ARENA_NO_BINS \
  (ARENA_NO_SMALL_BINS + ARENA_NO_MEDIUM_BINS + ARENA_NO_LARGE_BINS)
#define CHUNKS_PER_MAPPING 4

typedef struct huge_chunk {
  ll_entry_t entry;
  size_t len;  // Unaligned length of the mapping
} huge_chunk_t;

typedef struct arena_state {
  uint32_t secret;       // PRNG seed
  ll_head_t chunk_list;  // Chunk pointers
  uintptr_t alloc_ptr;
  unsigned chunks_left;
  ll_head_t huge_mappings;  // Huge allocation pointers
  bin_t bins[ARENA_NO_BINS];
} arena_t;

void arena_init(arena_t *arena);
run_t *arena_allocate_run(arena_t *arena, bin_t *bin);
chunk_t *arena_allocate_chunk(arena_t *arena);
void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk);
chunk_t *arena_get_chunk_from_ptr(arena_t *arena, void *ptr);
bin_t *arena_get_bin_by_reg_size(arena_t *arena, uint16_t reg_size);
huge_chunk_t *arena_find_huge_mapping(arena_t *arena, void *huge_map);
void *arena_allocate_huge_mapping(arena_t *arena, size_t len);
void arena_deallocate_huge_mapping(arena_t *arena, void *map, size_t len);
void arena_store_huge_meta(arena_t *arena, huge_chunk_t *huge);
void arena_delete_huge_meta(arena_t *arena, huge_chunk_t *huge);
void arena_truncate_huge_mapping(arena_t *arena, huge_chunk_t *huge,
                                 unsigned len_pages);
#endif /* SEALLOC_ARENA_H_ */
