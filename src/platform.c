#include <sealloc/platform_api.h>
#include <stddef.h>

void *platform_map(void *hint, size_t len) { return NULL; }
int platform_unmap(void *ptr, size_t len) { return 0; }
int platform_guard(void *ptr, size_t len) { return 0; }
int platform_unguard(void *ptr, size_t len) { return 0; }
