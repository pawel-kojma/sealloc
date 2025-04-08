#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
}

TEST(MallocApiTest, FreeInvalid) {
  arena_t arena;
  arena_init(&arena);
  void *reg = sealloc_malloc(&arena, 16);
  ASSERT_NE(reg, nullptr);
  sealloc_free(&arena, reg);
  EXPECT_DEATH({ sealloc_free(&arena, reg); }, ".*Invalid call to free().*");
}
