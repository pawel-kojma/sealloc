#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
}

class MallocApiTest : public testing::Test {
 protected:
  arena_t arena;
  void SetUp() override { arena_init(&arena); }
};

TEST_F(MallocApiTest, CallocSimple) {
  int *reg = (int *)sealloc_calloc(&arena, 2, 16);
  EXPECT_NE(reg, nullptr);
  for (int i = 0; i < 8; i++) {
    reg[i] = i + 1;
  }
  for (int i = 0; i < 8; i++) {
    EXPECT_EQ(reg[i], i + 1);
  }
}

TEST_F(MallocApiTest, CallocBadArgs) {
  int *reg = (int *)sealloc_calloc(&arena, 0, 16);
  EXPECT_EQ(reg, nullptr);
  int *reg1 = (int *)sealloc_calloc(&arena, 2, 0);
  EXPECT_EQ(reg, nullptr);
}
