#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/utils.h>
}


namespace {
class InternalAllocatorTest : public ::testing::Test {
 protected:
  int_alloc_t ia;
  void SetUp() override { internal_allocator_init(&ia); }
};

TEST_F(InternalAllocatorTest, BasicAllocations) {
  void *a, *b, *c, *d;
  a = internal_alloc(&ia, 16);
  b = internal_alloc(&ia, 16);
  EXPECT_NE(a, nullptr);
  EXPECT_NE(b, nullptr);
  internal_free(&ia, a);
  internal_free(&ia, b);
  c = internal_alloc(&ia, 16);
  d = internal_alloc(&ia, 16);
  EXPECT_EQ(a, c);
  EXPECT_EQ(b, d);
}

TEST_F(InternalAllocatorTest, Exhaust) {
  void *a, *b;
  a = internal_alloc(&ia, INTERNAL_ALLOC_CHUNK_SIZE_BYTES);
  b = internal_alloc(&ia, 16);
  EXPECT_NE(a, nullptr);
  EXPECT_EQ(b, nullptr);
}

}  // namespace
