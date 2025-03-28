#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/chunk.h>
#include <sealloc/platform_api.h>
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
  unsigned run_size_small = 2 * PAGE_SIZE;
  unsigned run_size_large = 4 * PAGE_SIZE;
  void SetUp() override {
    chunk = (chunk_t *)malloc(sizeof(chunk_t));

    platform_map(NULL, CHUNK_SIZE_BYTES, &heap);
  }
};

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_SPLIT = 1,
  NODE_USED = 2,
  NODE_FULL = 3,
  NODE_GUARD = 4,
  NODE_DEPLETED = 5,
  NODE_UNMAPPED = 6
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
  unsigned reg_size = 16;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size_small, reg_size);
  EXPECT_NE(alloc, nullptr);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_GUARD);
  EXPECT_EQ(chunk->free_mem, CHUNK_SIZE_BYTES - (run_size_small + CHUNK_LEAST_REGION_SIZE_BYTES));
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
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 32);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 48);
  EXPECT_NE(alloc, nullptr);
  EXPECT_EQ(chunk->reg_size_small_medium[0], 1);
  EXPECT_EQ(chunk->reg_size_small_medium[2], 2);
  EXPECT_EQ(chunk->reg_size_small_medium[4], 3);
  EXPECT_EQ(chunk->reg_size_small_medium[1], 0xff);
  EXPECT_EQ(chunk->reg_size_small_medium[3], 0xff);
  EXPECT_EQ(chunk->reg_size_small_medium[5], 0xff);
}

TEST_F(ChunkUtilsTest, ChunkSingleDeallocate) {
  void *alloc;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_FREE);
}

TEST_F(ChunkUtilsTest, ChunkAllocationPlacement1) {
  void *alloc1, *alloc2, *alloc3;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ((uintptr_t)alloc1, (uintptr_t)chunk->entry.key);
  EXPECT_EQ((uintptr_t)alloc2, (uintptr_t)chunk->entry.key + (4 * PAGE_SIZE));
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_GUARD);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 514), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 515), NODE_GUARD);
}

TEST_F(ChunkUtilsTest, ChunkAllocationPlacement2) {
  void *alloc_large, *alloc_small;
  chunk_init(chunk, heap);
  alloc_large = chunk_allocate_run(chunk, run_size_large, run_size_large);
  alloc_small = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ((uintptr_t)alloc_large, (uintptr_t)chunk->entry.key);
  EXPECT_EQ((uintptr_t)alloc_small,
            (uintptr_t)chunk->entry.key + 6 * PAGE_SIZE);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, (512 / 2)), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 514), NODE_GUARD);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 515), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 516), NODE_GUARD);
}

TEST_F(ChunkUtilsTest, ChunkAllocateOnGuardPage) {
  void *alloc1, *alloc2, *alloc3;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc1);
  chunk_deallocate_run(chunk, alloc2);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 512), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_FREE);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 514), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 515), NODE_FREE);
  alloc3 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 513), NODE_USED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 514), NODE_DEPLETED);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepleted) {
  void *alloc1, *alloc2, *alloc3, *alloc4;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc1);
  chunk_deallocate_run(chunk, alloc2);
  alloc3 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc4 = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc3);
  chunk_deallocate_run(chunk, alloc4);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 256), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 257), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 128), NODE_DEPLETED);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 64), NODE_SPLIT);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepletedUnmapping) {
  void *alloc1, *alloc2;
  unsigned run_size = 16 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size, run_size);
  chunk_deallocate_run(chunk, alloc1);
  alloc2 = chunk_allocate_run(chunk, run_size, run_size);
  chunk_deallocate_run(chunk, alloc2);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 32), NODE_UNMAPPED);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceFreeNodes) {
  void *alloc1;
  unsigned run_size = 8 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size, run_size);
  unsigned idx = 516, it;
  it = idx;
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, it), NODE_GUARD);
  it /= 2;
  while (it > 0) {
    EXPECT_EQ(get_tree_item(chunk->buddy_tree, it), NODE_SPLIT);
    it /= 2;
  }
  chunk_deallocate_run(chunk, alloc1);
  it = idx;
  while (it > 64) {
    EXPECT_EQ(get_tree_item(chunk->buddy_tree, it), NODE_FREE);
    it /= 2;
  }
  EXPECT_NE(get_tree_item(chunk->buddy_tree, 32), NODE_FREE);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceFullNodes) {
  void *alloc1, *alloc2;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 256), NODE_FULL);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 128), NODE_SPLIT);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 257), NODE_FULL);
  EXPECT_EQ(get_tree_item(chunk->buddy_tree, 128), NODE_FULL);
}

TEST_F(ChunkUtilsTest, ChunkLeafNodeInfoUsage) {
  void *alloc1, *alloc2, *alloc3, *alloc4;
  unsigned run_size_large1 = 4 * PAGE_SIZE;
  unsigned run_size_large2 = 8 * PAGE_SIZE;
  chunk_init(chunk, heap);
  alloc1 = chunk_allocate_run(chunk, run_size_large1, run_size_large1);
  alloc2 = chunk_allocate_run(chunk, run_size_large2, run_size_large2);
  chunk_deallocate_run(chunk, alloc2);
  alloc3 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_EQ((uintptr_t)alloc2, (uintptr_t)chunk->entry.key + 8 * PAGE_SIZE);
  EXPECT_EQ((uintptr_t)alloc3, (uintptr_t)chunk->entry.key + 6 * PAGE_SIZE);
}

TEST_F(ChunkUtilsTest, ChunkGetRunPointerPositive) {
  void *expected_run_ptr, *run_ptr;
  unsigned reg_size, run_size;
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
  void *alloc, *run_ptr;
  unsigned reg_size, run_size;
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

TEST_F(ChunkUtilsTest, ChunkIsNotFull) {
  unsigned chunks_to_alloc = (CHUNK_NO_NODES_LAST_LAYER / 2) - 1;
  chunk_init(chunk, heap);
  for (int i = 0; i < chunks_to_alloc; i++) {
    EXPECT_NE(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  }
  EXPECT_FALSE(chunk_is_full(chunk));
  EXPECT_EQ(chunk->free_mem, 2 * CHUNK_LEAST_REGION_SIZE_BYTES);
  EXPECT_NE(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  EXPECT_EQ(chunk->free_mem, 0);
}

TEST_F(ChunkUtilsTest, ChunkExhaustion) {
  unsigned chunks_to_alloc = CHUNK_NO_NODES_LAST_LAYER / 2;
  void *chunk_alloc[chunks_to_alloc];
  chunk_init(chunk, heap);
  void *ptr = heap;
  for (int i = 0; i < chunks_to_alloc; i++) {
    chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    EXPECT_EQ(chunk_alloc[i], ptr)
        << "Failed to alloc " << i + 1 << "-th in first round (max "
        << chunks_to_alloc << ")";
    ptr = (void *)((uintptr_t)ptr + 2 * CHUNK_LEAST_REGION_SIZE_BYTES);
  }
  ASSERT_EQ(chunk->free_mem, 0);
  unsigned free_mem = 0;
  for (int i = 0; i < chunks_to_alloc; i++) {
    EXPECT_FALSE(chunk_deallocate_run(chunk, chunk_alloc[i]));
    free_mem += CHUNK_LEAST_REGION_SIZE_BYTES;
    ASSERT_EQ(chunk->free_mem, free_mem)
        << "Failed to free on alloc " << i + 1 << "/" << chunks_to_alloc;
  }
  for (int i = 0; i < chunks_to_alloc; i++) {
    chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    EXPECT_NE(chunk_alloc[i], nullptr)
        << "Failed to alloc " << i + 1 << "-th in second round (max "
        << chunks_to_alloc << ")";
  }
  bool unmapped = false;
  for (int i = 0; i < chunks_to_alloc; i++) {
    EXPECT_FALSE(unmapped);
    unmapped = chunk_deallocate_run(chunk, chunk_alloc[i]);
  }
  EXPECT_TRUE(unmapped);
}

TEST_F(ChunkUtilsTest, ChunkDeathOnGuardPageWrite) {
  void *alloc;
  chunk_init(chunk, heap);
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  ((int *)alloc)[run_size_small / sizeof(int) - 1] = 42;
  EXPECT_DEATH({ ((int *)alloc)[run_size_small / sizeof(int)] = 42; }, ".*");
}

}  // namespace
