#include <sealloc/arena.h>
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>

static arena_t main_arena;

__attribute__((constructor)) void init_heap(void) { arena_init(&main_arena); }

void *sealloc_malloc(size_t size) {
  void *ptr;
  size = ALIGNUP_16(size);
  if (size >= MAX_LARGE_SIZE) {
    // We have a huge allocation
    ptr = arena_allocate_huge_mapping(&main_arena, size);
    return ptr;
  }
  // Since size is at most large class, size_t will fit into uint16_t
  bin_t *bin = arena_get_bin_by_reg_size(&main_arena, size);
  run_t *run = bin->runcur;
  if (run != NULL) {
    ptr = run_allocate(run, bin);
    if (run_is_depleted(run)) {
      bin_retire_current_run(bin);
    }
    return ptr;
  }
  run = arena_allocate_run(&main_arena, bin);
  if (run == NULL) {
    // EOM
    return NULL;
  }
  bin->runcur = run;
  ptr = run_allocate(run, bin);
  if (run_is_depleted(run)) {
    bin_retire_current_run(bin);
  }
  return ptr;
}

void sealloc_free(void *ptr) {
  chunk_t *chunk;
  run_t *run;
  bin_t *bin;
  void *run_ptr = NULL;
  unsigned run_size = 0, reg_size = 0;
  if (ptr == NULL) return;
  chunk = arena_get_chunk_from_ptr(&main_arena, ptr);
  if (chunk == NULL) {
    // maybe a huge allocation
  }
  chunk_get_run_ptr(chunk, ptr, &run_ptr, &run_size, &reg_size);
  if (run_ptr == NULL) {
    // If run_ptr is still NULL, then supposed run is not registered in metadata
    se_error("Invalid call to free()");
  }
  if (reg_size > 0) {
    // small or medium class
    bin = arena_get_bin_by_reg_size(&main_arena, reg_size);
  } else {
    // large class
    bin = arena_get_bin_by_reg_size(&main_arena, run_size);
  }
  run = bin_get_run_by_addr(bin, run_ptr);

  // Might fail because invalid region is being freed
  run_deallocate(run, bin, ptr);

  if (run_is_freeable(run, bin)) {
    bin_delete_run(bin, run);
    if (chunk_deallocate_run(chunk, run->entry.key)) {
      arena_deallocate_chunk(&main_arena, chunk);
    }
    internal_free(run);
  }
}
