// Main library header file
#include <stddef.h>

#ifndef SEALLOC_H_
#define SEALLOC_H_

void *sealloc_malloc(size_t size);
void sealloc_free(void *ptr);
void *sealloc_calloc(size_t nmemb, size_t size);
void *sealloc_realloc(void *ptr, size_t size);

#endif /* SEALLOC_H_ */
