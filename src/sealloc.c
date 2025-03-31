#include <sealloc/arena.h>
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/run.h>
#include <sealloc/size_class.h>
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

static void *sealloc_allocate_with_bin(bin_t *bin) {
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
  run = arena_allocate_run(&main_arena, bin);
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

void *sealloc_malloc(size_t size) {
  size_t aligned_size;
  if (size == 0) return NULL;
  if (size >= LARGE_SIZE_MAX_REGION) {
    huge_chunk_t *huge;
    huge = internal_alloc(sizeof(huge_chunk_t));
    if (huge == NULL) return NULL;
    huge->len = ALIGNUP_PAGE(size);
    se_debug("Allocating huge chunk");
    huge->entry.key = arena_allocate_huge_mapping(&main_arena, huge->len);
    arena_store_huge_meta(&main_arena, huge);
    return huge->entry.key;
  }
  // TODO: handle all cases of size, eg. size == 0 , size == 1 etc..
  if (IS_SIZE_SMALL(size)) {
    aligned_size = ALIGNUP_SMALL_SIZE(size);
  } else if (IS_SIZE_MEDIUM(size)) {
    aligned_size = ALIGNUP_MEDIUM_SIZE(size);
  } else {
    // TODO: Bug if size == 5 then aligned is 8192
    aligned_size = alignup_large_size(size);
  }

  se_debug("Allocating region of size %zu (aligned to %zu)", size,
           aligned_size);
  bin_t *bin = arena_get_bin_by_reg_size(&main_arena, aligned_size);
  return sealloc_allocate_with_bin(bin);
}

static metadata_t locate_metadata_for_ptr(void *ptr, chunk_t **chunk_ret,
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
  se_debug("Found run (run_ptr : %p)", run_ptr);
  if (reg_size > 0) {
    se_debug("Getting bin for small/medium class");
    *bin_ret = arena_get_bin_by_reg_size(&main_arena, reg_size);
  } else {
    se_debug("Getting bin for large class");
    *bin_ret = arena_get_bin_by_reg_size(&main_arena, run_size);
  }
  *run_ret = bin_get_run_by_addr(*bin_ret, run_ptr);
  se_debug("Found run metadata at %p (reg_size : %u, run_size_pages %u)",
           (void *)*run_ret, (*bin_ret)->reg_size, (*bin_ret)->run_size_pages);
  return METADATA_REGULAR;
}

static void sealloc_free_with_metadata(chunk_t *chunk, bin_t *bin, run_t *run,
                                       void *ptr) {
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
    arena_deallocate_huge_mapping(&main_arena, huge->entry.key,
                                  ALIGNUP_PAGE(huge->len));
    arena_delete_huge_meta(&main_arena, huge);
    internal_free(huge);
    return;
  }

  sealloc_free_with_metadata(chunk, bin, run, ptr);
}

void *sealloc_realloc(void *ptr, size_t size) {
  se_debug("Reallocating a region at %p of size %zu", ptr, size);
  chunk_t *chunk;
  run_t *run_old;
  bin_t *bin_old, *bin_new;
  huge_chunk_t *huge;
  metadata_t meta;
  size_t size_palign = ALIGNUP_PAGE(size);
  if (ptr == NULL) {
    se_debug("Pointer is NULL, fallback to malloc");
    return sealloc_malloc(size);
  }
  meta = locate_metadata_for_ptr(ptr, &chunk, &run_old, &bin_old, &huge);
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
      arena_truncate_huge_mapping(&main_arena, huge, truncate_pages);
    }
    return huge->entry.key;
  }
  se_debug("Reallocating region of small/medium/large class");
  /*
   * size == old size -> zostawiamy jak jest
   * size > old_size i size jest dalej w tym samym binie -> zostawiamy jak jest
   * size > old_size i size jest w późniejszym binie -> zwalniamy region i
   * alokujemy nowy size < old_size i size jest w tym samym binie -> zostawiamy
   * jak jest size < old_size i size jest w wczesniejszym binie -> zwalniamy i
   * alokujemy
   */
  run_validate_ptr(run_old, bin_old, ptr);
  bin_new = arena_get_bin_by_reg_size(&main_arena, size);
  if (bin_new->reg_size == bin_old->reg_size) {
    se_debug("New allocation is still in the same class");
    return ptr;
  }
  void *dest = sealloc_allocate_with_bin(bin_new);
  if (dest == NULL) {
    se_debug("End of Memory");
    return NULL;
  }
  if (bin_new->reg_size > bin_old->reg_size) {
    memcpy(dest, ptr, bin_old->reg_size);
  } else if (bin_new->reg_size < bin_old->reg_size) {
    memcpy(dest, ptr, bin_new->reg_size);
  }
  sealloc_free_with_metadata(chunk, bin_old, run_old, ptr);
  return dest;
}

void *sealloc_calloc(size_t nmemb, size_t size) {
  // TODO: check for multiplication overflow
  void *ptr = sealloc_malloc(nmemb * size);
  memset(ptr, 0, nmemb * size);
  return ptr;
}
