#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
#include <sealloc/utils.h>
}

TEST(Sealloc, MultiAllocationSmall) {
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(16);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(16);
}

TEST(Sealloc, MultiAllocationMedium) {
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(16);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(16);
}

TEST(Sealloc, MultiAllocationLarge) {
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(16);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(16);
}
