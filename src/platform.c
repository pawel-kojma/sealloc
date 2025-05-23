#include <assert.h>
#include <stddef.h>

#include "sealloc/logging.h"
#include "sealloc/platform_api.h"
#include "sealloc/utils.h"

#ifdef __linux__

#include <errno.h>
#include <fcntl.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#ifdef __aarch64__

#include "sealloc/arch/aarch64.h"

int is_mte_enabled = 0;

static int additional_prot_flags = 0;

__attribute__((constructor)) void init_aarch64(void) {
  unsigned long hwcap2 = getauxval(AT_HWCAP2);
  if (hwcap2 & HWCAP2_MTE) {
    if (prctl(PR_SET_TAGGED_ADDR_CTRL,
              PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC |
                  (PLATFORM_AARCH64_TAGS << PR_MTE_TAG_SHIFT),
              0, 0, 0)) {
      se_debug("MTE supported but prctl() failed");
    } else {
      se_debug("MTE support activated");
      additional_prot_flags = PROT_MTE;
      is_mte_enabled = 1;
    }
  } else {
    se_debug("MTE is not supported");
  }
}

#elif
static int additional_prot_flags = 0;
#endif

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

const char *platform_strerror(platform_status_code_t code) {
  switch (code) {
    case PLATFORM_STATUS_ERR_INVAL:
      return "Invalid arguments passed to function";
    case PLATFORM_STATUS_ERR_NOMEM:
      return "No available memory or mapping cap reached";
    case PLATFORM_STATUS_ERR_UNKNOWN:
      return "Unknown error";
    case PLATFORM_STATUS_OK:
      return "No error";
    case PLATFORM_STATUS_CEILING_HIT:
      return "Maximum address hit";
    case PLATFORM_DIFFERENT_ADDRESS:
      return "kernel created mapping under different address than specified";
  }
  return NULL;
}

platform_status_code_t platform_map(void *hint, size_t len, void **result) {
  assert(len > 0);
  void *map = mmap(hint, len, PROT_READ | PROT_WRITE | additional_prot_flags,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (map == MAP_FAILED) return get_error_from_errno();
  *result = map;
  se_debug("Mapping (hint : %p, len : %zu, result : %p)", hint, len, *result);
  return PLATFORM_STATUS_OK;
}

platform_status_code_t platform_get_program_break(void **result) {
  *result = sbrk(0);
  se_debug("Getting program break : %p", *result);
  if (*result == (void *)-1) {
    return get_error_from_errno();
  }
  return PLATFORM_STATUS_OK;
}
platform_status_code_t platform_map_probe(volatile uintptr_t *probe, uintptr_t ceiling,
                                          size_t len) {
  assert(len > 0);
  assert(IS_ALIGNED(len, PAGE_SIZE));
  assert(IS_ALIGNED(*probe, PAGE_SIZE));
  void *map = NULL;
  uintptr_t new_probe = *probe;

  if (new_probe >= ceiling) {
    return PLATFORM_STATUS_CEILING_HIT;
  }
  while (map == NULL) {
    map = mmap((void *)new_probe, len,
               PROT_READ | PROT_WRITE | additional_prot_flags,
               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    se_debug("Mapping (probe : %p, len : %zu, result : %p)", (void *)new_probe,
             len, map);
    if (map == MAP_FAILED) {
      if (errno == EEXIST) {
        /* We hit a mapping, try incrementing probe */
        se_debug("Existing mapping hit");
        new_probe += PAGE_SIZE;
        if (new_probe >= ceiling) {
          return PLATFORM_STATUS_CEILING_HIT;
        }
        map = NULL;
      } else
        return get_error_from_errno();
    }
  }
  if ((uintptr_t)map != new_probe) {
    // Kernel seems to ignore MAP_FIXED_NOREPLACE, but rarely
    se_debug("map : %p, new_probe %p", map, (void *)new_probe);
    *probe = (uintptr_t)map;
    return PLATFORM_DIFFERENT_ADDRESS;
  }
  *probe = (uintptr_t)map;
  return PLATFORM_STATUS_OK;
}

platform_status_code_t platform_unmap(void *ptr, size_t len) {
  assert(len > 0);
  se_debug("Unmapping (ptr : %p, len : %zu)", ptr, len);
  if (munmap(ptr, len) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}
platform_status_code_t platform_guard(void *ptr, size_t len) {
  assert(len > 0);
  se_debug("Guarding (ptr : %p, len : %zu)", ptr, len);
  if (mprotect(ptr, len, PROT_NONE) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}
platform_status_code_t platform_unguard(void *ptr, size_t len) {
  assert(len > 0);
  se_debug("Unguarding (ptr : %p, len : %zu)", ptr, len);
  if (mprotect(ptr, len, PROT_READ | PROT_WRITE) == 0) {
    return PLATFORM_STATUS_OK;
  }
  return get_error_from_errno();
}
platform_status_code_t platform_get_random(uint32_t *buf) {
  // Manual recommends /dev/urandom for fast random data
  se_debug("Getting random value to (ptr : %p)", buf);
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
  se_debug("Got random value : %u", *buf);
  return PLATFORM_STATUS_OK;
}

#endif
