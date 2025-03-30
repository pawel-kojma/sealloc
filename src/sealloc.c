#include <sealloc/arena.h>
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
#include <string.h>

typedef enum metadata_type {
  METADATA_INVALID,
  METADATA_REGULAR,
  METADATA_HUGE
} metadata_t;

// Global arena state
static arena_t main_arena;

__attribute__((constructor)) void init_heap(void) { arena_init(&main_arena); }

void *sealloc_malloc(size_t size) {
  void *ptr;
  se_debug("Allocating region of size %zu (aligned to %zu)", size,
           ALIGNUP_16(size));
  size = ALIGNUP_16(size);
  if (size >= MAX_LARGE_SIZE) {
    huge_chunk_t *huge;
    huge = internal_alloc(sizeof(huge_chunk_t));
    if (huge == NULL) return NULL;
    huge->len = ALIGN_PAGE(size);
    se_debug("Allocating huge chunk");
    arena_allocate_huge_mapping(&main_arena, huge);
    return huge->entry.key;
  }
  // Since size is at most large class, size_t will fit into uint16_t
  bin_t *bin = arena_get_bin_by_reg_size(&main_arena, size);
  run_t *run = bin->runcur;
  if (run != NULL) {
    se_debug("Allocating from bin for region sizes %u", bin->reg_size);
    ptr = run_allocate(run, bin);
    if (run_is_depleted(run)) {
      se_debug("Retiring run");
      bin_retire_current_run(bin);
    }
    return ptr;
  }
  se_debug("Allocating run for current request");
  run = arena_allocate_run(&main_arena, bin);
  if (run == NULL) {
    se_debug("End of memory");
    return NULL;
  }
  bin->runcur = run;
  se_debug("Allocating from fresh run");
  ptr = run_allocate(run, bin);
  if (run_is_depleted(run)) {
    se_debug("Retiring run");
    bin_retire_current_run(bin);
  }
  return ptr;
}

metadata_t locate_metadata_for_ptr(void *ptr, chunk_t **chunk_ret,
                                   run_t **run_ret, bin_t **bin_ret,
                                   huge_chunk_t **huge_ret) {
  chunk_t *chunk;
  void *run_ptr = NULL;
  unsigned run_size = 0, reg_size = 0;
  se_debug("Getting chunk from pointer");
  chunk = arena_get_chunk_from_ptr(&main_arena, ptr);
  if (chunk == NULL) {
    se_debug("No chunk found, checking huge allocations.");
    huge_chunk_t *huge_chunk;
    huge_chunk = arena_find_huge_mapping(&main_arena, ptr);
    if (huge_chunk == NULL) {
      return METADATA_INVALID;
    }
    *huge_ret = huge_chunk;
    return METADATA_HUGE;
  }
  *chunk_ret = chunk;
  se_debug("Found chunk metadata at %p", (void *)chunk);
  chunk_get_run_ptr(chunk, ptr, &run_ptr, &run_size, &reg_size);
  if (run_ptr == NULL) {
    return METADATA_INVALID;
  }
  if (reg_size > 0) {
    se_debug("Getting bin for small/medium class");
    *bin_ret = arena_get_bin_by_reg_size(&main_arena, reg_size);
  } else {
    se_debug("Getting bin for large class");
    *bin_ret = arena_get_bin_by_reg_size(&main_arena, run_size);
  }
  *run_ret = bin_get_run_by_addr(*bin_ret, run_ptr);
  se_debug("Found run medatada at %p (reg_size : %u, run_size_pages %u)",
           (void *)*run_ret, (*bin_ret)->reg_size, (*bin_ret)->run_size_pages);
  return METADATA_REGULAR;
}

void sealloc_free(void *ptr) {
  se_debug("Freeing a region at %p", ptr);
  chunk_t *chunk;
  run_t *run;
  bin_t *bin;
  huge_chunk_t *huge;
  metadata_t meta;
  if (ptr == NULL) return;
  meta = locate_metadata_for_ptr(ptr, &chunk, &run, &bin, &huge);
  if (meta == METADATA_INVALID) {
    se_error("Invalid call to free()");
  }
  if (meta == METADATA_HUGE) {
    arena_deallocate_huge_mapping(&main_arena, huge);
    internal_free(huge);
    return;
  }
  // Might fail because invalid region is being freed
  run_deallocate(run, bin, ptr);

  if (run_is_freeable(run, bin)) {
    se_debug("Run is freeable, deleting");
    bin_delete_run(bin, run);
    if (chunk_deallocate_run(chunk, run->entry.key)) {
      se_debug("Chunk is fully unmapped, deallocating chunk metadata");
      arena_deallocate_chunk(&main_arena, chunk);
    }
    internal_free(run);
  }
}

void *sealloc_realloc(void *ptr, size_t size) {
  se_debug("Reallocating a region at %p of size %zu", ptr, size);
  chunk_t *chunk;
  run_t *run;
  bin_t *bin;
  huge_chunk_t *huge;
  metadata_t meta;
  size_t size_palign = ALIGNUP_PAGE(size);
  if (ptr == NULL) return sealloc_malloc(size);
  meta = locate_metadata_for_ptr(ptr, &chunk, &run, &bin, &huge);
  if (meta == METADATA_INVALID) {
    se_error("Invalid call to realloc()");
  }
  if (meta == METADATA_HUGE) {
    se_debug("Reallocating huge chunk at %p", huge->entry.key);
    if (size > huge->len) {
      se_debug("Creating new mapping (new_size %zu > old_size %zu)", size,
               huge->len);
      void *dst = arena_allocate_huge_mapping(&main_arena, size_palign);
      memcpy(dst, huge->entry.key, huge->len);
      arena_delete_huge_meta(&main_arena, huge);
      huge->entry.key = dst;
      huge->len = size;
      arena_store_huge_meta(&main_arena, huge);
    } else if (size < huge->len) {
      unsigned truncate_pages = (ALIGNUP_PAGE(huge->len) - ALIGNUP_PAGE(size));
      se_debug(
          "Truncating existing mapping by %u pages (new_size %zu < old_size "
          "%zu)",
          truncate_pages, size, huge->len);
      arena_truncate_huge_mapping(&main_arena, huge);
    }
    return huge->entry.key;
  }
  se_debug("Reallocating region of small/medium/large class");
}
