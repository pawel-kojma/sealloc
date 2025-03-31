#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, FreeHuge) {
  size_t huge_size = 2097152;  // 2MB
  void *reg = sealloc_malloc(huge_size);
  ASSERT_NE(reg, nullptr);
  sealloc_free(reg);
}
