#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/chunk.h>
#include <sealloc/utils.h>
}

TEST(ChunkUtils, ChunkInit) {
  chunk_t chunk;
  void *heap = nullptr;
  chunk_init(&chunk, heap);
}

namespace {
class ChunkUtilsTest : public ::testing::Test {
 protected:
  chunk_t *chunk;
  void *heap;
  void SetUp() override {
    chunk = (chunk_t *)malloc(sizeof(chunk_t));
    heap = malloc(16);
  }
};

template <class T>
bool all_unique(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  return std::adjacent_find(v.begin(), v.end()) == v.end();
}

TEST_F(ChunkUtilsTest, ChunkSingleAllocation) {
  void *alloc;
  unsigned run_size = 2 * PAGE_SIZE;
  unsigned reg_size = 16;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size, reg_size);
  EXPECT_NE(alloc, nullptr);
}

TEST_F(ChunkUtilsTest, ChunkManyAllocations) {
  constexpr unsigned CHUNKS = 100;
  std::vector<std::pair<size_t, size_t>> run_reg_sizes = {
      {2 * PAGE_SIZE, 16},
      {2 * PAGE_SIZE, 32},
      {2 * PAGE_SIZE, 48},
      {2 * PAGE_SIZE, 1024},
      {4 * PAGE_SIZE, 16384}};
  std::vector<void *> chunks;
  chunks.reserve(CHUNKS);
  std::srand(123);
  chunk_init(chunk, heap);
  for (int i = 0; i < CHUNKS; i++) {
    auto [run_size, reg_size] = run_reg_sizes[rand() % run_reg_sizes.size()];
    chunks[i] = chunk_allocate_run(chunk, run_size, reg_size);
    EXPECT_NE(chunks[i], nullptr) << "Failed at " << i << " idx\n";
  }
  EXPECT_TRUE(all_unique(chunks));
}

TEST_F(ChunkUtilsTest, ChunkRegSizeArrayUpdate) {
  void *alloc;
  unsigned run_size = 2 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size, 16);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size, 32);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size, 48);
  EXPECT_NE(alloc, nullptr);
  EXPECT_EQ(chunk->reg_size_small_medium[0], 1);
  EXPECT_EQ(chunk->reg_size_small_medium[2], 2);
  EXPECT_EQ(chunk->reg_size_small_medium[4], 3);
  EXPECT_EQ(chunk->reg_size_small_medium[1], 0xff);
  EXPECT_EQ(chunk->reg_size_small_medium[3], 0xff);
  EXPECT_EQ(chunk->reg_size_small_medium[5], 0xff);
}

}  // namespace
