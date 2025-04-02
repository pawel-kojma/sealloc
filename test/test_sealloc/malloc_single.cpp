#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/random.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

class MallocApiTest : public testing::Test {
 protected:
  arena_t arena;
  void SetUp() override { arena_init(&arena); }
};

TEST_F(MallocApiTest, SingeAllocation) {
  void *reg = sealloc_malloc(&arena, 16);
  EXPECT_NE(reg, nullptr);
}

TEST_F(MallocApiTest, LargestAllocationIsNotHuge) {
  void *reg = sealloc_malloc(&arena, LARGE_SIZE_MAX_REGION);
  EXPECT_NE(reg, nullptr);
  unsigned skip_bins = NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES;
  EXPECT_EQ(
      arena.bins[skip_bins + size_to_idx_large(LARGE_SIZE_MAX_REGION)].reg_size,
      LARGE_SIZE_MAX_REGION);
}
