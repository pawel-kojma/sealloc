#include <assert.h>
#include <string.h>

#include "sealloc/arena.h"
#include "sealloc/bin.h"
#include "sealloc/chunk.h"
#include "sealloc/internal_allocator.h"
#include "sealloc/logging.h"
#include "sealloc/run.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

typedef enum metadata_type {
  METADATA_INVALID,
  METADATA_REGULAR,
  METADATA_HUGE
} metadata_t;

static void *sealloc_allocate_with_bin(arena_t *arena, bin_t *bin) {
  void *ptr;
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
  run = arena_allocate_run(arena, bin);
  if (run == NULL) {
    se_debug("End of memory");
    return NULL;
  }
  se_debug("Got run (run_ptr : %p)", run->entry.key);
  bin->runcur = run;
  se_debug("Allocating from fresh run");
  ptr = run_allocate(run, bin);
  if (run_is_depleted(run)) {
    se_debug("Retiring run");
    bin_retire_current_run(bin);
  }
  return ptr;
}

static huge_chunk_t *sealloc_allocate_huge(arena_t *arena,
                                           size_t aligned_size) {
  huge_chunk_t *huge;
  huge = internal_alloc(sizeof(huge_chunk_t));
  if (huge == NULL) return NULL;
  huge->len = aligned_size;
  huge->entry.link.fd = NULL;
  huge->entry.link.bk = NULL;
  se_debug("Allocating huge chunk");
  huge->entry.key = arena_allocate_huge_mapping(arena, huge->len);
  if (huge->entry.key == NULL) {
    return NULL;
  }
  arena_store_huge_meta(arena, huge);
  return huge;
}

void *sealloc_malloc(arena_t *arena, size_t size) {
  size_t aligned_size;

  /*
   * Some programs seem to allocate size 0 to reallocate in future
   * and fail if NULL is returned, like grep.
   */
  if (size == 0) size = SMALL_SIZE_MIN_REGION;

  if (IS_SIZE_HUGE(size)) {
    huge_chunk_t *huge = sealloc_allocate_huge(arena, ALIGNUP_PAGE(size));
    if (huge == NULL) return NULL;
    return huge->entry.key;
  }

  if (IS_SIZE_SMALL(size)) {
    aligned_size = ALIGNUP_SMALL_SIZE(size);
  } else if (IS_SIZE_MEDIUM(size)) {
    aligned_size = alignup_medium_size(size);
  } else {
    aligned_size = alignup_large_size(size);
  }

  se_debug("Allocating region of size %zu (aligned to %zu)", size,
           aligned_size);
  bin_t *bin = arena_get_bin_by_reg_size(arena, aligned_size);
  return sealloc_allocate_with_bin(arena, bin);
}

static metadata_t locate_metadata_for_ptr(arena_t *arena, void *ptr,
                                          chunk_t **chunk_ret, run_t **run_ret,
                                          bin_t **bin_ret,
                                          huge_chunk_t **huge_ret) {
  chunk_t *chunk;
  void *run_ptr = NULL;
  unsigned run_size = 0, reg_size = 0;
  se_debug("Getting chunk from pointer");
  chunk = arena_get_chunk_from_ptr(arena, ptr);
  if (chunk == NULL) {
    se_debug("No chunk found, checking huge allocations.");
    huge_chunk_t *huge_chunk;
    huge_chunk = arena_find_huge_mapping(arena, ptr);
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
  se_debug("Found run (run_ptr : %p)", run_ptr);
  if (reg_size > 0) {
    se_debug("Getting bin for small/medium class");
    *bin_ret = arena_get_bin_by_reg_size(arena, reg_size);
  } else {
    se_debug("Getting bin for large class");
    *bin_ret = arena_get_bin_by_reg_size(arena, run_size);
  }
  *run_ret = bin_get_run_by_addr(*bin_ret, run_ptr);
  se_debug("Found run metadata at %p (reg_size : %u, run_size_pages %u)",
           (void *)*run_ret, (*bin_ret)->reg_size, (*bin_ret)->run_size_pages);
  return METADATA_REGULAR;
}

static void sealloc_free_with_metadata(arena_t *arena, chunk_t *chunk,
                                       bin_t *bin, run_t *run, void *ptr) {
  if (!run_deallocate(run, bin, ptr)) {
    se_log("Invalid call to free()");
    abort();
  }

  if (run_is_freeable(run, bin)) {
    se_debug("Run is freeable, deleting");
    bin_delete_run(bin, run);
    if (chunk_deallocate_run(chunk, run->entry.key)) {
      se_debug("Chunk is fully unmapped, deallocating chunk metadata");
      arena_deallocate_chunk(arena, chunk);
    }
    internal_free(run);
  }
}

void sealloc_free(arena_t *arena, void *ptr) {
  se_debug("Freeing a region at %p", ptr);
  chunk_t *chunk;
  run_t *run;
  bin_t *bin;
  huge_chunk_t *huge;
  metadata_t meta;
  if (ptr == NULL) return;
  meta = locate_metadata_for_ptr(arena, ptr, &chunk, &run, &bin, &huge);
  if (meta == METADATA_INVALID) {
    se_log("Invalid call to free()");
    abort();
  }
  if (meta == METADATA_HUGE) {
    arena_deallocate_huge_mapping(arena, huge->entry.key,
                                  ALIGNUP_PAGE(huge->len));
    arena_delete_huge_meta(arena, huge);
    internal_free(huge);
    return;
  }

  sealloc_free_with_metadata(arena, chunk, bin, run, ptr);
}

static void *realloc_huge(arena_t *arena, huge_chunk_t *huge, size_t new_size) {
  se_debug("Reallocating huge chunk at %p", huge->entry.key);
  if (IS_SIZE_HUGE(new_size)) {
    void *new_map = arena_allocate_huge_mapping(arena, new_size);
    if (new_map == NULL) {
      se_debug("Failed to reallocate huge mapping");
      return NULL;
    }
    if (huge->len > new_size) {
      memcpy(new_map, huge->entry.key, new_size);
    } else {
      memcpy(new_map, huge->entry.key, huge->len);
    }
    arena_delete_huge_meta(arena, huge);
    huge->entry.key = new_map;
    huge->len = new_size;
    huge->entry.link.bk = NULL;
    huge->entry.link.fd = NULL;
    arena_store_huge_meta(arena, huge);
    return huge->entry.key;
  }
  // huge -> regular case
  void *new_reg = sealloc_malloc(arena, new_size);
  if (new_reg == NULL) {
    se_debug("Failed to allocate region for huge mapping reallocation");
    return NULL;
  }
  memcpy(new_reg, huge->entry.key, new_size);
  arena_deallocate_huge_mapping(arena, huge->entry.key, huge->len);
  arena_delete_huge_meta(arena, huge);
  internal_free(huge);
  return new_reg;
}

void *sealloc_realloc(arena_t *arena, void *old_ptr, size_t new_size) {
  se_debug("Reallocating a region at %p to size %zu", old_ptr, new_size);
  chunk_t *chunk;
  run_t *run_old;
  bin_t *bin_old, *bin_new;
  huge_chunk_t *huge;
  metadata_t meta;
  size_t new_size_aligned;
  if (old_ptr == NULL) {
    se_debug("Pointer is NULL, fallback to malloc");
    return sealloc_malloc(arena, new_size);
  }
  meta = locate_metadata_for_ptr(arena, old_ptr, &chunk, &run_old, &bin_old,
                                 &huge);
  if (meta == METADATA_INVALID) {
    se_log("Invalid call to realloc()");
    abort();
  }
  if (meta == METADATA_HUGE) {
    return realloc_huge(arena, huge, ALIGNUP_PAGE(new_size));
  }

  if (run_validate_ptr(run_old, bin_old, old_ptr) == SIZE_MAX) {
    se_log("Invalid call to realloc()");
    abort();
  }
  se_debug("Reallocating region of small/medium/large class");
  if (IS_SIZE_HUGE(new_size)) {
    huge = sealloc_allocate_huge(arena, ALIGNUP_PAGE(new_size));
    if (huge == NULL) return NULL;
    memcpy(huge->entry.key, old_ptr, bin_old->reg_size);
    sealloc_free_with_metadata(arena, chunk, bin_old, run_old, old_ptr);
    return huge->entry.key;
  }

  if (IS_SIZE_SMALL(new_size)) {
    new_size_aligned = ALIGNUP_SMALL_SIZE(new_size);
  } else if (IS_SIZE_MEDIUM(new_size)) {
    new_size_aligned = alignup_medium_size(new_size);
  } else {
    new_size_aligned = alignup_large_size(new_size);
  }

  bin_new = arena_get_bin_by_reg_size(arena, new_size_aligned);
  if (bin_new->reg_size == bin_old->reg_size) {
    se_debug("New allocation is still in the same class");
    return old_ptr;
  }

  // Allocate new region
  void *new_ptr = sealloc_allocate_with_bin(arena, bin_new);
  if (new_ptr == NULL) {
    se_debug("End of Memory");
    return NULL;
  }

  // Copy data from old to new
  if (bin_new->reg_size > bin_old->reg_size) {
    memcpy(new_ptr, old_ptr, bin_old->reg_size);
  } else { /* bin_new->reg_size < bin_old->reg_size */
    memcpy(new_ptr, old_ptr, bin_new->reg_size);
  }
  sealloc_free_with_metadata(arena, chunk, bin_old, run_old, old_ptr);
  return new_ptr;
}

void *sealloc_calloc(arena_t *arena, size_t nmemb, size_t size) {
  if (nmemb == 0 || size == 0) return NULL;
  void *ptr = sealloc_malloc(arena, nmemb * size);
  memset(ptr, 0, nmemb * size);
  return ptr;
}
