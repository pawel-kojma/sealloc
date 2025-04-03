#include <gtest/gtest.h>

extern "C" {
#include <sealloc/arena.h>
#include <sealloc/random.h>
#include <sealloc/sealloc.h>
#include <sealloc/size_class.h>
}

constexpr unsigned MAXCHUNKS = 50;
constexpr unsigned MAXROUNDS = 1000000;

class MallocApiTest : public testing::Test {
 protected:
  arena_t arena;
  unsigned nmalloc = 0;
  unsigned nfree = 0;
  unsigned nrealloc = 0;
  unsigned ncalloc = 0;
  std::vector<size_t> SIZES;
  void *chunks[MAXCHUNKS] = {};
  size_t sizes[MAXCHUNKS] = {0};
  size_t chunk_size, rand_size;
  void *chunk;
  size_t idx;
  int op;
  void SetUp() override {
    arena_init(&arena);
    // set internal seed to known value
    init_splitmix32(123);

    // sizes of classess + edge values
    // small unaligned
    for (int i = 1; i < SMALL_SIZE_MIN_REGION - 1; i++) SIZES.push_back(i);
    // all small sizes
    for (int i = SMALL_SIZE_MIN_REGION; i <= SMALL_SIZE_MAX_REGION;
         i += SMALL_SIZE_CLASS_ALIGNMENT) {
      SIZES.push_back(i - 1);
      SIZES.push_back(i);
      SIZES.push_back(i + 1);
    }
    // all medium sizes
    for (int i = MEDIUM_SIZE_MIN_REGION; i <= MEDIUM_SIZE_MAX_REGION;
         i += alignup_medium_size(i+1)) {
      SIZES.push_back(i - 1);
      SIZES.push_back(i);
      SIZES.push_back(i + 1);
    }
    // all large sizes
    for (int i = LARGE_SIZE_MIN_REGION; i <= LARGE_SIZE_MAX_REGION;
         i += alignup_large_size(i + 1)) {
      SIZES.push_back(i - 1);
      SIZES.push_back(i);
      SIZES.push_back(i + 1);
    }
  }

  void print_op() {
    switch (op) {
      case 0:
        std::cerr << "======== Free nr " << nfree << " =========\n";
        nfree++;
        break;
      case 1:
        std::cerr << "======== Realloc nr " << nrealloc << " =========\n";
        nrealloc++;
        break;
      case 2:
        std::cerr << "======== Calloc nr " << ncalloc << " =========\n";
        ncalloc++;
        break;
      case 3:
        std::cerr << "======== Malloc nr " << nmalloc << " =========\n";
        nmalloc++;
        break;
    }
  }
};

bool is_zero(void *a, size_t b) {
  uint8_t *ap = (uint8_t *)a;
  for (int i = 0; i < b; i++) {
    if (*ap != 0) return false;
  }
  return true;
}

TEST_F(MallocApiTest, FuzzTesting) {
  std::srand(123);
  for (int i = 0; i < MAXROUNDS; i++) {
    idx = rand() % MAXCHUNKS;
    chunk = chunks[idx];
    chunk_size = sizes[idx];
    if (chunk == nullptr) {
      // malloc, calloc, realloc
      rand_size = SIZES[rand() % SIZES.size()];
      sizes[idx] = rand_size;
      op = (rand() % 3) + 1;
      print_op();
      if (op == 3) {
        // malloc
        chunks[idx] = sealloc_malloc(&arena, rand_size);
        ASSERT_NE(chunks[idx], nullptr) << "End of Memory";
      } else if (op == 2) {
        // calloc
        chunks[idx] = sealloc_calloc(&arena, 1, rand_size);
        ASSERT_NE(chunks[idx], nullptr) << "End of Memory";
        ASSERT_PRED2(is_zero, chunks[idx], sizes[idx]);
      } else if (op == 1) {
        // realloc
        chunks[idx] = sealloc_realloc(&arena, nullptr, rand_size);
        ASSERT_NE(chunks[idx], nullptr) << "End of Memory";
      }
    } else {
      op = rand() % 2;
      print_op();
      if (op == 0) {
        // free
        sealloc_free(&arena, chunks[idx]);
        chunks[idx] = nullptr;
      } else {
        // realloc
        chunks[idx] = sealloc_realloc(&arena, chunks[idx], rand_size);
        sizes[idx] = rand_size;
        ASSERT_NE(chunks[idx], nullptr) << "End of Memory";
      }
    }
  }
}
