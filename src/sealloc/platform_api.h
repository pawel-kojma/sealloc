/* API for system specific functions */

#ifndef SEALLOC_PLATFORM_API_H_
#define SEALLOC_PLATFORM_API_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * @brief Platform error return codes for functions.
 */
typedef enum status_code {
  PLATFORM_STATUS_ERR_UNKNOWN,
  PLATFORM_STATUS_ERR_NOMEM,
  PLATFORM_STATUS_ERR_INVAL,
  PLATFORM_STATUS_OK,
  PLATFORM_STATUS_CEILING_HIT,
  PLATFORM_DIFFERENT_ADDRESS
} platform_status_code_t;

#ifdef __aarch64__
extern int is_mte_enabled;
#endif

/*!
 * @brief Converts code to string which describes error
 *
 * @param[in] code
 * @return error description.
 */
const char *platform_strerror(platform_status_code_t code);

/*!
 * @brief Allocates a mapping from the operating system.
 *
 * @param[in] hint
 * @param[in] len Page-aligned length of requested mapping
 * @param[in,out] result storage for mapping result
 * @return error code.
 * @post *result points to allocated mapping iff return code is
 * PLATFORM_STATUS_OK
 */
platform_status_code_t platform_map(void *hint, size_t len, void **result);

/*!
 * @brief Gets program break.
 *
 * @param[in,out] result storage for mapping result
 * @return error code.
 * @post *result points to program break iff return code is
 * PLATFORM_STATUS_OK
 */
platform_status_code_t platform_get_program_break(void **result);

/*!
 * @brief Allocates a mapping from the operating system, by probing
 * incrementally.
 *
 * @param[in, out] probe Start probe parameter, updated to new mapping address
 * @param[in] ceiling Maximum address for a mapping (exclusive).
 * @param[in] len Length of requested mapping
 * @return error code.
 * @pre probe is page-aligned
 * @pre len is page-aligned
 */
platform_status_code_t platform_map_probe(volatile uintptr_t *probe, uintptr_t ceiling,
                                          size_t len);

/*!
 * @brief Unmaps/decommits page-aligned piece of memory
 *
 * @param[in] ptr Pointer to unmap form
 * @param[in] len Page-aligned mapping length
 * @return error code.
 */
platform_status_code_t platform_unmap(void *ptr, size_t len);

/*!
 * @brief Guard previously mapped memory
 *
 * @param[in] ptr Pointer to existing mapping
 * @param[in] len Page-aligned mapping length to guard
 * @return error code.
 */
platform_status_code_t platform_guard(void *ptr, size_t len);

/*!
 * @brief Sets default (rw-) permissions for the mapping
 *
 * @param[in] ptr Pointer to existing mapping
 * @param[in] len Page-aligned mapping length to unguard
 * @return error code.
 */
platform_status_code_t platform_unguard(void *ptr, size_t len);

/*!
 * @brief Get random 32-bit unsigned integer from OS
 *
 * @param[out] rand Storage for random integer
 * @return error code.
 * @post *rand contains random integer iff error code is PLATFORM_STATUS_OK
 */
platform_status_code_t platform_get_random(uint32_t *rand);

#endif /* SEALLOC_PLATFORM_API_H_ */
