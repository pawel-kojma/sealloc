#include <gtest/gtest.h>

#include <random>

extern "C" {
#include <sealloc.h>
#include <sealloc/size_class.h>
}

TEST(Sealloc, MallocHugeMulti) {
  std::mt19937 gen{1};
  std::uniform_real_distribution<> dist{1, 10 * PAGE_SIZE};
  size_t huge_size_base = LARGE_SIZE_MAX_REGION, chunk_allocs = 100;
  for (int i = 0; i < chunk_allocs; i++) {
    void *reg = sealloc_malloc(huge_size_base + dist(gen));
    EXPECT_NE(reg, nullptr);
  }
}
