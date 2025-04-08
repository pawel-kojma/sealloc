#include <sealloc_api.h>
#include <stddef.h>

#include "sealloc/arena.h"
#include "sealloc/logging.h"
#include "sealloc/sealloc.h"

// Main arena object
static arena_t arena;

void *malloc(size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }
#ifdef DEBUG
  void *ret = sealloc_malloc(&arena, size);
  se_debug("Returning pointer: %p", ret);
  return ret;
#else
  return sealloc_malloc(&arena, size);
#endif
}
void free(void *ptr) { sealloc_free(&arena, ptr); }
void *calloc(size_t nmemb, size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }

#ifdef DEBUG
  se_debug("Allocating size: nmemb=%zu size=%zu", nmemb, size);
  void *ret = sealloc_calloc(&arena, nmemb, size);
  se_debug("Returning pointer: %p", ret);
  return ret;
#else
  return sealloc_calloc(&arena, nmemb, size);
#endif
}
void *realloc(void *ptr, size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }

#ifdef DEBUG
  void *ret = sealloc_realloc(&arena, ptr, size);
  se_debug("Returning pointer: %p", ret);
  return ret;
#else
  return sealloc_realloc(&arena, ptr, size);
#endif
}
