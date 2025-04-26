#include <sealloc_api.h>
#include <stddef.h>

#include "sealloc/arena.h"
#include "sealloc/logging.h"
#include "sealloc/sealloc.h"
#include "sealloc/size_class.h"

// Main arena object
static arena_t arena;

#ifdef STATISTICS
#include <unistd.h>
__attribute__((destructor)) void close_stats_file(void) {
  if (arena.is_initialized != 0 && arena.stats_fd >= 0) close(arena.stats_fd);
}

void log_allocation(size_t size) {
  char *class;
  if (IS_SIZE_SMALL(size))
    class = "S";
  else if (IS_SIZE_MEDIUM(size))
    class = "M";
  else if (IS_SIZE_LARGE(size))
    class = "L";
  else {
    class = "H";
  }
  if (arena.stats_fd >= 0) fse_log(arena.stats_fd, "%s %zu\n", class, size);
}
#endif

void *malloc(size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }
#ifdef STATISTICS
  log_allocation(size);
#endif
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

#ifdef STATISTICS
  log_allocation(nmemb * size);
#endif
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

#ifdef STATISTICS
  log_allocation(size);
#endif
#ifdef DEBUG
  void *ret = sealloc_realloc(&arena, ptr, size);
  se_debug("Returning pointer: %p", ret);
  return ret;
#else
  return sealloc_realloc(&arena, ptr, size);
#endif
}
