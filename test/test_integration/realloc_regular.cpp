#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

namespace {
class MallocApiTest : public ::testing::Test {
 protected:
  void *reg, *reg_realloc;
  arena_t arena;

  void SetUp() override { arena_init(&arena); }
};

TEST_F(MallocApiTest, ReallocSmallSameSize) {
  reg = sealloc_malloc(&arena, 32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 29);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocSmallExpanded) {
  size_t size = 32;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocSmallTruncated) {
  reg = sealloc_malloc(&arena, 32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 16);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocMediumSameSize) {
  size_t size = MEDIUM_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocMediumExpanded) {
  size_t size = MEDIUM_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocMediumTruncated) {
  size_t size = MEDIUM_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocLargeSameSize) {
  size_t size = LARGE_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocLargeExpanded) {
  size_t size = LARGE_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocLargeTruncated) {
  size_t size = LARGE_SIZE_MIN_REGION;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

}  // namespace
