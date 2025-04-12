#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/chunk.h>
#include <sealloc/platform_api.h>
#include <sealloc/utils.h>
}

TEST(ChunkUtils, ChunkInit) {
  chunk_t chunk;
  void *heap = (void *)1;
  chunk_init(&chunk, heap);
}

namespace {
class ChunkUtilsTest : public ::testing::Test {
 protected:
  chunk_t *chunk;
  void *heap;
  unsigned run_size_small = 2 * PAGE_SIZE;
  unsigned run_size_large = 4 * PAGE_SIZE;
  void SetUp() override {
    chunk = (chunk_t *)malloc(sizeof(chunk_t));
    platform_map(NULL, CHUNK_SIZE_BYTES, &heap);
    init_splitmix32(1);
    chunk_init(chunk, heap);
  }
};

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_USED = 1,
  NODE_DEPLETED = 2,
  NODE_UNMAPPED = 3
} chunk_node_t;

inline unsigned get_mask(unsigned bits) { return (1 << bits) - 1; }

inline unsigned min(unsigned a, unsigned b) { return a > b ? b : a; }

static jump_node_t get_jt_item(jump_node_t *mem, unsigned idx) {
  return mem[idx - 1];
}

static chunk_node_t get_buddy_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (chunk_node_t)(mem[word] >> (2 * off)) & 3;
}

template <class T>
bool all_unique(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  return std::adjacent_find(v.begin(), v.end()) == v.end();
}

TEST_F(ChunkUtilsTest, ChunkSingleAllocation) {
  void *alloc;
  unsigned reg_size = 16;
  alloc = chunk_allocate_run(chunk, run_size_small, reg_size);
  EXPECT_NE(alloc, nullptr);
  // 1580013426 % 4096 == 1906
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 4096 + 1906), NODE_USED);
  jump_node_t node = get_jt_item(chunk->jump_tree, 4096 + 1906);
  EXPECT_EQ(node->prev, 0);
  EXPECT_EQ(node->next, 0);
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
  chunk_init(chunk, heap);
  /*
   * 1580013426 % 4096 == 1906
   * 914163893 % 4096 = 2229
   * 1798215561 % 4096 = 1929
   */
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 32);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 48);
  EXPECT_NE(alloc, nullptr);
  EXPECT_EQ(chunk->reg_size_small_medium[1906], 1);
  EXPECT_EQ(chunk->reg_size_small_medium[2229], 2);
  EXPECT_EQ(chunk->reg_size_small_medium[1929], 3);
}

TEST_F(ChunkUtilsTest, ChunkRegSizeArrayLargeHandling) {
  chunk_init(chunk, heap);
  // Allocate smallest large size, which will be a leaf node
  void *run_ptr = nullptr,
       *alloc = chunk_allocate_run(chunk, run_size_small, 8192);
  unsigned run_size = 0, reg_size = 0;

  chunk_get_run_ptr(chunk, alloc, &run_ptr, &run_size, &reg_size);
  EXPECT_EQ(alloc, run_ptr);
  EXPECT_EQ(run_size, run_size_small);
  EXPECT_EQ(reg_size, 0);
}

TEST_F(ChunkUtilsTest, ChunkSingleDeallocate) {
  void *alloc;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 4096 + 1906), NODE_DEPLETED);
}

TEST_F(ChunkUtilsTest, ChunkAllocationPlacement1) {
  void *alloc1, *alloc2, *alloc3;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  ptrdiff_t a1 = (ptrdiff_t)alloc1, a2 = (ptrdiff_t)alloc2;
  EXPECT_EQ((a1 - a2) % CHUNK_LEAST_REGION_SIZE_BYTES, 0);
  EXPECT_EQ(a1,
            (ptrdiff_t)chunk->entry.key + 1906 * CHUNK_LEAST_REGION_SIZE_BYTES);
  EXPECT_EQ(a2,
            (ptrdiff_t)chunk->entry.key + 2229 * CHUNK_LEAST_REGION_SIZE_BYTES);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepletedUnmappingLarge) {
  void *alloc1;
  chunk_init(chunk, heap);

  for (int i = 0; i < 32; i++) {
    alloc1 =
        chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION);
    chunk_deallocate_run(chunk, alloc1);
  }
  EXPECT_EQ(get_buddy_tree_item(chunk->buddy_tree, 1), NODE_UNMAPPED);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepletedUnmappingLarge) {
  void *alloc1;
  chunk_init(chunk, heap);

  for (int i = 0; i < 4096; i++) {
    alloc1 = chunk_allocate_run(chunk, CHUNK_LEAST_REGION_SIZE_BYTES, 16);
    chunk_deallocate_run(chunk, alloc1);
  }
  EXPECT_EQ(get_buddy_tree_item(chunk->buddy_tree, 1), NODE_UNMAPPED);
}

TEST_F(ChunkUtilsTest, ChunkGetRunPointerPositive) {
  void *expected_run_ptr, *run_ptr = nullptr;
  unsigned reg_size = 0, run_size = 0;
  run_ptr = NULL;
  chunk_init(chunk, heap);
  expected_run_ptr = chunk_allocate_run(chunk, run_size_small, 48);
  chunk_get_run_ptr(chunk, (void *)((uintptr_t)expected_run_ptr + 48 * 5),
                    &run_ptr, &run_size, &reg_size);
  EXPECT_NE(run_ptr, nullptr);
  EXPECT_EQ(expected_run_ptr, run_ptr);
  EXPECT_EQ(reg_size, 48);
  EXPECT_EQ(run_size, run_size_small);
}

TEST_F(ChunkUtilsTest, ChunkGetRunPointerNegative) {
  void *alloc, *run_ptr = 0;
  unsigned reg_size = 0, run_size = 0;
  run_ptr = NULL;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size_small, 48);
  chunk_get_run_ptr(chunk, (void *)((uintptr_t)alloc + run_size_small),
                    &run_ptr, &run_size, &reg_size);
  EXPECT_EQ(run_ptr, nullptr);
}

TEST_F(ChunkUtilsTest, ChunkIsFull) {
  unsigned chunks_to_alloc = CHUNK_NO_NODES_LAST_LAYER / 2;
  chunk_init(chunk, heap);
  for (int i = 0; i < chunks_to_alloc; i++) {
    EXPECT_NE(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  }
  EXPECT_TRUE(chunk_is_full(chunk));
}

TEST_F(ChunkUtilsTest, SizesArrayUpdate){

}

}  // namespace
