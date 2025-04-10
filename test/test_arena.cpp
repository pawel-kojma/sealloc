#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/chunk.h>
#include <sealloc/platform_api.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

namespace {
class ArenaUtilsTest : public ::testing::Test {
 protected:
  arena_t arena;
  unsigned run_size_small = 2 * PAGE_SIZE;
  size_t huge_chunk_size = 2097152;  // 2MB
  void SetUp() override { arena_init(&arena); }

  void exhaust_chunk(chunk_t *chunk) {
    unsigned chunks_to_alloc = CHUNK_NO_NODES_LAST_LAYER / 2;
    void *chunk_alloc[chunks_to_alloc];
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_deallocate_run(chunk, chunk_alloc[i]);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_alloc[i] = chunk_allocate_run(chunk, run_size_small, 16);
    }
    for (int i = 0; i < chunks_to_alloc; i++) {
      chunk_deallocate_run(chunk, chunk_alloc[i]);
    }
  }

  ptrdiff_t pmask(void *ptr) {
    return reinterpret_cast<ptrdiff_t>(ptr) & ~(PAGE_SIZE - 1);
  }

  uintptr_t get_bin_idx(bin_t *bin) {
    return ((uintptr_t)bin - (uintptr_t)&arena.bins) / sizeof(bin_t);
  }
};

TEST_F(ArenaUtilsTest, ArenaInit) {
  EXPECT_EQ(arena.is_initialized, 1);

  EXPECT_TRUE(arena.brk > 0);
  EXPECT_TRUE(arena.chunk_alloc_ptr < arena.brk);
  EXPECT_TRUE(arena.huge_alloc_ptr > arena.brk);
  EXPECT_TRUE(arena.internal_alloc_ptr > arena.brk);
  EXPECT_EQ(arena.chunks_left, 0);
  for (int i = 0; i < ARENA_NO_BINS; i++) {
    EXPECT_EQ(arena.bins[i].reg_size, 0);
  }
}

TEST_F(ArenaUtilsTest, ArenaChunkAllocate) {
  chunk_t *chunk = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaChunkDeallocate) {
  chunk_t *chunk1;
  chunk1 = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk1, nullptr);
  exhaust_chunk(chunk1);
  arena_deallocate_chunk(&arena, chunk1);
}

TEST_F(ArenaUtilsTest, ArenaGetChunkFromPtr) {
  chunk_t *chunk1, *chunk2, *chunk3;

  chunk1 = arena_allocate_chunk(&arena);
  chunk2 = arena_allocate_chunk(&arena);
  chunk3 = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk1, nullptr);
  EXPECT_NE(chunk2, nullptr);
  EXPECT_NE(chunk3, nullptr);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk1->entry.key), chunk1);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk2->entry.key), chunk2);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, chunk3->entry.key), chunk3);
  EXPECT_EQ(arena_get_chunk_from_ptr(
                &arena, (void *)((uintptr_t)chunk1->entry.key + 42)),
            chunk1);
  EXPECT_NE(arena_get_chunk_from_ptr(
                &arena, (void *)((uintptr_t)chunk3->entry.key - 1)),
            chunk3);
  EXPECT_EQ(arena_get_chunk_from_ptr(&arena, (void *)(0x42)), nullptr);
}

TEST_F(ArenaUtilsTest, ArenaGetBinByRegSize) {
  bin_t *small, *medium, *large1, *large2;
  small = arena_get_bin_by_reg_size(&arena, SMALL_SIZE_MIN_REGION);
  medium = arena_get_bin_by_reg_size(&arena, MEDIUM_SIZE_MIN_REGION);
  large1 = arena_get_bin_by_reg_size(&arena, 2 * PAGE_SIZE);
  large2 = arena_get_bin_by_reg_size(&arena, 4 * PAGE_SIZE);
  EXPECT_EQ(get_bin_idx(small), 0);
  EXPECT_EQ(get_bin_idx(medium), NO_SMALL_SIZE_CLASSES);
  EXPECT_EQ(get_bin_idx(large1),
            NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES);
  EXPECT_EQ(get_bin_idx(large2),
            NO_SMALL_SIZE_CLASSES + NO_MEDIUM_SIZE_CLASSES + 1);
}

TEST_F(ArenaUtilsTest, ArenaAllocateHugeChunk) {
  void *huge_chunk;
  huge_chunk = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  EXPECT_NE(huge_chunk, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaReallocateHugeChunkSameSize) {
  huge_chunk_t *huge_chunk1;
  void *key;
  huge_chunk1 = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  key = huge_chunk1->entry.key;
  arena_reallocate_huge_mapping(&arena, huge_chunk1, huge_chunk_size);
  EXPECT_EQ(huge_chunk1->entry.key, key);
}

TEST_F(ArenaUtilsTest, ArenaReallocateHugeChunkExpand) {
  huge_chunk_t *huge_chunk1;
  void *key;
  huge_chunk1 = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  EXPECT_NE(huge_chunk1->entry.key, nullptr);
  key = huge_chunk1->entry.key;
  arena_reallocate_huge_mapping(&arena, huge_chunk1, 2 * huge_chunk_size);
  EXPECT_NE(huge_chunk1->entry.key, nullptr);
  EXPECT_NE(huge_chunk1->entry.key, key);
}

TEST_F(ArenaUtilsTest, ArenaDeallocateHugeChunk) {
  huge_chunk_t *huge_chunk;
  huge_chunk = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  EXPECT_NE(huge_chunk, nullptr);
  arena_deallocate_huge_mapping(&arena, huge_chunk);
}

TEST_F(ArenaUtilsTest, ArenaFindHugeMapping) {
  huge_chunk_t *h1_expect, *h2_expect, *h1, *h2;
  h1_expect = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  h2_expect = arena_allocate_huge_mapping(&arena, huge_chunk_size);
  h1 = arena_find_huge_mapping(&arena, h1_expect->entry.key);
  h2 = arena_find_huge_mapping(&arena, h2_expect->entry.key);
  EXPECT_EQ(h1, h1_expect);
  EXPECT_EQ(h2, h2_expect);
}

TEST_F(ArenaUtilsTest, ArenaAllocateRun) {
  bin_t *bin = arena_get_bin_by_reg_size(&arena, SMALL_SIZE_CLASS_ALIGNMENT);
  run_t *run = arena_allocate_run(&arena, bin);
  EXPECT_NE(run, nullptr);
}

TEST_F(ArenaUtilsTest, ArenaInternalAlloc) {
  void *a, *b, *full;

  full = arena_internal_alloc(&arena, INTERNAL_ALLOC_CHUNK_SIZE_BYTES);
  a = arena_internal_alloc(&arena, 16);
  b = arena_internal_alloc(&arena, 16);
  EXPECT_NE(pmask(a), pmask(full));
  EXPECT_EQ(pmask(a), pmask(b));
}

TEST_F(ArenaUtilsTest, RandomizedAllocationCrashTest) {
  constexpr unsigned CHUNKS = 10000;
  void *a, *b, *full;
  void *chunks[CHUNKS];
  size_t size;
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};

  std::srand(123);
  for (int i = 0; i < CHUNKS; i++) {
    size = SIZES[rand() % SIZES.size()];
    chunks[i] = arena_internal_alloc(&arena, size);
  }
  for (int i = 0; i < CHUNKS; i++) {
    if (rand() % 2 == 0) {
      arena_internal_free(&arena, chunks[i]);
    }
  }
}

TEST_F(ArenaUtilsTest, MemoryIntegrity) {
  constexpr unsigned CHUNKS = 10000;
  void *a, *b, *full;
  void *chunks_dut[CHUNKS];
  std::independent_bits_engine<std::default_random_engine, 8, unsigned char>
      engine{1};
  std::vector<unsigned char> chunks_exp[CHUNKS];
  size_t size[CHUNKS];
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};

  for (int i = 0; i < CHUNKS; i++) {
    size[i] = SIZES[rand() % SIZES.size()];
    chunks_dut[i] = arena_internal_alloc(&arena, size[i]);
    chunks_exp[i].reserve(size[i]);
    std::generate(chunks_exp[i].begin(), chunks_exp[i].end(), std::ref(engine));
    std::memcpy(chunks_dut[i], chunks_exp[i].data(), size[i]);
  }

  for (int i = 0; i < CHUNKS; i++) {
    EXPECT_PRED3([](const void *a, const void *b,
                    size_t s) { return std::memcmp(a, b, s) == 0; },
                 chunks_dut[i], chunks_exp[i].data(), size[i])
        << "Chunks of size " << size[i] << " at index " << i << " differ";
  }
}

TEST_F(ArenaUtilsTest, CeilingHitOnProbeSuccess) {
  platform_status_code_t code;
  uintptr_t res = 0;
  code = platform_map(NULL, PAGE_SIZE, (void **)&res);
  EXPECT_NE(res, 0);
  arena.chunk_alloc_ptr = res;
  arena.brk = res + PAGE_SIZE;
  chunk_t *chunk = arena_allocate_chunk(&arena);
  EXPECT_NE(chunk, nullptr);
  EXPECT_NE(res, arena.chunk_alloc_ptr);
}

}  // namespace
