#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, SingeAllocation) {
  void *reg = sealloc_malloc(16);
  EXPECT_NE(reg, nullptr);
}
