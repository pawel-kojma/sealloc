#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

TEST(MallocApiTest, CorrectBinStateSmall) {
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  unsigned chunks = CHUNK_LEAST_REGION_SIZE_BYTES / SMALL_SIZE_MIN_REGION;
  void *reg;
  for (int i = 0; i < chunks - BIN_MINIMUM_REGIONS; i++) {
    reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
    EXPECT_NE(reg, nullptr);
    EXPECT_EQ(arena.bins[0].run_list_active_cnt, 1);
    EXPECT_EQ(arena.bins[0].avail_regs, chunks - i - 1);
  }

  EXPECT_EQ(arena.bins[0].avail_regs, BIN_MINIMUM_REGIONS);
  reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[0].avail_regs, BIN_MINIMUM_REGIONS - 1);
  EXPECT_EQ(arena.bins[0].run_list_active_cnt, 1);

  reg = sealloc_malloc(&arena, SMALL_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[0].avail_regs, BIN_MINIMUM_REGIONS - 1 + chunks - 1);
  EXPECT_EQ(arena.bins[0].run_list_active_cnt, 2);
}

TEST(MallocApiTest, CorrectBinStateMedium) {
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  unsigned chunks = CHUNK_LEAST_REGION_SIZE_BYTES / MEDIUM_SIZE_MIN_REGION;
  void *reg;
  unsigned min_run_lists = BIN_MINIMUM_REGIONS / chunks;
  size_t base = NO_SMALL_SIZE_CLASSES;
  size_t idx = base + size_to_idx_medium(MEDIUM_SIZE_MIN_REGION);
  reg = sealloc_malloc(&arena, MEDIUM_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[idx].run_list_active_cnt, min_run_lists);
  EXPECT_EQ(arena.bins[idx].avail_regs, BIN_MINIMUM_REGIONS - 1);

  reg = sealloc_malloc(&arena, MEDIUM_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[idx].run_list_active_cnt, min_run_lists + 1);
  EXPECT_EQ(arena.bins[idx].avail_regs, BIN_MINIMUM_REGIONS - 1 + chunks - 1);
}

TEST(MallocApiTest, CorrectBinStateLarge) {
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  void *reg;
  size_t base = NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES;
  size_t idx = base + size_to_idx_large(LARGE_SIZE_MIN_REGION);
  reg = sealloc_malloc(&arena, LARGE_SIZE_MIN_REGION);
  EXPECT_NE(reg, nullptr);
  EXPECT_EQ(arena.bins[idx].run_list_active_cnt, BIN_MINIMUM_REGIONS - 1);
  EXPECT_EQ(arena.bins[idx].avail_regs, BIN_MINIMUM_REGIONS - 1);

  reg = sealloc_malloc(&arena, LARGE_SIZE_MIN_REGION);
  EXPECT_EQ(arena.bins[idx].run_list_active_cnt, BIN_MINIMUM_REGIONS - 1);
  EXPECT_EQ(arena.bins[idx].avail_regs, BIN_MINIMUM_REGIONS - 1);
}
