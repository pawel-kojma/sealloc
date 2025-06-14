#ifndef SEALLOC_API_H_
#define SEALLOC_API_H_

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#endif /* SEALLOC_API_H_ */
