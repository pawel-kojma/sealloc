#include <gtest/gtest.h>

extern "C" {
#include <sealloc/sealloc.h>
#include <sealloc/arena.h>
}

TEST(MallocApiTest, MallocHugeSingle) {
  arena_t arena;
  arena_init(&arena);
  size_t huge_size = 2097152;  // 2MB
  void *reg = sealloc_malloc(&arena, huge_size);
  EXPECT_NE(reg, nullptr);
}
