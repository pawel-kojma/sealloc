#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
#include <sealloc/utils.h>
#include <sealloc/size_class.h>
}

namespace {
class ReallocRegular : public ::testing::Test {
 protected:
  void *reg, *reg_realloc;
};

TEST_F(ReallocRegular, SmallSameSize) {
  reg = sealloc_malloc(32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, 29);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, SmallExpanded) {
  size_t size = 32;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, SmallTruncated) {
  reg = sealloc_malloc(32);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, 16);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumSameSize) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumExpanded) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, MediumTruncated) {
  size_t size = MEDIUM_SIZE_CLASS_ALIGNMENT;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeSameSize) {
  size_t size = 8192;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeExpanded) {
  size_t size = 8192;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocRegular, LargeTruncated) {
  size_t size = 8192;
  reg = sealloc_malloc(size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, 48);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

}  // namespace
