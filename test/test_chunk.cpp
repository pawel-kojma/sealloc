#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/chunk.h>
#include <sealloc/platform_api.h>
#include <sealloc/random.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

namespace {
class ChunkUtilsTest : public ::testing::Test {
 protected:
  chunk_t *chunk;
  void *heap;
  unsigned run_size_small = 4 * PAGE_SIZE;
  unsigned run_size_large = 8 * PAGE_SIZE;
  void SetUp() override {
    chunk = (chunk_t *)malloc(sizeof(chunk_t));
    platform_map(NULL, CHUNK_SIZE_BYTES, &heap);
    init_splitmix32(1);
    chunk_init(chunk, heap);
  }

  jump_node_t get_jt_item(jump_node_t *mem, unsigned idx) {
    return mem[idx - 1];
  }

  void print_nodes_on_level(unsigned level) {
    unsigned all_nodes_count = 1 << level;
    unsigned leftmost_idx = 1 << level, rightmost_idx = (1 << (level + 1)) - 1;
    std::cout << "Jump Start : " << chunk->jump_tree_first_index[level] << " ";
    for (int i = leftmost_idx; i <= rightmost_idx; i++) {
      std::cout << i - leftmost_idx << "(p=" << chunk->jump_tree[i - 1].prev
                << ", n=" << chunk->jump_tree[i - 1].next << ")" << " ";
    }
    std::cout << '\n';
  }

  void validate_free_list_on_level(unsigned level,
                                   unsigned expected_free_count) {
    unsigned all_nodes_count = 1 << level;
    unsigned leftmost_idx = 1 << level, rightmost_idx = (1 << (level + 1)) - 1;
    unsigned free_count = chunk->avail_nodes_count[level];
    unsigned actual_free_count, idx;
    jump_node_t node;
    std::vector<int> is_free_idx;
    is_free_idx.reserve(all_nodes_count);
    for (int i = 0; i < all_nodes_count; i++) {
      is_free_idx[i] = false;
    }
    EXPECT_EQ(free_count, expected_free_count);

    // Validate first free idx
    idx = chunk->jump_tree_first_index[level];
    if (idx == 0) {
      EXPECT_EQ(free_count, 0);
    } else {
      EXPECT_TRUE(idx >= leftmost_idx);
      EXPECT_TRUE(idx <= rightmost_idx);
    }

    if (free_count == 0) EXPECT_EQ(idx, 0);

    // Validate actual free count
    if (expected_free_count > 1) {
      idx = chunk->jump_tree_first_index[level];
      actual_free_count = 1;
      for (node = get_jt_item(chunk->jump_tree, idx); node.next != 0;
           node = get_jt_item(chunk->jump_tree, idx)) {
        actual_free_count++;
        idx += node.next;
        EXPECT_TRUE(idx >= leftmost_idx);
        EXPECT_TRUE(idx <= rightmost_idx);
      }
      EXPECT_EQ(actual_free_count, expected_free_count)
          << "Failed at level " << level;
    }

    // Validate node metadata
    if (expected_free_count > 1) {
      idx = chunk->jump_tree_first_index[level];
      for (node = get_jt_item(chunk->jump_tree, idx); node.next != 0;
           node = get_jt_item(chunk->jump_tree, idx)) {
        is_free_idx[idx - leftmost_idx] = true;
        idx += node.next;
      }
      // add the last one
      is_free_idx[idx - leftmost_idx] = true;

      for (idx = leftmost_idx; idx <= rightmost_idx; idx++) {
        node = get_jt_item(chunk->jump_tree, idx);
        if (is_free_idx[idx - leftmost_idx]) {
          EXPECT_TRUE(node.prev != 0 || node.next != 0)
              << "Failed at idx " << idx - leftmost_idx;
        } else {
          EXPECT_EQ(node.prev, 0) << "Failed at idx " << idx - leftmost_idx;
          EXPECT_EQ(node.next, 0) << "Failed at idx " << idx - leftmost_idx;
        }
      }
    } else {
      for (idx = leftmost_idx; idx <= rightmost_idx; idx++) {
        node = get_jt_item(chunk->jump_tree, idx);
        EXPECT_EQ(node.prev, 0);
        EXPECT_EQ(node.next, 0);
      }
    }
  }

  void validate_entire_tree() {
    for (int level = 0; level <= CHUNK_BUDDY_TREE_DEPTH; level++) {
      validate_free_list_on_level(level, chunk->avail_nodes_count[level]);
    }
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

static chunk_node_t get_buddy_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (chunk_node_t)(mem[word] >> (2 * off) & 3);
}

template <class T>
bool all_unique(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  return std::adjacent_find(v.begin(), v.end()) == v.end();
}

unsigned size2idx(unsigned run_size) {
  return ctz(run_size / CHUNK_LEAST_REGION_SIZE_BYTES);
}

TEST(ChunkUtilsTestUtility, Size2Index) {
  std::vector<unsigned> sizes = {16384,   32768,   65536,    131072,
                                 262144,  524288,  1048576,  2097152,
                                 4194304, 8388608, 16777216, 33554432};
  for (int i = 0; i < sizes.size(); i++) {
    EXPECT_EQ(size2idx(sizes[i]), i);
  }
}

TEST_F(ChunkUtilsTest, ChunkInit) {
  jump_node_t node;
  std::vector<unsigned> pow = {1,   2,   4,   8,    16,   32,  64,
                               128, 256, 512, 1024, 2048};
  EXPECT_EQ(chunk->avail_nodes_count[0], 0);
  for (int i = 1; i < pow.size(); i++) {
    EXPECT_EQ(pow[i], chunk->avail_nodes_count[i]);
    EXPECT_EQ(pow[i], chunk->jump_tree_first_index[i]);
  }
  node = get_jt_item(chunk->jump_tree, 1);
  EXPECT_EQ(node.prev, 0);
  EXPECT_EQ(node.next, 0);
  for (int i = 2; i <= 2048; i *= 2) {
    node = get_jt_item(chunk->jump_tree, i);
    EXPECT_EQ(node.prev, 0);
    EXPECT_EQ(node.next, 1);
    node = get_jt_item(chunk->jump_tree, 2 * i - 1);
    EXPECT_EQ(node.next, 0);
    EXPECT_EQ(node.prev, 1);
  }
}

TEST_F(ChunkUtilsTest, ChunkSingleAllocation) {
  void *alloc, *expected;
  unsigned reg_size = 16;
  alloc = chunk_allocate_run(chunk, run_size_small, reg_size);
  expected = (void *)((uintptr_t)chunk->entry.key +
                      (1906 * CHUNK_LEAST_REGION_SIZE_BYTES));
  EXPECT_EQ(alloc, expected);
  // 1580013426 % 4096 == 1906
  EXPECT_EQ(get_buddy_tree_item(chunk->buddy_tree, 2048 + 1906), NODE_USED);
  jump_node_t node = get_jt_item(chunk->jump_tree, 2048 + 1906);
  EXPECT_EQ(node.prev, 0);
  EXPECT_EQ(node.next, 0);
  node = get_jt_item(chunk->jump_tree, 2048 + 1905);
  EXPECT_EQ(node.prev, 1);
  EXPECT_EQ(node.next, 2);
  node = get_jt_item(chunk->jump_tree, 2048 + 1907);
  EXPECT_EQ(node.prev, 2);
  EXPECT_EQ(node.next, 1);
}

TEST_F(ChunkUtilsTest, ChunkManyAllocations) {
  constexpr unsigned CHUNKS = 100;
  std::vector<std::pair<size_t, size_t>> run_reg_sizes = {
      {4 * PAGE_SIZE, 16},    {4 * PAGE_SIZE, 32},
      {4 * PAGE_SIZE, 48},    {4 * PAGE_SIZE, 1024},
      {4 * PAGE_SIZE, 16384}, {8 * PAGE_SIZE, 8 * PAGE_SIZE}};
  std::vector<void *> chunks;
  chunks.reserve(CHUNKS);
  std::srand(123);
  for (int i = 0; i < CHUNKS; i++) {
    auto [run_size, reg_size] = run_reg_sizes[rand() % run_reg_sizes.size()];
    chunks[i] = chunk_allocate_run(chunk, run_size, reg_size);
    EXPECT_NE(chunks[i], nullptr) << "Failed at idx " << i;
    validate_entire_tree();
  }
  EXPECT_TRUE(all_unique(chunks));
}

TEST_F(ChunkUtilsTest, ChunkRegSizeArrayUpdate) {
  void *alloc;
  /*
   * 1580013426 % 2048 = 1906
   * 350525680 % 2048 = 240
   * 3524174333 % 2048 = 509
   */
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 32);
  EXPECT_NE(alloc, nullptr);
  alloc = chunk_allocate_run(chunk, run_size_small, 48);
  EXPECT_NE(alloc, nullptr);
  EXPECT_EQ(chunk->reg_size_small_medium[1906], 16);
  EXPECT_EQ(chunk->reg_size_small_medium[240], 32);
  EXPECT_EQ(chunk->reg_size_small_medium[509], 48);
  EXPECT_EQ(chunk->reg_size_small_medium[508], 65535);
  EXPECT_EQ(chunk->reg_size_small_medium[510], 65535);
}

TEST_F(ChunkUtilsTest, ChunkRegSizeArrayLargeHandling) {
  // Allocate smallest large size, which will be a leaf node
  void *run_ptr = nullptr,
       *alloc = chunk_allocate_run(chunk, run_size_small, 16384);
  unsigned run_size = 0, reg_size = 0;

  chunk_get_run_ptr(chunk, alloc, &run_ptr, &run_size, &reg_size);
  EXPECT_EQ(alloc, run_ptr);
  EXPECT_EQ(run_size, run_size_small);
  EXPECT_EQ(reg_size, 0);
}

TEST_F(ChunkUtilsTest, ChunkSingleDeallocate) {
  void *alloc;
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  chunk_deallocate_run(chunk, alloc);
  EXPECT_EQ(get_buddy_tree_item(chunk->buddy_tree, 1906), NODE_DEPLETED);
}

TEST_F(ChunkUtilsTest, AvailNodesCountUpdateSmall) {
  void *alloc;
  alloc = chunk_allocate_run(chunk, run_size_small, 16);
  unsigned short l = 1;
  for (int i = 0; i < CHUNK_BUDDY_TREE_DEPTH + 1; i++) {
    EXPECT_EQ(chunk->avail_nodes_count[i], l - 1) << "Failed on level " << i;
    l *= 2;
  }
}

TEST_F(ChunkUtilsTest, AvailNodesCountUpdateLarge) {
  void *alloc;
  alloc =
      chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION);
  EXPECT_EQ(chunk->avail_nodes_count[0], 0);
  EXPECT_EQ(chunk->avail_nodes_count[1], 1);
  EXPECT_EQ(chunk->avail_nodes_count[2], 3);
  EXPECT_EQ(chunk->avail_nodes_count[3], 7);
  EXPECT_EQ(chunk->avail_nodes_count[4], 15);
  EXPECT_EQ(chunk->avail_nodes_count[5], 31);
  EXPECT_EQ(chunk->avail_nodes_count[6], 62);
  EXPECT_EQ(chunk->avail_nodes_count[7], 124);
  EXPECT_EQ(chunk->avail_nodes_count[8], 248);
  EXPECT_EQ(chunk->avail_nodes_count[9], 496);
  EXPECT_EQ(chunk->avail_nodes_count[10], 992);
  EXPECT_EQ(chunk->avail_nodes_count[11], 1984);
}

TEST_F(ChunkUtilsTest, ChunkDeallocateRunSmall) {
  void *alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  EXPECT_NE(alloc1, nullptr);
  chunk_deallocate_run(chunk, alloc1);
}

TEST_F(ChunkUtilsTest, ChunkDeallocateRunLarge) {
  void *alloc1 = chunk_allocate_run(chunk, run_size_large, run_size_large);
  EXPECT_NE(alloc1, nullptr);
  chunk_deallocate_run(chunk, alloc1);
}

TEST_F(ChunkUtilsTest, ChunkValidateTreeSmall) {
  void *alloc;
  unsigned reg_size = 16;
  alloc = chunk_allocate_run(chunk, run_size_small, reg_size);
  validate_entire_tree();
}

TEST_F(ChunkUtilsTest, ChunkValidateTreeMedium) {
  void *alloc;
  alloc = chunk_allocate_run(chunk, run_size_small, 2048);
  validate_entire_tree();
}

TEST_F(ChunkUtilsTest, ChunkValidateTreeLarge) {
  void *alloc;
  alloc =
      chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION);
  validate_entire_tree();
}

TEST_F(ChunkUtilsTest, ReturnsNullWhenFull) {
  constexpr unsigned CHUNKS = 2047;
  for (int i = 0; i < CHUNKS; i++) {
    EXPECT_NE(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  }
  EXPECT_EQ(chunk_allocate_run(chunk, CHUNK_LEAST_REGION_SIZE_BYTES * 2,
                               CHUNK_LEAST_REGION_SIZE_BYTES * 2),
            nullptr);
  EXPECT_EQ(
      chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION),
      nullptr);
  EXPECT_NE(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  EXPECT_EQ(chunk_allocate_run(chunk, run_size_small, 16), nullptr);
  for (int i = 0; i <= CHUNK_BUDDY_TREE_DEPTH; i++) {
    EXPECT_EQ(chunk->jump_tree_first_index[i], 0);
  }
}

TEST_F(ChunkUtilsTest, ChunkAllocateAllNodesLarge) {
  constexpr unsigned CHUNKS = 32;
  constexpr unsigned START_IDX = 32;
  constexpr unsigned LEVEL = 5;
  std::vector<void *> chunks;
  jump_node_t node;
  unsigned free_count, idx;
  chunks.reserve(CHUNKS);
  for (int i = 0; i < CHUNKS; i++) {
    chunks[i] =
        chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION);
    EXPECT_NE(chunks[i], nullptr);
    EXPECT_EQ(chunk->avail_nodes_count[LEVEL], CHUNKS - i - 1);
    validate_free_list_on_level(LEVEL, CHUNKS - i - 1);
    print_nodes_on_level(LEVEL);
  }
  validate_free_list_on_level(LEVEL, 0);
  EXPECT_TRUE(all_unique(chunks));
}

TEST_F(ChunkUtilsTest, ChunkAllocateAllNodesMedium) {
  constexpr unsigned CHUNKS = 2048;
  constexpr unsigned START_IDX = 2048;
  constexpr unsigned LEVEL = 11;
  std::vector<void *> chunks;
  jump_node_t node;
  unsigned free_count, idx;
  chunks.reserve(CHUNKS);
  for (int i = 0; i < CHUNKS; i++) {
    chunks[i] = chunk_allocate_run(chunk, run_size_small, 2048);
    EXPECT_NE(chunks[i], nullptr);
    EXPECT_EQ(chunk->avail_nodes_count[LEVEL], CHUNKS - i - 1);
    validate_free_list_on_level(LEVEL, CHUNKS - i - 1);
  }
  validate_free_list_on_level(LEVEL, 0);
  EXPECT_TRUE(all_unique(chunks));
}

TEST_F(ChunkUtilsTest, ChunkAllocateAllNodesSmall) {
  constexpr unsigned CHUNKS = 2048;
  constexpr unsigned START_IDX = 2048;
  constexpr unsigned LEVEL = 11;
  std::vector<void *> chunks;
  jump_node_t node;
  unsigned free_count, idx;
  chunks.reserve(CHUNKS);
  for (int i = 0; i < CHUNKS; i++) {
    chunks[i] = chunk_allocate_run(chunk, run_size_small, 16);
    EXPECT_NE(chunks[i], nullptr);
    EXPECT_EQ(chunk->avail_nodes_count[LEVEL], CHUNKS - i - 1);
    validate_free_list_on_level(LEVEL, CHUNKS - i - 1);
  }
  validate_free_list_on_level(LEVEL, 0);
  EXPECT_TRUE(all_unique(chunks));
}

TEST_F(ChunkUtilsTest, ChunkAllocationPlacementSmall) {
  void *alloc1, *alloc2, *alloc3;
  alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
  alloc2 = chunk_allocate_run(chunk, run_size_small, 16);
  ptrdiff_t a1 = (ptrdiff_t)alloc1, a2 = (ptrdiff_t)alloc2;
  EXPECT_EQ((a1 - a2) % CHUNK_LEAST_REGION_SIZE_BYTES, 0);
  EXPECT_EQ(a1,
            (ptrdiff_t)chunk->entry.key + 1906 * CHUNK_LEAST_REGION_SIZE_BYTES);
  EXPECT_EQ(a2,
            (ptrdiff_t)chunk->entry.key + 2288 * CHUNK_LEAST_REGION_SIZE_BYTES);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepletedUnmappingLarge) {
  constexpr int CHUNKS = 32;
  void *alloc1;
  bool is_unmapped;
  for (int i = 0; i < CHUNKS; i++) {
    alloc1 =
        chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION);
    EXPECT_NE(alloc1, nullptr);
    is_unmapped = chunk_deallocate_run(chunk, alloc1);
    if (i + 1 == CHUNKS)
      EXPECT_TRUE(is_unmapped);
    else
      EXPECT_FALSE(is_unmapped);
  }
  EXPECT_EQ(get_buddy_tree_item(chunk->buddy_tree, 1), NODE_UNMAPPED);
}

TEST_F(ChunkUtilsTest, ChunkCoalesceDepletedUnmappingSmall) {
  constexpr int CHUNKS = 2048;
  void *alloc1;
  bool is_unmapped;
  for (int i = 0; i < CHUNKS; i++) {
    alloc1 = chunk_allocate_run(chunk, run_size_small, 16);
    EXPECT_NE(alloc1, nullptr);
    is_unmapped = chunk_deallocate_run(chunk, alloc1);
    if (i + 1 == CHUNKS)
      EXPECT_TRUE(is_unmapped);
    else
      EXPECT_FALSE(is_unmapped);
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
  constexpr unsigned CHUNKS = 32;
  chunk_init(chunk, heap);
  for (int i = 0; i < CHUNKS; i++) {
    EXPECT_NE(
        chunk_allocate_run(chunk, LARGE_SIZE_MAX_REGION, LARGE_SIZE_MAX_REGION),
        nullptr);
  }
  EXPECT_TRUE(chunk_is_full(chunk));
}

}  // namespace
