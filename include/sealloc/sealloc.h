// Main library header file
#include <sealloc/arena.h>
#include <stddef.h>

#ifndef SEALLOC_H_
#define SEALLOC_H_

void *sealloc_malloc(arena_t *arena, size_t size);
void sealloc_free(arena_t *arena, void *ptr);
void *sealloc_calloc(arena_t *arena, size_t nmemb, size_t size);
void *sealloc_realloc(arena_t *arena, void *ptr, size_t size);

#endif /* SEALLOC_H_ */
