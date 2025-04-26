#include <gtest/gtest.h>

#include <random>

extern "C" {
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
#include <sealloc/arena.h>
}

TEST(MallocApiTest, MallocHugeMulti) {
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  std::mt19937 gen{1};
  std::uniform_real_distribution<> dist{1, 10 * PAGE_SIZE};
  size_t huge_size_base = LARGE_SIZE_MAX_REGION, chunk_allocs = 100;
  for (int i = 0; i < chunk_allocs; i++) {
    void *reg = sealloc_malloc(&arena, huge_size_base + dist(gen));
    EXPECT_NE(reg, nullptr);
  }
}
