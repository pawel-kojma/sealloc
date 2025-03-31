#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, FreeRegular) {
  void *reg = sealloc_malloc(16);
  ASSERT_NE(reg, nullptr);
  sealloc_free(reg);
}
