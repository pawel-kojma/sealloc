#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
#include <sealloc_api.h>
#include <stddef.h>

// Main arena object
static arena_t arena;

void *malloc(size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }
  return sealloc_malloc(&arena, size);
}
void free(void *ptr) { sealloc_free(&arena, ptr); }
void *calloc(size_t nmemb, size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }
  return sealloc_calloc(&arena, nmemb, size);
}
void *realloc(void *ptr, size_t size) {
  if (arena.is_initialized == 0) {
    arena_init(&arena);
  }
  return sealloc_realloc(&arena, ptr, size);
}
