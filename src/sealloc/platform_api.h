/* API for system specific functions */


#ifndef SEALLOC_PLATFORM_API_H_
#define SEALLOC_PLATFORM_API_H_

#include "logging.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef enum status_code {
  PLATFORM_STATUS_ERR_UNKNOWN,
  PLATFORM_STATUS_ERR_NOMEM,
  PLATFORM_STATUS_ERR_INVAL,
  PLATFORM_STATUS_OK
} platform_status_code_t;

char *platform_strerror(platform_status_code_t code);
platform_status_code_t platform_map(void *hint, size_t len, void **result);
platform_status_code_t platform_unmap(void *ptr, size_t len);
platform_status_code_t platform_guard(void *ptr, size_t len);
platform_status_code_t platform_unguard(void *ptr, size_t len);
platform_status_code_t platform_get_random(uint32_t *rand);

#endif /* SEALLOC_PLATFORM_API_H_ */
