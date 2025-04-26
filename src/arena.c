#include "sealloc/arena.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sealloc/chunk.h"
#include "sealloc/container_ll.h"
#include "sealloc/internal_allocator.h"
#include "sealloc/logging.h"
#include "sealloc/platform_api.h"
#include "sealloc/random.h"
#include "sealloc/run.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

static void reset_ia_ptr_start(arena_t *arena) {
  arena->internal_alloc_ptr =
      arena->brk + (splitmix64() % (MAX_USERSPACE_ADDR64 - arena->brk));
  arena->internal_alloc_ptr = ALIGNUP_PAGE(arena->internal_alloc_ptr);
}

static void reset_huge_alloc_ptr_start(arena_t *arena) {
  arena->huge_alloc_ptr =
      arena->brk + (splitmix64() % (MAX_USERSPACE_ADDR64 - arena->brk));
  arena->huge_alloc_ptr = ALIGNUP_PAGE(arena->huge_alloc_ptr);
}

static void reset_chunk_ptr_start(arena_t *arena) {
  // Make sure program break is reasonably large (at least 45 bits) to be our
  // separation point between regular chunks and huge/internal mappings
  arena->chunk_alloc_ptr = ALIGNUP_PAGE(splitmix32());
  if (arena->brk <= MASK_44_BITS) {
    arena->brk = ALIGNUP_PAGE(splitmix64() & MASK_45_BITS);
  }
}
typedef void (*reset_fun)(arena_t *);

static uintptr_t arena_morecore(arena_t *arena, volatile uintptr_t *probe_ptr,
                                reset_fun reset, size_t size,
                                volatile uintptr_t *ceil) {
  platform_status_code_t code;
  uintptr_t result;
  se_debug("Mapping more memory at %p", *probe_ptr);
  code = platform_map_probe(probe_ptr, *ceil, size);
  while (code != PLATFORM_STATUS_OK) {
    if (code == PLATFORM_STATUS_CEILING_HIT) {
      se_debug("Ceilling hit with %p, resetting", *probe_ptr);
      reset(arena);
    } else if (code == PLATFORM_DIFFERENT_ADDRESS) {
      se_debug("Different address mapped %p", *probe_ptr);
      result = *probe_ptr;
      reset(arena);
      return result;
    }
    se_debug("Mapping more memory at %p", *probe_ptr);
    code = platform_map_probe(probe_ptr, *ceil, size);
  }
  return *probe_ptr;
}

void arena_init(arena_t *arena) {
  assert(arena->is_initialized == 0);
  platform_status_code_t code;
  void *ptr;
#ifdef STATISTICS
#include <fcntl.h>
  int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int fd = open("./heap_stats.sealloc", O_WRONLY | O_APPEND | O_CREAT, perms);
  if (fd < 0) {
    se_log("Couldn't open file to write heap statistics to\n");
    arena->stats_fd = -1;
  } else {
    se_log("Opened heap_stats.sealloc file for statistics\n");
    arena->stats_fd = fd;
  }
#endif
#ifdef DEBUG
  char *user_rand = getenv("SEALLOC_SEED");
  if (user_rand != NULL) {
    arena->secret = str2u32(user_rand);
    se_log("Using user passed secret: %u\n", arena->secret);
  } else
#endif
      if ((code = platform_get_random(&arena->secret)) != PLATFORM_STATUS_OK) {
    se_error("Failed to get random value: %s", platform_strerror(code));
  }
#ifdef DEBUG
  se_log("Using secret %u\n", arena->secret);
#endif
  if ((code = platform_get_program_break(&ptr)) != PLATFORM_STATUS_OK) {
    se_error("Failed to get program break: %s", platform_strerror(code));
  }
  arena->brk = (uintptr_t)ptr;
  init_splitmix32(arena->secret);
  init_splitmix64(arena->secret);
  ll_init(&arena->internal_alloc_list);
  ll_init(&arena->chunk_list);
  ll_init(&arena->huge_alloc_list);
  arena->is_initialized = 1;

  /* Regular allocations start at 32-bit address */
  reset_chunk_ptr_start(arena);
  /* Metadata and huge allocations start at random 48-bit address bigger than
   * program break */
  reset_ia_ptr_start(arena);
  reset_huge_alloc_ptr_start(arena);
  arena->chunk_ptr = 0;
  arena->chunks_left = 0;
  memset(arena->bins, 0, sizeof(bin_t) * ARENA_NO_BINS);
}

void *arena_internal_alloc(arena_t *arena, size_t size) {
  int_alloc_t *map;
  void *alloc;
  // Loop over memory mappings and try to satisfy the request
  for (ll_entry_t *root = arena->internal_alloc_list.ll; root != NULL;
       root = root->link.fd) {
    se_debug("Trying to allocate with root = %p", (void *)root);
    alloc = internal_alloc(CONTAINER_OF(root, int_alloc_t, entry), size);
    if (alloc != NULL) return alloc;
  }

  // No mapping can satisfy the request, try to get more memory
  se_debug("Trying to allocate more metadata memory");
  uintptr_t ceil_addr = MAX_USERSPACE_ADDR64;
  // Get new metadata mapping
  map = (int_alloc_t *)arena_morecore(
      arena, &arena->internal_alloc_ptr, reset_ia_ptr_start,
      ALIGNUP_PAGE(sizeof(int_alloc_t)), &ceil_addr);

  // Update pointer for next mapping
  if ((uintptr_t)map == arena->internal_alloc_ptr)
    arena->internal_alloc_ptr += ALIGNUP_PAGE(sizeof(int_alloc_t));

  internal_allocator_init(map);
  ll_add(&arena->internal_alloc_list, &map->entry);

  se_debug("Allocating from fresh mapping");
  return internal_alloc(map, size);
}

void arena_internal_free(arena_t *arena, void *ptr) {
  uintptr_t ptr_dest = (uintptr_t)ptr;
  int_alloc_t *root = NULL;

  // Find mapping that contains the ptr
  for (root = CONTAINER_OF(arena->internal_alloc_list.ll, int_alloc_t, entry);
       root != NULL;
       root = CONTAINER_OF(root->entry.link.fd, int_alloc_t, entry)) {
    if ((uintptr_t)&root->memory <= ptr_dest &&
        ptr_dest <= (uintptr_t)root->memory + sizeof(root->memory))
      break;
  }

  // No mapping found, panic
  if (!root) {
    se_error("root == NULL");
  }

  internal_free(root, ptr);
}

run_t *arena_allocate_run(arena_t *arena, bin_t *bin) {
  assert(arena->is_initialized == 1);
  assert(bin->reg_size != 0);

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
      run = arena_internal_alloc(
          arena, sizeof(run_t) + BITS2BYTES_CEIL(bin->reg_mask_size_bits));
      run_init(run, bin, run_ptr);
      return run;
    }
  }
  // No luck finding, allocate a new one
  // Will also be addded to chunk list
  chunk = arena_allocate_chunk(arena);
  run_ptr =
      chunk_allocate_run(chunk, bin->run_size_pages * PAGE_SIZE, bin->reg_size);
  assert(run_ptr != NULL && "Failed to allocate run from fresh chunk");

  run = arena_internal_alloc(
      arena, sizeof(run_t) + BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  run_init(run, bin, run_ptr);
  return run;
}

chunk_t *arena_allocate_chunk(arena_t *arena) {
  assert(arena->is_initialized == 1);

  chunk_t *chunk_meta = arena_internal_alloc(arena, sizeof(chunk_t));
  platform_status_code_t code;
  size_t map_len = CHUNKS_PER_MAPPING * (CHUNK_SIZE_BYTES + PAGE_SIZE);
  // get more memory if needed
  if (arena->chunks_left == 0) {
    arena->chunk_ptr =
        arena_morecore(arena, &arena->chunk_alloc_ptr, reset_chunk_ptr_start,
                       map_len, &arena->brk);
    arena->chunks_left = CHUNKS_PER_MAPPING;
  }

  // Guard one page after the end of chunk
  code =
      platform_guard((void *)(arena->chunk_ptr + CHUNK_SIZE_BYTES), PAGE_SIZE);
  if (code != PLATFORM_STATUS_OK) {
    se_error("Failed to allocate mapping (size : %u): %s", PAGE_SIZE,
             platform_strerror(code));
  }
  chunk_init(chunk_meta, (void *)arena->chunk_ptr);
  ll_add(&arena->chunk_list, &chunk_meta->entry);
  arena->chunk_ptr += (CHUNK_SIZE_BYTES + PAGE_SIZE);
  arena->chunks_left--;
  return chunk_meta;
}

void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk) {
  assert(arena->is_initialized == 1);
  assert(chunk_is_unmapped(chunk));
  se_debug("Deallocating chunk");
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
  arena_internal_free(arena, chunk);
}

chunk_t *arena_get_chunk_from_ptr(const arena_t *arena, const void *ptr,
                                  chunk_t *start_chunk) {
  assert(arena->is_initialized == 1);

  chunk_t *chunk = NULL;
  ptrdiff_t target = (ptrdiff_t)ptr;
  ll_entry_t *entry =
      (start_chunk == NULL) ? arena->chunk_list.ll : start_chunk->entry.link.fd;
  for (; entry != NULL; entry = entry->link.fd) {
    se_debug("Trying entry entry=%p, entry->key=%p", (void *)entry, entry->key);
    assert(IS_ALIGNED((ptrdiff_t)entry->key, PAGE_SIZE));
    if ((ptrdiff_t)entry->key <= target &&
        target < ((ptrdiff_t)entry->key + CHUNK_SIZE_BYTES)) {
      chunk = CONTAINER_OF(entry, chunk_t, entry);
      assert(chunk->entry.key == entry->key);
      break;
    }
  }
  return chunk;
}

static inline unsigned ceil_div(unsigned a, unsigned b) {
  return (a / b) + (a % b == 0 ? 0 : 1);
}

bool arena_supply_runs(arena_t *arena, bin_t *bin) {
  assert(BIN_MINIMUM_REGIONS > bin->avail_regs);
  run_t *run;
  unsigned runs_to_allocate = ceil_div(BIN_MINIMUM_REGIONS - bin->avail_regs,
                                       bin->reg_mask_size_bits / 2);
  se_debug("Adding %u more runs (%u regions total)", runs_to_allocate,
           runs_to_allocate * (bin->reg_mask_size_bits / 2));
  for (unsigned i = 0; i < runs_to_allocate; i++) {
    run = arena_allocate_run(arena, bin);
    if (run == NULL) return false;
    bin_add_run(bin, run);
    se_debug("Added run %p to bin %p", run, bin);
  }
  return true;
}

bin_t *arena_get_bin_by_reg_size(arena_t *arena, unsigned reg_size) {
  assert(arena->is_initialized == 1);
  assert(reg_size >= 1);
  assert(reg_size <= LARGE_SIZE_MAX_REGION);

  unsigned skip_bins = 0;
  bin_t *bin = NULL;
  // Assume reg_size is either small, medium or large class
  if (IS_SIZE_SMALL(reg_size)) {
    // 0 .. 31
    assert(ALIGNUP_SMALL_SIZE(reg_size) == reg_size);
    bin = &arena->bins[SIZE_TO_IDX_SMALL(reg_size)];
  } else if (IS_SIZE_MEDIUM(reg_size)) {
    // 32 .. 34
    assert(alignup_medium_size(reg_size) == reg_size);
    skip_bins = NO_SMALL_SIZE_CLASSES;
    bin = &arena->bins[skip_bins + size_to_idx_medium(reg_size)];
  } else {
    // 35 ... 43
    assert(alignup_large_size(reg_size) == reg_size);
    skip_bins = NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES;
    bin = &arena->bins[skip_bins + size_to_idx_large(reg_size)];
  }
  if (bin->reg_size == 0) {
    bin_init(bin, reg_size);
  }
  assert(bin->reg_size == reg_size);
  return bin;
}

huge_chunk_t *arena_find_huge_mapping(const arena_t *arena,
                                      const void *huge_map) {
  assert(arena->is_initialized == 1);
  ll_entry_t *entry = ll_find(&arena->huge_alloc_list, huge_map);
  if (entry == NULL) return NULL;
  huge_chunk_t *huge = CONTAINER_OF(entry, huge_chunk_t, entry);
  assert(huge->entry.key == huge_map);
  return huge;
}

huge_chunk_t *arena_allocate_huge_mapping(arena_t *arena, size_t len) {
  assert(IS_ALIGNED(len, PAGE_SIZE));
  assert(arena->is_initialized == 1);
  huge_chunk_t *huge;
  uintptr_t map;
  uintptr_t ceil_addr = MAX_USERSPACE_ADDR64;

  huge = arena_internal_alloc(arena, sizeof(huge_chunk_t));
  huge->len = len;
  huge->entry.link.fd = NULL;
  huge->entry.link.bk = NULL;
  map = arena_morecore(arena, &arena->huge_alloc_ptr,
                       reset_huge_alloc_ptr_start, len, &ceil_addr);
  huge->entry.key = (void *)map;
  // Leave one page space in between to avoid overflows
  if (map == arena->huge_alloc_ptr) arena->huge_alloc_ptr += len + PAGE_SIZE;
  ll_add(&arena->huge_alloc_list, &huge->entry);
  return huge;
}

void arena_reallocate_huge_mapping(arena_t *arena, huge_chunk_t *huge,
                                   size_t new_size) {
  assert(IS_ALIGNED(huge->len, PAGE_SIZE));
  assert(IS_ALIGNED(new_size, PAGE_SIZE));
  assert(arena->is_initialized == 1);
  assert(IS_SIZE_HUGE(new_size));

  // If aligned size is the same, then do nothing
  if (new_size == huge->len) return;

  platform_status_code_t code;
  uintptr_t map;
  uintptr_t ceil_addr = MAX_USERSPACE_ADDR64;
  // Make new huge allocation
  map = arena_morecore(arena, &arena->huge_alloc_ptr,
                       reset_huge_alloc_ptr_start, new_size, &ceil_addr);
  // Leave one page space in between to avoid overflows
  if (map == arena->huge_alloc_ptr)
    arena->huge_alloc_ptr += new_size + PAGE_SIZE;

  // Transfer data from old one to new
  if (huge->len > new_size) {
    memcpy((void *)map, huge->entry.key, new_size);
  } else {
    memcpy((void *)map, huge->entry.key, huge->len);
  }

  // Deallocate old mapping
  if ((code = platform_unmap(huge->entry.key, huge->len)) !=
      PLATFORM_STATUS_OK) {
    se_error("Failed to deallocate huge mapping (ptr : %p, size : %u): %s",
             huge->entry.key, huge->len, platform_strerror(code));
  }

  // Update chunk info
  huge->entry.key = (void *)map;
  huge->len = new_size;
}

void arena_deallocate_huge_mapping(arena_t *arena, huge_chunk_t *huge) {
  assert(IS_ALIGNED(huge->len, PAGE_SIZE));
  assert(arena->is_initialized == 1);
  platform_status_code_t code;
  if ((code = platform_unmap(huge->entry.key, huge->len)) !=
      PLATFORM_STATUS_OK) {
    se_error("Failed to deallocate huge mapping (ptr : %p, size : %u): %s",
             huge->entry.key, huge->len, platform_strerror(code));
  }
  ll_del(&arena->huge_alloc_list, &huge->entry);
  arena_internal_free(arena, huge);
}
