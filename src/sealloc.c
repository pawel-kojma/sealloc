#include <sealloc/arena.h>
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
//TODO: implement something to allocate run from some chunk in the arena
}
