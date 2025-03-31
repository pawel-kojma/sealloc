#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/container_ll.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/platform_api.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
#include <string.h>

void arena_init(arena_t *arena) {
  platform_status_code_t code;
  if ((code = platform_get_random(&arena->secret)) != PLATFORM_STATUS_OK) {
    se_error("Failed to get random value: %s", platform_strerror(code));
  }
  init_splitmix32(arena->secret);
  internal_allocator_init();
  ll_init(&arena->chunk_list);
  ll_init(&arena->huge_mappings);
  arena->alloc_ptr = 0;
  arena->chunks_left = 0;
  memset(arena->bins, 0, sizeof(bin_t) * ARENA_NO_BINS);
}

run_t *arena_allocate_run(arena_t *arena, bin_t *bin) {
  chunk_t *chunk;
  run_t *run;
  void *run_ptr;
  for (ll_entry_t *entry = arena->chunk_list.ll; entry != NULL;
       entry = entry->link.fd) {
    chunk = CONTAINER_OF(entry, chunk_t, entry);
    run_ptr = chunk_allocate_run(chunk, bin->run_size_pages * PAGE_SIZE,
                                 bin->reg_size);
    if (run_ptr != NULL) {
      // We just allocated a run
      run = internal_alloc(sizeof(run_t));
      if (run == NULL) {
        // EOM
        return NULL;
      }
      run_init(run, bin, run_ptr);
      return run;
    }
  }
  // No luck finding, allocate a new one
  // Will also be addded to chunk list
  chunk = arena_allocate_chunk(arena);
  if (chunk == NULL) {
    // EOM
    return NULL;
  }
  run_ptr =
      chunk_allocate_run(chunk, bin->run_size_pages * PAGE_SIZE, bin->reg_size);
  if (run_ptr == NULL) {
    se_error("Failed to allocate run from fresh chunk");
  }
  run = internal_alloc(sizeof(run_t));
  if (run == NULL) {
    // EOM
    return NULL;
  }
  run_init(run, bin, run_ptr);
  return run;
}

chunk_t *arena_allocate_chunk(arena_t *arena) {
  chunk_t *chunk_meta = internal_alloc(sizeof(chunk_t));
  platform_status_code_t code;
  size_t map_len = CHUNKS_PER_MAPPING * (CHUNK_SIZE_BYTES + PAGE_SIZE);
  if (chunk_meta == NULL) {
    // End of memory;
    return NULL;
  }

  // get more memory if needed
  if (arena->alloc_ptr == 0 || arena->chunks_left == 0) {
    if ((code = platform_map(NULL, map_len, (void **)&arena->alloc_ptr)) !=
        PLATFORM_STATUS_OK) {
      se_error("Failed to allocate mapping (size : %u): %s", map_len,
               platform_strerror(code));
    }
    arena->chunks_left = CHUNKS_PER_MAPPING;
  }

  // Guard one page after the end of chunk
  if ((code = platform_guard((void *)(arena->alloc_ptr + CHUNK_SIZE_BYTES),
                             PAGE_SIZE)) != PLATFORM_STATUS_OK) {
    se_error("Failed to allocate mapping (size : %u): %s", PAGE_SIZE,
             platform_strerror(code));
  }
  chunk_init(chunk_meta, (void *)arena->alloc_ptr);
  ll_add(&arena->chunk_list, &chunk_meta->entry);
  arena->alloc_ptr += (CHUNK_SIZE_BYTES + PAGE_SIZE);
  arena->chunks_left--;
  return chunk_meta;
}

void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk) {
  platform_status_code_t code;

  // Unmap guard page
  if ((code = platform_unmap(
           (void *)((uintptr_t)chunk->entry.key + CHUNK_SIZE_BYTES),
           PAGE_SIZE)) != PLATFORM_STATUS_OK) {
    se_error("Failed to unmap mapping (size : %u): %s", PAGE_SIZE,
             platform_strerror(code));
  }
  // Delete from chunk list
  ll_del(&arena->chunk_list, &chunk->entry);

  // Free metadata
  internal_free(chunk);
}

chunk_t *arena_get_chunk_from_ptr(arena_t *arena, void *ptr) {
  // ptr is some pointer inside of a chunk
  chunk_t *chunk = NULL;
  ptrdiff_t target = (ptrdiff_t)ptr;
  for (ll_entry_t *entry = arena->chunk_list.ll; entry != NULL;
       entry = entry->link.fd) {
    se_debug("Trying entry entry=%p, entry->key=%p", (void *)entry, entry->key);
    if ((ptrdiff_t)entry->key <= target &&
        target < (ptrdiff_t)entry->key + CHUNK_SIZE_BYTES) {
      chunk = CONTAINER_OF(entry, chunk_t, entry);
      break;
    }
  }
  return chunk;
}

bin_t *arena_get_bin_by_reg_size(arena_t *arena, uint16_t reg_size) {
  unsigned skip_bins = 0;
  bin_t *bin = NULL;
  // Assume reg_size is either small, medium or large class
  if (IS_SIZE_SMALL(reg_size)) {
    bin = &arena->bins[SIZE_TO_IDX_SMALL(reg_size)];
  } else if (IS_SIZE_MEDIUM(reg_size)) {
    skip_bins = NO_SMALL_SIZE_CLASSES;
    bin = &arena->bins[skip_bins + SIZE_TO_IDX_MEDIUM(reg_size)];
  } else {
    skip_bins += NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES;
    bin = &arena->bins[skip_bins + size_to_idx_large(reg_size)];
  }
  if (bin->reg_size == 0) {
    bin_init(bin, reg_size);
  }
  return bin;
}

huge_chunk_t *arena_find_huge_mapping(arena_t *arena, void *huge_map) {
  return CONTAINER_OF(ll_find(&arena->huge_mappings, huge_map), huge_chunk_t,
                      entry);
}

void arena_store_huge_meta(arena_t *arena, huge_chunk_t *huge) {
  ll_add(&arena->huge_mappings, &huge->entry);
}

void arena_delete_huge_meta(arena_t *arena, huge_chunk_t *huge) {
  ll_del(&arena->huge_mappings, &huge->entry);
}

void *arena_allocate_huge_mapping(arena_t *arena, size_t len) {
  platform_status_code_t code;
  void *huge_map;
  if ((code = platform_map(NULL, len, (void **)&huge_map)) !=
      PLATFORM_STATUS_OK) {
    se_error("Failed to allocate huge mapping (size : %u): %s", len,
             platform_strerror(code));
  }
  return huge_map;
}
void arena_deallocate_huge_mapping(arena_t *arena, void *map, size_t len) {
  platform_status_code_t code;
  if ((code = platform_unmap(map, len)) != PLATFORM_STATUS_OK) {
    se_error("Failed to deallocate huge mapping (ptr : %p, size : %u): %s", map,
             len, platform_strerror(code));
  }
}

void arena_truncate_huge_mapping(arena_t *arena, huge_chunk_t *huge,
                                 unsigned trunc_len_aligned) {
  platform_status_code_t code;
  size_t cur_size = ALIGNUP_PAGE(huge->len);
  void *map =
      (void *)((uintptr_t)huge->entry.key + (cur_size - trunc_len_aligned));
  if ((code = platform_unmap(map, trunc_len_aligned)) != PLATFORM_STATUS_OK) {
    se_error(
        "Failed to truncate huge mapping (ptr : %p, size : %u, truncate : %u): "
        "%s",
        huge->entry.key, huge->len, trunc_len_aligned, platform_strerror(code));
  }
}
