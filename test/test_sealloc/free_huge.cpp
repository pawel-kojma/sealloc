#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
}

TEST(MallocApiTest, FreeHuge) {
  arena_t arena;
  arena_init(&arena);
  size_t huge_size = 2097152;  // 2MB
  void *reg = sealloc_malloc(&arena, huge_size);
  ASSERT_NE(reg, nullptr);
  sealloc_free(&arena, reg);
}
