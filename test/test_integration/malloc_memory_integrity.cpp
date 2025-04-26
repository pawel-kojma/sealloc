#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>
#include <iostream>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/random.h>
#include <sealloc/sealloc.h>
}

TEST(MallocApiTest, MemoryIntegrity) {
  arena_t arena;
  arena.is_initialized = 0;
  arena_init(&arena);
  constexpr unsigned CHUNKS = 10000;
  void *a, *b, *full;
  void *chunks_dut[CHUNKS];
  std::independent_bits_engine<std::default_random_engine, 8, unsigned char>
      engine{1};
  std::vector<unsigned char> chunks_exp[CHUNKS];
  size_t size[CHUNKS];
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};
  init_splitmix32(2060289228);
  for (int i = 0; i < CHUNKS; i++) {
    size[i] = SIZES[rand() % SIZES.size()];
    std::cout << "========================== Allocation " << i + 1
              << " ==========================\n";
    chunks_dut[i] = sealloc_malloc(&arena, size[i]);
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
