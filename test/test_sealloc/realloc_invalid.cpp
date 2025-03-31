#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
}

TEST(ReallocInvalid, InvalidRegion) {
  ASSERT_DEATH(
      { void *reg = sealloc_realloc((void *)42, 16); },
      ".*Invalid call to realloc().*");
}

TEST(ReallocInvalid, FreedRegion) {
  void *reg = sealloc_malloc(16);
  sealloc_free(reg);
  ASSERT_DEATH(
      { void *reg1 = sealloc_realloc(reg, 32); },
      ".*Invalid call to realloc().*");
}

TEST(ReallocInvalid, UnallocatedRegion) {
  void *reg = sealloc_malloc(48);
  ASSERT_DEATH(
      { void *reg1 = sealloc_realloc((void *)((uintptr_t)reg + 48), 32); },
      ".*Invalid call to realloc().*");
}
