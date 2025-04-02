#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
#include <sealloc/utils.h>
}

namespace {
class MallocApiTest : public ::testing::Test {
 protected:
  void *reg, *reg_realloc;
  size_t huge_size = 2097152;  // 2MB
  arena_t arena;

  void SetUp() override { arena_init(&arena); }
};

TEST_F(MallocApiTest, SmallToHuge) {
  reg = sealloc_malloc(&arena, 16);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, huge_size);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocHugeToSmall) {
  reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, 16);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocHugeExpandOneByte) {
  reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, huge_size + 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocHugeExpandMultiPage) {
  reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, huge_size + 3 * PAGE_SIZE);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocHugeTruncate) {
  reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, huge_size - 3 * PAGE_SIZE);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_NE(reg_realloc, reg);
}

TEST_F(MallocApiTest, ReallocHugeSameSize) {
  GTEST_SKIP() << "For now this optimization is not enabled";
  reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  reg_realloc = sealloc_realloc(&arena, reg, huge_size - 1);
  EXPECT_NE(reg_realloc, nullptr);
  EXPECT_EQ(reg_realloc, reg);
}

}  // namespace
