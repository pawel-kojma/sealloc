#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/utils.h>
#include <sealloc/size_class.h>
}

TEST(ArenaUtils, ArenaInit) {
  arena_t arena;
  arena_init(&arena);
}

namespace {
class ArenaUtilsTest : public ::testing::Test {
 protected:
  arena_t arena;
  unsigned run_size_small = 2 * PAGE_SIZE;
  size_t huge_chunk_size = 2097152;  // 2MB
  void SetUp() override { arena_init(&arena); }

  void exhaust_chunk(chunk_t *chunk) {
    unsigned chunks_to_alloc = CHUNK_NO_NODES_LAST_LAYER / 2;
    void *chunk_alloc[chunks_to_alloc];
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_deallocate_run(chunk, chunk_alloc[i]);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_deallocate_run(chunk, chunk_alloc[i]);
    }
  }

  uintptr_t get_bin_idx(bin_t *bin) {
    return ((uintptr_t)bin - (uintptr_t)&arena.bins) / sizeof(bin_t);
  }
};

TEST_F(ArenaUtilsTest, ArenaChunkAllocate) {
  chunk_t *chunk = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaChunkDeallocate) {
  chunk_t *chunk1;
  chunk1 = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk1, nullptr);
  exhaust_chunk(chunk1);
  arena_deallocate_chunk(&arena, chunk1);
}

TEST_F(ArenaUtilsTest, ArenaGetChunkFromPtr) {
  chunk_t *chunk1, *chunk2, *chunk3;

  chunk1 = arena_allocate_chunk(&arena);
  chunk2 = arena_allocate_chunk(&arena);
  chunk3 = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk1, nullptr);
  EXPECT_NE(chunk2, nullptr);
  EXPECT_NE(chunk3, nullptr);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk1->entry.key), chunk1);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk2->entry.key), chunk2);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk3->entry.key), chunk3);
  EXPECT_EQ(arena_get_chunk_from_ptr(
                &arena, (void *)((uintptr_t)chunk1->entry.key + 42)),
            chunk1);
  EXPECT_NE(arena_get_chunk_from_ptr(
                &arena, (void *)((uintptr_t)chunk3->entry.key - 1)),
            chunk3);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, (void *)(0x42)), nullptr);
}

TEST_F(ArenaUtilsTest, ArenaGetBinByRegSize) {
  bin_t *small, *medium, *large1, *large2;
  small = arena_get_bin_by_reg_size(&arena, SMALL_SIZE_MIN_REGION);
  medium = arena_get_bin_by_reg_size(&arena, MEDIUM_SIZE_MIN_REGION);
  large1 = arena_get_bin_by_reg_size(&arena, 2 * PAGE_SIZE);
  large2 = arena_get_bin_by_reg_size(&arena, 4 * PAGE_SIZE);
  EXPECT_EQ(get_bin_idx(small), 0);
  EXPECT_EQ(get_bin_idx(medium), NO_SMALL_SIZE_CLASSES);
  EXPECT_EQ(get_bin_idx(large1), NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES);
  EXPECT_EQ(get_bin_idx(large2),
            NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES + 1);
}

TEST_F(ArenaUtilsTest, ArenaAllocateHugeChunk) {
  void *huge_chunk;
  huge_chunk = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  EXPECT_NE(huge_chunk, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaDeallocateHugeChunk) {
  void *huge_chunk;
  huge_chunk = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  EXPECT_NE(huge_chunk, nullptr);
  arena_deallocate_huge_mapping(&arena, huge_chunk, huge_chunk_size);
}

TEST_F(ArenaUtilsTest, ArenaFindHugeMapping) {
  huge_chunk_t h1_expect, h2_expect, *h1, *h2;
  h1_expect.entry.key = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  h1_expect.len = huge_chunk_size;
  arena_store_huge_meta(&arena, &h1_expect);
  h2_expect.entry.key = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  h2_expect.len = huge_chunk_size;
  arena_store_huge_meta(&arena, &h2_expect);
  h1 = arena_find_huge_mapping(&arena, h1_expect.entry.key);
  h2 = arena_find_huge_mapping(&arena, h2_expect.entry.key);
  EXPECT_EQ(h1, &h1_expect);
  EXPECT_EQ(h2, &h2_expect);
}

TEST_F(ArenaUtilsTest, ArenaAllocateRun) {
  bin_t *bin = arena_get_bin_by_reg_size(&arena, SMALL_SIZE_CLASS_ALIGNMENT);
  run_t *run = arena_allocate_run(&arena, bin);
  EXPECT_NE(run, nullptr);
}

}  // namespace
