
#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, SingeAllocation) {
  void *reg = sealloc_malloc(16);
  EXPECT_NE(reg, nullptr);
}

TEST(Sealloc, MultiAllocation) {
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(16);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(16);
}
