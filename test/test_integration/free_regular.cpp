#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/sealloc.h>
}

TEST(MallocApiTest, FreeRegular) {
  arena_t arena;
  arena_init(&arena);
  constexpr unsigned CHUNKS = 100;
  void *a, *b, *full;
  void *chunks[CHUNKS];
  size_t size;
  std::vector<size_t> SIZES{5, 10, 16, 17, 24, 32, 50, 4535, 12343, 544223};

  std::srand(123);
  int nalloc = 0;
  for (int i = 0; i < CHUNKS; i++) {
    size = SIZES[rand() % SIZES.size()];
    nalloc++;
    std::cerr << "======== Allocation nr " << nalloc << " =========\n";
    chunks[i] = sealloc_malloc(&arena, size);
    EXPECT_NE(chunks[i], nullptr);
  }
  int nfree = 0;
  for (int i = 0; i < CHUNKS; i++) {
    if (rand() % 2 == 0) {
      nfree++;
      std::cerr << "======== Free nr " << nfree << " =========\n";
      sealloc_free(&arena, chunks[i]);
    }
  }
}
