#include <sealloc/platform_api.h>
#include <stddef.h>

#ifdef __linux__

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static platform_status_code_t get_error_from_errno(void) {
  switch (errno) {
    case EINVAL:
      return PLATFORM_STATUS_ERR_INVAL;
    case ENOMEM:
      return PLATFORM_STATUS_ERR_NOMEM;
    default:
      return PLATFORM_STATUS_ERR_UNKNOWN;
  }
}

char *platform_strerror(platform_status_code_t code) {
  switch (code) {
    case PLATFORM_STATUS_ERR_INVAL:
      return "Invalid arguments passed to function";
    case PLATFORM_STATUS_ERR_NOMEM:
      return "No available memory or mapping cap reached";
    case PLATFORM_STATUS_ERR_UNKNOWN:
      return "Unknown error";
    case PLATFORM_STATUS_OK:
      return "No error";
  }
  return NULL;
}

platform_status_code_t platform_map(void *hint, size_t len, void **result) {
  void *map = mmap(hint, len, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (map == MAP_FAILED) return get_error_from_errno();
  *result = map;
  return PLATFORM_STATUS_OK;
}
platform_status_code_t platform_unmap(void *ptr, size_t len) {
  if (munmap(ptr, len) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}
platform_status_code_t platform_guard(void *ptr, size_t len) {
  if (mprotect(ptr, len, PROT_NONE) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}
platform_status_code_t platform_unguard(void *ptr, size_t len) {
  if (mprotect(ptr, len, PROT_READ | PROT_WRITE) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}

platform_status_code_t platform_get_random(uint32_t *buf) {
  // Manual recommends /dev/urandom for fast random data
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    return get_error_from_errno();
  }
  if (read(fd, buf, sizeof(uint32_t)) < 0) {
    return get_error_from_errno();
  }
  if (close(fd) < 0) {
    return get_error_from_errno();
  }
  return PLATFORM_STATUS_OK;
}

#endif
