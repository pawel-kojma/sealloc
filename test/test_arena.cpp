#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/chunk.h>
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
  void SetUp() override { arena_init(&arena); }
};

TEST_F(ArenaUtilsTest, ArenaChunkAllocate) {
  chunk_t *chunk = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaChunkDeallocate) {
  chunk_t *chunk1, *chunk2, *chunk3;
  chunk1 = arena_allocate_chunk(&arena);
  chunk2 = arena_allocate_chunk(&arena);
  chunk3 = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk1, nullptr);
  EXPECT_NE(chunk2, nullptr);
  EXPECT_NE(chunk3, nullptr);
}
}  // namespace
