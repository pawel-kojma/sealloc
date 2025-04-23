#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/platform_api.h>
#include <sealloc/random.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

TEST(MallocApiTest, HugeInChunkHandling) {
  std::vector<void *> large;
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  large.push_back(sealloc_malloc(&arena, LARGE_SIZE_MAX_REGION));
  void *chunk_ptr = arena.chunk_list.ll->key;
  EXPECT_NE(chunk_ptr, nullptr);
  for (int i = 0; i < 4 * (CHUNK_SIZE_BYTES / LARGE_SIZE_MAX_REGION); i++) {
    large.push_back(sealloc_malloc(&arena, LARGE_SIZE_MAX_REGION));
  }
  std::sort(large.begin(), large.end());
  for (int i = 0; i < 8; i++) {
    // Enough to unmap memory
    sealloc_free(&arena, large[i]);
  }
  arena.huge_alloc_ptr = (uintptr_t)chunk_ptr;
  void *huge = sealloc_malloc(&arena, 2 * LARGE_SIZE_MAX_REGION);
  EXPECT_TRUE((uintptr_t)huge >= (uintptr_t)chunk_ptr &&
              (uintptr_t)huge < (uintptr_t)chunk_ptr + CHUNK_SIZE_BYTES);
  chunk_t *chunk;
  bin_t *bin;
  run_t *run;
  huge_chunk_t *huge_m;
  EXPECT_EQ(locate_metadata_for_ptr(&arena, huge, &chunk, &run, &bin, &huge_m),
            METADATA_HUGE);
}
