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

TEST(MallocApiTest, HugeInChunkHandlingUnmappedNode) {
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

TEST(MallocApiTest, InvalidFreeHandling) {
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
  void *invalid_ptr = large[0];
  chunk_t *chunk;
  bin_t *bin;
  run_t *run;
  huge_chunk_t *huge_m;
  EXPECT_EQ(
      locate_metadata_for_ptr(&arena, invalid_ptr, &chunk, &run, &bin, &huge_m),
      METADATA_INVALID);
}

TEST(MallocApiTest, HugeInChunkHandlingWithDepletedNodes) {
  std::vector<void *> large;
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  unsigned second_smallest_large =
      alignup_large_size(LARGE_SIZE_MIN_REGION + 1);
  large.push_back(sealloc_malloc(&arena, second_smallest_large));
  void *chunk_ptr = arena.chunk_list.ll->key;
  EXPECT_NE(chunk_ptr, nullptr);
  for (int i = 0; i < 20 * (CHUNK_SIZE_BYTES / second_smallest_large); i++) {
    large.push_back(sealloc_malloc(&arena, second_smallest_large));
  }
  std::sort(large.begin(), large.end());
  for (int i = 0; i < 64; i++) {
    // Enough to unmap memory
    sealloc_free(&arena, large[i]);
  }
  arena.huge_alloc_ptr = (uintptr_t)chunk_ptr;
  void *huge = sealloc_malloc(&arena, ALIGNUP_PAGE(LARGE_SIZE_MAX_REGION + 1));
  EXPECT_TRUE((uintptr_t)huge >= (uintptr_t)chunk_ptr &&
              (uintptr_t)huge < (uintptr_t)chunk_ptr + CHUNK_SIZE_BYTES);
  chunk_t *chunk;
  bin_t *bin;
  run_t *run;
  huge_chunk_t *huge_m;
  EXPECT_EQ(locate_metadata_for_ptr(&arena, huge, &chunk, &run, &bin, &huge_m),
            METADATA_HUGE);
}

TEST(MallocApiTest, InvalidFreeHandlingWithDepletedNodes) {
  std::vector<void *> large;
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  unsigned second_smallest_large =
      alignup_large_size(LARGE_SIZE_MIN_REGION + 1);
  large.push_back(sealloc_malloc(&arena, second_smallest_large));
  void *chunk_ptr = arena.chunk_list.ll->key;
  EXPECT_NE(chunk_ptr, nullptr);
  for (int i = 0; i < 20 * (CHUNK_SIZE_BYTES / second_smallest_large); i++) {
    large.push_back(sealloc_malloc(&arena, second_smallest_large));
  }
  std::sort(large.begin(), large.end());
  for (int i = 0; i < 64; i++) {
    // Enough to unmap memory
    sealloc_free(&arena, large[i]);
  }
  chunk_t *chunk;
  bin_t *bin;
  run_t *run;
  huge_chunk_t *huge_m;
  EXPECT_EQ(
      locate_metadata_for_ptr(&arena, large[0], &chunk, &run, &bin, &huge_m),
      METADATA_INVALID);
}
