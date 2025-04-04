#ifndef SEALLOC_API_H_
#define SEALLOC_API_H_

#include <sealloc/sealloc_export.h>
#include <stddef.h>

SEALLOC_EXPORT void *malloc(size_t size);
SEALLOC_EXPORT void free(void *ptr);
SEALLOC_EXPORT void *calloc(size_t nmemb, size_t size);
SEALLOC_EXPORT void *realloc(void *ptr, size_t size);

#endif /* SEALLOC_API_H_ */
