#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/internal_allocator.h>
#include <sealloc/utils.h>
}

ptrdiff_t pmask(void *ptr) {
  return reinterpret_cast<ptrdiff_t>(ptr) & ~(PAGE_SIZE - 1);
}

TEST(InternalAllocatorTest, BasicAllocations) {
  void *a, *b, *c, *d;
  int r = internal_allocator_init();
  EXPECT_EQ(r, 0) << "Initialization failed" << std::endl;
  a = internal_alloc(16);
  b = internal_alloc(16);
  EXPECT_NE(a, nullptr);
  EXPECT_NE(b, nullptr);
  internal_free(a);
  internal_free(b);
  c = internal_alloc(16);
  d = internal_alloc(16);
  EXPECT_EQ(a, c);
  EXPECT_EQ(b, d);
}

TEST(InternalAllocatorTest, MappingAllocation) {
  void *a, *b, *full;
  int r = internal_allocator_init();
  EXPECT_EQ(r, 0) << "Initialization failed" << std::endl;

  full = internal_alloc(INTERNAL_ALLOC_CHUNK_SIZE_BYTES);
  a = internal_alloc(16);
  b = internal_alloc(16);
  EXPECT_NE(pmask(a), pmask(full));
  EXPECT_EQ(pmask(a), pmask(b));
}

TEST(InternalAllocatorTest, RandomizedAllocationCrashTest) {
  constexpr unsigned CHUNKS = 10000;
  void *a, *b, *full;
  void *chunks[CHUNKS];
  size_t size;
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};

  int r = internal_allocator_init();
  EXPECT_EQ(r, 0) << "Initialization failed" << std::endl;
  std::srand(123);
  for (int i = 0; i < CHUNKS; i++) {
    size = SIZES[rand() % SIZES.size()];
    chunks[i] = internal_alloc(size);
  }
  for (int i = 0; i < CHUNKS; i++) {
    if (rand() % 2 == 0) {
      internal_free(chunks[i]);
    }
  }
  std::cout << "Success\n";
}

TEST(InternalAllocatorTest, MemoryIntegrity) {
  constexpr unsigned CHUNKS = 10000;
  void *a, *b, *full;
  void *chunks_dut[CHUNKS];
  std::independent_bits_engine<std::default_random_engine, 8, unsigned char>
      engine{1};
  std::vector<unsigned char> chunks_exp[CHUNKS];
  size_t size[CHUNKS];
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};
  int r = internal_allocator_init();
  EXPECT_EQ(r, 0) << "Initialization failed" << std::endl;

  for (int i = 0; i < CHUNKS; i++) {
    size[i] = SIZES[rand() % SIZES.size()];
    chunks_dut[i] = internal_alloc(size[i]);
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
