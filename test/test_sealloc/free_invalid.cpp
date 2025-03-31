#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(Sealloc, FreeInvalid) {
  void *reg = sealloc_malloc(16);
  ASSERT_NE(reg, nullptr);
  sealloc_free(reg);
  EXPECT_DEATH({ sealloc_free(reg); }, ".*Invalid call to free().*");
}
