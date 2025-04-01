#include <gtest/gtest.h>

extern "C" {
#include <sealloc.h>
#include <sealloc/arena.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

TEST(MallocApiTest, MultiAllocationSmall) {
  arena_t arena;
  arena_init(&arena);
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(&arena, 16);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(&arena, 16);
}

TEST(MallocApiTest, MultiAllocationMedium) {
  arena_t arena;
  arena_init(&arena);
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(&arena, MEDIUM_SIZE_CLASS_ALIGNMENT);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(&arena, MEDIUM_SIZE_CLASS_ALIGNMENT);
}

TEST(MallocApiTest, MultiAllocationLarge) {
  arena_t arena;
  arena_init(&arena);
  unsigned chunks = 512;
  void *reg[chunks];
  for (int i = 0; i < chunks; i++) {
    reg[i] = sealloc_malloc(&arena, 8192);
    EXPECT_NE(reg[i], nullptr);
  }
  reg[0] = sealloc_malloc(&arena, 8192);
}
