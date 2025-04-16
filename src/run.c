#include "sealloc/run.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "sealloc/bin.h"
#include "sealloc/generator.h"
#include "sealloc/logging.h"
#include "sealloc/random.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

#ifdef __aarch64__
#include "sealloc/arch/aarch64.h"
#include "sealloc/platform_api.h"

/*!
 * @brief Tags the memory granule under address addr with same tag as the
 * tagged_ptr.
 */
static inline void set_tag(const uint64_t tagged_ptr, const uint64_t addr) {
  __asm__ volatile("stg %0, [%1]" : : "r"(tagged_ptr), "r"(addr) : "memory");
}

/*!
 * @brief Loads the tag from memory pointed by addr
 */
static inline uint64_t get_tag_from_memory(const uint64_t addr) {
  uint64_t res;
  __asm__ volatile("ldg %0, [%1]" : "=r"(res) : "r"(addr));
  return res;
}

/*!
 * @brief Tags the pointer with random tag, excluding tags specified in
 * excludes.
 */
static inline uint64_t tag_pointer(const uint64_t ptr,
                                   const uint64_t excludes) {
  uint64_t res;
  __asm__("irg %0, %1, %2" : "=r"(res) : "r"(ptr), "r"(excludes));
  return res;
}

/*!
 * @brief Tags the pointer with random tag using default excludes.
 */
static inline uint64_t tag_pointer_default(const uint64_t ptr) {
  uint64_t res;
  __asm__("irg %0, %1" : "=r"(res) : "r"(ptr));
  return res;
}

/*!
 * @brief Tags n granules of memory starting at addr with tag in tagged_ptr.
 * @warning n must be a multiple of MEMORY_GRANULE_SIZE
 */
static inline void set_tag_n(const uint64_t tagged_ptr, const uint64_t addr,
                             const size_t n) {
  assert(n % MEMORY_GRANULE_SIZE == 0);
  assert(tagged_ptr % MEMORY_GRANULE_SIZE == 0);
  assert(addr % MEMORY_GRANULE_SIZE == 0);
  size_t n2g = n / MEMORY_2_GRANULE_SIZE;
  size_t n1g = (n - MEMORY_2_GRANULE_SIZE * n2g) / MEMORY_GRANULE_SIZE;
  uint64_t _addr = addr;
  for (size_t i = 0; i < n2g; i++) {
    __asm__ volatile("st2g %0, [%1]"
                     :
                     : "r"(tagged_ptr), "r"(_addr)
                     : "memory");
    _addr += MEMORY_2_GRANULE_SIZE;
  }
  for (size_t i = 0; i < n1g; i++) {
    __asm__ volatile("stg %0, [%1]" : : "r"(tagged_ptr), "r"(_addr) : "memory");
    _addr += MEMORY_GRANULE_SIZE;
  }
}

/*!
 *  @brief adds tag from tagged_ptr to excludes mask.
 */
static inline uint64_t add_to_excludes(const uint64_t tagged_ptr,
                                       const uint64_t excludes) {
  uint64_t res;
  __asm__("gmi %0, %1, %2" : "=r"(res) : "r"(tagged_ptr), "r"(excludes));
  return res;
}

#endif

// Get state of a region from bitmap
static inline bstate_t get_bitmap_item(uint8_t *mem, size_t idx) {
  size_t word = idx / 4;
  size_t off = idx % 4;
  return (bstate_t)(mem[word] >> (2 * off) & 3);
}

// Set region state inside a bitmap
static inline void set_bitmap_item(uint8_t *mem, size_t idx, bstate_t state) {
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
  run->entry.key = heap;
  run->entry.link.fd = NULL;
  run->entry.link.bk = NULL;
  run->navail = bin->reg_mask_size_bits / 2;
  run->nfreed = 0;

  // Choose a generator
  if (IS_SIZE_SMALL(bin->reg_size)) {
    gen_idx = SIZE_TO_IDX_SMALL(bin->reg_size);
    run->gen = GENERATORS_SMALL[gen_idx][splitmix32() %
                                         GENERATORS_SMALL_LENGTHS[gen_idx]];
  } else if (IS_SIZE_MEDIUM(bin->reg_size)) {
    gen_idx = size_to_idx_medium(bin->reg_size);
    run->gen = GENERATORS_MEDIUM[gen_idx][splitmix32() %
                                          GENERATORS_MEDIUM_LENGTHS[gen_idx]];
  } else {
    // Runs services single large allocation
    run->gen = 1;
  }
  // Start from random element
  run->current_idx = splitmix32() % (bin->reg_mask_size_bits / 2);

  // Prepare bitmap, mark everything as free
  memset(run->reg_bitmap, 0x0, BITS2BYTES_CEIL(bin->reg_mask_size_bits));
}

// Allocate region from run
void *run_allocate(run_t *run, bin_t *bin) {
  // Check if we still have regions to give
  if (run->navail == 0) return NULL;

  uintptr_t heap = (uintptr_t)run->entry.key;
  unsigned elems = (unsigned)(bin->reg_mask_size_bits / 2);
  void *ptr;

  // Get next item from generator
  run->current_idx = (run->gen + run->current_idx) % elems;

  // Sanity check
  bstate_t state;
  if ((state = get_bitmap_item(run->reg_bitmap, run->current_idx)) !=
      STATE_FREE) {
    se_error(
        "Region is not free, Region(heap=%p, nfree=%u, gen=%u, "
        "current_idx=%u), state=%u",
        run->entry.key, run->navail, run->gen, run->current_idx, state);
  }

  // Mark region as allocated
  set_bitmap_item(run->reg_bitmap, run->current_idx, STATE_ALLOC);

  // Decrese amount of free regions
  run->navail--;
  se_debug("Allocating region at current_idx %u, next is %u", run->current_idx,
           (run->gen + run->current_idx) % elems);
  ptr = (void *)(heap + (run->current_idx * bin->reg_size));

#if __aarch64__ && __ARM_FEATURE_MEMORY_TAGGING
  if (!is_mte_enabled) {
    return ptr;
  }
  // Always keep tag 0 in excludes
  uint64_t excludes = 1, nptr;
  if (run->current_idx > 0 &&
      get_bitmap_item(run->reg_bitmap, run->current_idx - 1) == STATE_ALLOC) {
    nptr = get_tag_from_memory((uint64_t)ptr - bin->reg_size);
    excludes = add_to_excludes(nptr, excludes);
  }

  if (run->current_idx < ((bin->reg_mask_size_bits / 2) - 1) &&
      get_bitmap_item(run->reg_bitmap, run->current_idx + 1) == STATE_ALLOC) {
    nptr = get_tag_from_memory((uint64_t)ptr + bin->reg_size);
    excludes = add_to_excludes(nptr, excludes);
  }
  ptr = (void *)tag_pointer((uint64_t)ptr, excludes);
    set_tag_n((uint64_t)ptr, (uint64_t)ptr, bin->reg_size);
  return ptr;
#else
  return ptr;
#endif
}

size_t run_validate_ptr(run_t *run, bin_t *bin, void *ptr) {
  // Here, we trust that ptr is in range of current run
  ptrdiff_t rel_ptr = (uintptr_t)ptr - (uintptr_t)run->entry.key;
  size_t bitmap_idx = rel_ptr / bin->reg_size;
  // Check if ptr is unaligned
  if (rel_ptr % bin->reg_size != 0) {
    se_debug("Unaligned ptr (ptr=%p)", ptr);
    return SIZE_MAX;
  }

  // Check for double free
  if (get_bitmap_item(run->reg_bitmap, bitmap_idx) != STATE_ALLOC) {
    // se_error("Provided ptr not freeable (ptr=%p)", ptr);
    return SIZE_MAX;
  }

  return bitmap_idx;
}

// Deallocate region from run
bool run_deallocate(run_t *run, bin_t *bin, void *ptr) {
  // Sanity check, if it passes we get region index in the bitmap
  size_t bitmap_idx = run_validate_ptr(run, bin, ptr);
  if (bitmap_idx == SIZE_MAX) return false;

  // Mark as freed
  set_bitmap_item(run->reg_bitmap, bitmap_idx, STATE_ALLOC_FREE);
  run->nfreed++;
  return true;
}

bool run_is_depleted(run_t *run) { return run->navail == 0; }
bool run_is_freeable(run_t *run, bin_t *bin) {
  return run->nfreed == bin->reg_mask_size_bits / 2;
}
