// Main library header file

#ifndef SEALLOC_H_
#define SEALLOC_H_

#include <stddef.h>

#include "arena.h"

typedef enum metadata_type {
  METADATA_INVALID,
  METADATA_REGULAR,
  METADATA_HUGE
} metadata_t;

metadata_t locate_metadata_for_ptr(arena_t *arena, void *ptr,
                                   chunk_t **chunk_ret, run_t **run_ret,
                                   bin_t **bin_ret, huge_chunk_t **huge_ret);
void *sealloc_malloc(arena_t *arena, size_t size);
void sealloc_free(arena_t *arena, void *ptr);
void *sealloc_calloc(arena_t *arena, size_t nmemb, size_t size);
void *sealloc_realloc(arena_t *arena, void *ptr, size_t size);

#endif /* SEALLOC_H_ */
