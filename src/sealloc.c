#include <sealloc/arena.h>
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>

// Global arena state
static arena_t main_arena;

__attribute__((constructor)) void init_heap(void) { arena_init(&main_arena); }

void *sealloc_malloc(size_t size) {
  void *ptr;
  se_debug("Allocating region of size %zu (aligned to %zu)", size,
           ALIGNUP_16(size));
  size = ALIGNUP_16(size);
  if (size >= MAX_LARGE_SIZE) {
    se_debug("Allocating huge chunk");
    ptr = arena_allocate_huge_mapping(&main_arena, size);
    return ptr;
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
    // EOM
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

void sealloc_free(void *ptr) {
  se_debug("Freeing a region at %p", ptr);
  chunk_t *chunk;
  run_t *run;
  bin_t *bin;
  void *run_ptr = NULL;
  unsigned run_size = 0, reg_size = 0;
  if (ptr == NULL) return;
  se_debug("Getting chunk from pointer");
  chunk = arena_get_chunk_from_ptr(&main_arena, ptr);
  if (chunk == NULL) {
    se_debug("No chunk found, checking huge allocations.");
    huge_chunk_t *huge_chunk;
    huge_chunk = arena_find_huge_mapping(&main_arena, ptr);
    if (huge_chunk == NULL) {
      se_error("Invalid call to free()");
    }
    se_debug("Freeing huge chunk at %p", (void *)huge_chunk);
    arena_deallocate_huge_mapping(&main_arena, huge_chunk->entry.key);
    internal_free(huge_chunk);
  }

  se_debug("Found chunk metadata at %p", (void *)chunk);
  chunk_get_run_ptr(chunk, ptr, &run_ptr, &run_size, &reg_size);
  if (run_ptr == NULL) {
    // If run_ptr is still NULL, then supposed run is not registered in metadata
    se_error("Invalid call to free()");
  }
  if (reg_size > 0) {
    se_debug("Getting bin for small/medium class");
    bin = arena_get_bin_by_reg_size(&main_arena, reg_size);
  } else {
    se_debug("Getting bin for large class");
    bin = arena_get_bin_by_reg_size(&main_arena, run_size);
  }
  run = bin_get_run_by_addr(bin, run_ptr);
  se_debug("Found run medatada at %p (reg_size : %u, run_size_pages %u)",
           (void *)run, bin->reg_size, bin->run_size_pages);
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
