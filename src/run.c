#include <sealloc/bin.h>
#include <sealloc/generator.h>
#include <sealloc/logging.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
#include <stdbool.h>
#include <string.h>

// Get state of a region from bitmap
static bstate_t get_bitmap_item(uint8_t *mem, size_t idx) {
  size_t word = idx / 4;
  size_t off = idx % 4;
  return (bstate_t)(mem[word] >> (2 * off)) & 3;
}

// Set region state inside a bitmap
static void set_bitmap_item(uint8_t *mem, size_t idx, bstate_t state) {
  size_t word = idx / 4;
  size_t off = idx % 4;
  uint8_t erase_bits = 3;  // 0b11
  // First, clear the bits we want to set in mem[word]
  // Second, set two state bits in their place
  mem[word] =
      (mem[word] & ~(erase_bits << (off * 2))) | ((uint8_t)state << (off * 2));
}

void run_init(run_t *run, bin_t *bin, void *heap) {
  unsigned gen_idx;
  run->run_heap = heap;
  run->nfree = bin->reg_mask_size_bits / 2;

  // Choose a generator
  if (IS_SIZE_SMALL(bin->reg_size)) {
    gen_idx = bin->reg_size / 16 - 1;
    run->gen = GENERATORS_SMALL[gen_idx][splitmix32() %
                                         GENERATORS_SMALL_LENGTHS[gen_idx]];
  } else if (IS_SIZE_MEDIUM(bin->reg_size)) {
    gen_idx = bin->reg_size / 1024 - 1;
    run->gen = GENERATORS_MEDIUM[gen_idx][splitmix32() %
                                          GENERATORS_MEDIUM_LENGTHS[gen_idx]];
  }
  // Start from random element
  run->current_idx = splitmix32() % (bin->reg_mask_size_bits / 2);

  // Prepare bitmap, mark everything as free
  memset(run->reg_bitmap, 0x0, BITS2BYTES_CEIL(bin->reg_mask_size_bits));
}

// Allocate region from run
void *run_allocate(run_t *run, bin_t *bin) {
  // Check if we still have regions to give
  if (run->nfree == 0) return NULL;

  uintptr_t heap = (uintptr_t)run->run_heap;
  unsigned elems = (unsigned)(bin->reg_mask_size_bits / 2);

  // Get next item from generator
  run->current_idx = (run->gen + run->current_idx) % elems;

  // Sanity check
  bstate_t state;
  if ((state = get_bitmap_item(run->reg_bitmap, run->current_idx)) !=
      STATE_FREE) {
    se_error(
        "Region is not free, Region(heap=%p, nfree=%u, gen=%u, "
        "current_idx=%u), state=%u",
        run->run_heap, run->nfree, run->gen, run->current_idx, state);
  }

  // Mark region as allocated
  set_bitmap_item(run->reg_bitmap, run->current_idx, STATE_ALLOC);

  // Decrese amount of free regions
  run->nfree--;

  return (void *)(heap + (run->current_idx * bin->reg_size));
}

static size_t run_validate_freeable(run_t *run, bin_t *bin, void *ptr) {
  // Here, we trust that ptr is in range of current run
  ptrdiff_t rel_ptr = (uintptr_t)ptr - (uintptr_t)run->run_heap;
  size_t bitmap_idx = rel_ptr / bin->reg_size;
  // Check if ptr is unaligned
  if (rel_ptr % bin->reg_size != 0) {
    se_error("Unaligned ptr (ptr=%p)", ptr);
  }

  // Check for double free
  if (get_bitmap_item(run->reg_bitmap, bitmap_idx) != STATE_ALLOC) {
    se_error("Provided ptr not freeable (ptr=%p)", ptr);
  }

  return bitmap_idx;
}

// Deallocate region from run
void run_deallocate(run_t *run, bin_t *bin, void *ptr) {
  // Sanity check, if it passes we get region index in the bitmap
  size_t bitmap_idx = run_validate_freeable(run, bin, ptr);

  // Mark as freed
  set_bitmap_item(run->reg_bitmap, bitmap_idx, STATE_ALLOC_FREE);
}

bool run_is_full(run_t *run) { return run->nfree == 0; }
bool run_is_empty(run_t *run, bin_t *bin) {
  return run->nfree == bin->reg_mask_size_bits / 2;
}
