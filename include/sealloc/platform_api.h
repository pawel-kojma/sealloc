/* API for system specific functions */

#ifndef _SEALLOC_PLATFORM_API_H_
#define _SEALLOC_PLATFORM_API_H_

#include <stddef.h>

void* platform_map(void *hint, size_t len);
int platform_unmap(void *ptr, size_t len);
int platform_guard(void *ptr, size_t len);
int platform_unguard(void *ptr, size_t len);

#endif  // _SEALLOC_PLATFORM_API_H_
