#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/sealloc.h>
#include <sealloc/arena.h>
#include <sealloc/random.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

class MallocApiTest : public testing::Test {
 protected:
  arena_t arena;
  void SetUp() override { arena_init(&arena); }
};

TEST(MallocApiTest, SingeAllocation) {
  arena_t arena;
  arena_init(&arena);
  void *reg = sealloc_malloc(&arena, 16);
  EXPECT_NE(reg, nullptr);
}
