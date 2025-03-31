#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, MallocHuge) {
  size_t huge_size = 2097152;  // 2MB
  void *reg = sealloc_malloc(huge_size);
  EXPECT_NE(reg, nullptr);
}
