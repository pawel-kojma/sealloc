#include <gtest/gtest.h>

extern "C" {
#include <sealloc/sealloc.h>
#include <sealloc/arena.h>
}

TEST(ReallocInvalid, InvalidRegion) {
  arena_t arena;
  arena_init(&arena);
  ASSERT_DEATH(
      { void *reg = sealloc_realloc(&arena, (void *)42, 16); },
      ".*Invalid call to realloc().*");
}

TEST(ReallocInvalid, FreedRegion) {
  arena_t arena;
  arena_init(&arena);
  void *reg = sealloc_malloc(&arena, 16);
  sealloc_free(&arena, reg);
  ASSERT_DEATH(
      { void *reg1 = sealloc_realloc(&arena, reg, 32); },
      ".*Invalid call to realloc().*");
}

TEST(ReallocInvalid, UnallocatedRegion) {
  arena_t arena;
  arena_init(&arena);
  void *reg = sealloc_malloc(&arena, 48);
  ASSERT_DEATH(
      {
        void *reg1 = sealloc_realloc(&arena, (void *)((uintptr_t)reg + 48), 32);
      },
      ".*Invalid call to realloc().*");
}
