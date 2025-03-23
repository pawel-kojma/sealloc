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

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_SPLIT = 1,
  NODE_USED_SET_GUARD = 2,
  NODE_USED_FOUND_GUARD = 3,
  NODE_FULL = 4,
  NODE_GUARD = 5,
  NODE_DEPLETED = 6,
  NODE_UNMAPPED = 7
} chunk_node_t;

inline unsigned get_mask(unsigned bits) { return (1 << bits) - 1; }

inline unsigned min(unsigned a, unsigned b) { return a > b ? b : a; }

chunk_node_t get_tree_item(uint8_t *mem, unsigned idx) {
  unsigned bits_to_skip = (idx - 1) * NODE_STATE_BITS;
  unsigned word_idx = bits_to_skip / 8, off = bits_to_skip % 8;
  uint8_t fst_part = (mem[word_idx] >> off) & get_mask(min(8 - off, 3));
  uint8_t snd_part = 0;
  if (NODE_STATE_BITS + off > 8) {
    snd_part = mem[word_idx + 1] & get_mask(off + NODE_STATE_BITS - 8);
    return (chunk_node_t)((snd_part << (8 - off)) | fst_part);
  }
  return (chunk_node_t)fst_part;
}

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
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_USED_SET_GUARD);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_GUARD);
  EXPECT_EQ(chunk->free_mem, CHUNK_SIZE_BYTES - run_size);
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

TEST_F(ChunkUtilsTest, ChunkSingleDeallocate){
  void *alloc;
  unsigned run_size = 2 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size, 16);
  chunk_deallocate_run(chunk, alloc);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_GUARD);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_FREE);
} 

TEST_F(ChunkUtilsTest, ChunkAllocateInGuardPage){
  void *alloc1, *alloc2, *alloc3;
  unsigned run_size = 2 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size, 16);
  alloc2 = chunk_allocate_run(chunk, run_size, 16);
  EXPECT_NE(alloc1, nullptr);
  EXPECT_NE(alloc2, nullptr);
  chunk_deallocate_run(chunk, alloc1);
  alloc3 = chunk_allocate_run(chunk, run_size, 16);
} 
}  // namespace
