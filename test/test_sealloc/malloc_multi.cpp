#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

TEST(MallocApiTest, MultiAllocationSmall) {
  arena_t arena;
  arena_init(&arena);
  unsigned chunks = 512;
  void *reg;
  for (int i = 0; i < chunks - 1; i++) {
    reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
    EXPECT_NE(reg, nullptr);
    EXPECT_NE(arena.bins[0].runcur, nullptr);
  }

  reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[0].runcur, nullptr);

  reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
  EXPECT_NE(arena.bins[0].runcur, nullptr);
  EXPECT_NE(arena.bins[0].runcur, nullptr);
}

TEST(MallocApiTest, MultiAllocationMedium) {
  arena_t arena;
  arena_init(&arena);
  unsigned chunks = CHUNK_LEAST_REGION_SIZE_BYTES / MEDIUM_SIZE_MIN_REGION;
  void *reg;
  size_t base = NO_SMALL_SIZE_CLASSES;
  for (int i = 0; i < chunks - 1; i++) {
    reg = sealloc_malloc(&arena, MEDIUM_SIZE_MIN_REGION);
    EXPECT_NE(reg, nullptr);
    EXPECT_NE(arena.bins[base + SIZE_TO_IDX_MEDIUM(MEDIUM_SIZE_MIN_REGION)].runcur,
              nullptr);
  }

  reg = sealloc_malloc(&arena, MEDIUM_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[base + SIZE_TO_IDX_MEDIUM(MEDIUM_SIZE_MIN_REGION)].runcur,
            nullptr);

  reg = sealloc_malloc(&arena, MEDIUM_SIZE_MIN_REGION);
  EXPECT_NE(arena.bins[base + SIZE_TO_IDX_MEDIUM(MEDIUM_SIZE_MIN_REGION)].runcur,
            nullptr);
}

TEST(MallocApiTest, MultiAllocationLarge) {
  arena_t arena;
  arena_init(&arena);
  void *reg;
  size_t base = NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES;
  reg = sealloc_malloc(&arena, LARGE_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[base + size_to_idx_large(LARGE_SIZE_MIN_REGION)].runcur,
            nullptr);
  reg = sealloc_malloc(&arena, LARGE_SIZE_MIN_REGION);
  EXPECT_EQ(arena.bins[base + size_to_idx_large(LARGE_SIZE_MIN_REGION)].runcur,
            nullptr);
}
