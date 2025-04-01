#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

namespace {
class ReallocRegular : public ::testing::Test {
 protected:
  void *reg, *reg_realloc;
  arena_t arena;

  void SetUp() override { arena_init(&arena); }
};

TEST_F(ReallocRegular, SmallSameSize) {
  reg = sealloc_malloc(&arena, 32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 29);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, SmallExpanded) {
  size_t size = 32;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, SmallTruncated) {
  reg = sealloc_malloc(&arena, 32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 16);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumSameSize) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumExpanded) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumTruncated) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeSameSize) {
  size_t size = 8192;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeExpanded) {
  size_t size = 8192;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeTruncated) {
  size_t size = 8192;
  reg = sealloc_malloc(&arena, size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

}  // namespace
