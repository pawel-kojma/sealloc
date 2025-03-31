#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
#include <sealloc/utils.h>
}

namespace {
class ReallocHuge : public ::testing::Test {
 protected:
  void *reg, *reg_realloc;
  size_t huge_size = 2097152;  // 2MB
};

TEST_F(ReallocHuge, SmallToHuge) {
  reg = sealloc_malloc(16);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, huge_size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocHuge, HugeToSmall) {
  reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, 16);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocHuge, HugeExpandOneByte) {
  reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, huge_size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocHuge, HugeExpandMultiPage) {
  reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, huge_size + 3 * PAGE_SIZE);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocHuge, HugeTruncate) {
  reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, huge_size - 3 * PAGE_SIZE);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(ReallocHuge, HugeSameSize) {
  reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(reg, huge_size - 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

}  // namespace
