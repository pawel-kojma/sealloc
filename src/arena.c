#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/container_ll.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/platform_api.h>
#include <sealloc/random.h>
#include <sealloc/utils.h>

void arena_init(arena_t *arena) {
  platform_status_code_t code;
  if ((code = platform_get_random(&arena->secret)) != PLATFORM_STATUS_OK) {
    se_error("Failed to get random value: %s", platform_strerror(code));
  }
  init_splitmix32(arena->secret);
  ll_init(&arena->chunk_list);
  ll_init(&arena->huge_mappings);
  arena->alloc_ptr = 0;
  arena->chunks_left = 0;
}

chunk_t *arena_allocate_chunk(arena_t *arena) {
  chunk_t *chunk_meta = internal_alloc(sizeof(chunk_t));
  platform_status_code_t code;
  size_t map_len = CHUNKS_PER_MAPPING * (CHUNK_SIZE_BYTES + PAGE_SIZE);
  if (chunk_meta == NULL) {
    // End of memory;
    return NULL;
  }

  // get more memory if needed
  if (arena->alloc_ptr == 0 || arena->chunks_left == 0) {
    if ((code = platform_map(NULL, map_len, (void **)&arena->alloc_ptr)) !=
        PLATFORM_STATUS_OK) {
      se_error("Failed to allocate mapping (size : %u): %s", map_len,
               platform_strerror(code));
    }
    arena->chunks_left = CHUNKS_PER_MAPPING;
  }

  // Guard one page after the end of chunk
  if ((code = platform_guard((void *)(arena->alloc_ptr + CHUNK_SIZE_BYTES),
                             PAGE_SIZE)) != PLATFORM_STATUS_OK) {
    se_error("Failed to allocate mapping (size : %u): %s", PAGE_SIZE,
             platform_strerror(code));
  }
  chunk_init(chunk_meta, (void *)arena->alloc_ptr);
  ll_add(&arena->chunk_list, &chunk_meta->entry);
  arena->alloc_ptr += (CHUNK_SIZE_BYTES + PAGE_SIZE);
  arena->chunks_left--;
  return chunk_meta;
}

void arena_deallocate_chunk(arena_t *arena, chunk_t *chunk) {
  platform_status_code_t code;

  // Unmap guard page
  if ((code = platform_unmap(
           (void *)((uintptr_t)chunk->entry.key + CHUNK_SIZE_BYTES),
           PAGE_SIZE)) != PLATFORM_STATUS_OK) {
    se_error("Failed to unmap mapping (size : %u): %s", PAGE_SIZE,
             platform_strerror(code));
  }
  // Delete from chunk list
  ll_del(&arena->chunk_list, &chunk->entry);

  // Free metadata
  internal_free(chunk);
}

chunk_t *arena_get_chunk_from_ptr(arena_t *arena, void *ptr) {
  // ptr is some pointer inside of a chunk
  chunk_t *chunk = NULL;
  ptrdiff_t target = (ptrdiff_t)ptr;
  for (ll_entry_t *entry = arena->chunk_list.ll; entry != NULL;
       entry = entry->link.fd) {
    if ((ptrdiff_t)entry->key <= target &&
        target < (ptrdiff_t)entry->key + CHUNK_SIZE_BYTES) {
      chunk = CONTAINER_OF(entry, chunk_t, entry);
      break;
    }
  }
  return chunk;
}

bin_t *arena_get_bin_by_reg_size(arena_t *arena, unsigned reg_size) { return NULL;}
