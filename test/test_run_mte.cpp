#include <gtest/gtest.h>

#include <bitset>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
#include <sys/mman.h>
}

uint64_t get_tag_from_ptr(uint64_t ptr) { return ptr >> 56; }

std::vector<void *> get_chunks(run_t *run, bin_t *bin) {
  unsigned elems = run->navail;
  std::vector<void *> chunks;
  for (unsigned i = 0; i < elems; i++) {
    chunks.push_back(run_allocate(run, bin));
    EXPECT_NE(get_tag_from_ptr(reinterpret_cast<uint64_t>(chunks[i])), 0);
  }
  std::sort(chunks.begin(), chunks.end(), [](void *a, void *b) {
    return (reinterpret_cast<uint64_t>(a) & 0xFFFFFFFFFFFF) <
           (reinterpret_cast<uint64_t>(b) & 0xFFFFFFFFFFFF);
  });
  return chunks;
}

class RunUtilsMTESmall : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    bin_init(bin, SMALL_SIZE_MIN_REGION);
    heap = mmap(NULL, bin->run_size_pages * PAGE_SIZE,
                PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
    run_init(run, bin, heap);
  }
};

class RunUtilsMTEParametrized : public ::testing::TestWithParam<unsigned> {};

using RunUtilsMTEDeathSmall = RunUtilsMTESmall;

TEST_P(RunUtilsMTEParametrized, DifferentTagsForNeighbors) {
  unsigned reg_size = GetParam();
  init_splitmix32(1);
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));
  bin_init(bin, reg_size);
  void *heap = mmap(NULL, bin->run_size_pages * PAGE_SIZE,
                    PROT_READ | PROT_WRITE | PROT_MTE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  run_t *run =
      (run_t *)malloc(sizeof(run_t) + BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  run_init(run, bin, heap);
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  uint64_t tags[elems];
  for (unsigned i = 0; i < elems; i++) {
    tags[i] = get_tag_from_ptr(reinterpret_cast<uint64_t>(chunks[i]));
  }
  for (unsigned i = 1; i < elems; i++) {
    EXPECT_NE(tags[i - 1], 0);
    EXPECT_NE(tags[i], 0);
    EXPECT_NE(tags[i - 1], tags[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    NeighborDifferentTags, RunUtilsMTEParametrized,
    ::testing::Values(SMALL_SIZE_MIN_REGION, 32, 48, 64, 80, 96, 112, 128, 144,
                      160, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320,
                      336, 352, 368, 384, 400, 416, 432, 448, 464, 480, 496,
                      SMALL_SIZE_MAX_REGION, MEDIUM_SIZE_MIN_REGION, 2048,
                      MEDIUM_SIZE_MAX_REGION));

TEST_F(RunUtilsMTEDeathSmall, DiesOnOverflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[0], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[0]);
  ASSERT_DEATH({ ptr[bin->reg_size] = 'a'; }, ".*");
}

TEST_F(RunUtilsMTEDeathSmall, DiesOnNonLinearOverflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[0], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[0]);
  ASSERT_DEATH({ ptr[bin->reg_size + 42] = 'a'; }, ".*");
}

TEST_F(RunUtilsMTEDeathSmall, DiesOnUnderflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[1], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[1]);
  // Make sure it does not die on normal access
  ptr[0] = 'a';
  ASSERT_DEATH({ ptr[-1] = 'a'; }, ".*");
}

namespace {
class RunUtilsMTEMedium : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    bin_init(bin, MEDIUM_SIZE_MIN_REGION);
    heap = mmap(NULL, bin->run_size_pages * PAGE_SIZE,
                PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
    run_init(run, bin, heap);
  }
};
};  // namespace

using RunUtilsMTEDeathMedium = RunUtilsMTEMedium;

TEST_F(RunUtilsMTEDeathMedium, DiesOnOverflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[0], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[0]);
  ASSERT_DEATH({ ptr[bin->reg_size] = 'a'; }, ".*");
}

TEST_F(RunUtilsMTEDeathMedium, DiesOnNonLinearOverflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[0], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[0]);
  ASSERT_DEATH({ ptr[bin->reg_size + 42] = 'a'; }, ".*");
}

TEST_F(RunUtilsMTEDeathMedium, DiesOnUnderflow) {
  unsigned elems = run->navail;
  std::vector<void *> chunks = get_chunks(run, bin);
  std::memset(chunks[1], 0x42, bin->reg_size);
  char *ptr = static_cast<char *>(chunks[1]);
  // Make sure it does not die on normal access
  ptr[0] = 'a';
  ASSERT_DEATH({ ptr[-1] = 'a'; }, ".*");
}

namespace {
class RunUtilsMTEDeathLarge : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    bin_init(bin, LARGE_SIZE_MIN_REGION);
    heap = mmap(NULL, bin->run_size_pages * PAGE_SIZE,
                PROT_READ | PROT_WRITE | PROT_MTE, MAP_PRIVATE | MAP_ANONYMOUS,
                -1, 0);
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
    run_init(run, bin, heap);
  }
};
};  // namespace

TEST_F(RunUtilsMTEDeathLarge, DiesOnAccessWithDifferentTag) {
  std::vector<void *> chunks = get_chunks(run, bin);
  uint64_t tagged_ptr = reinterpret_cast<uint64_t>(chunks[0]);
  EXPECT_NE(get_tag_from_ptr(tagged_ptr), 0);
  uint64_t res;
  uint64_t excludes = 1;
  __asm__("gmi %0, %1, %2" : "=r"(excludes) : "r"(tagged_ptr), "r"(excludes));
  __asm__("irg %0, %1, %2" : "=r"(res) : "r"(tagged_ptr), "r"(excludes));
  ASSERT_DEATH({ reinterpret_cast<char *>(res)[0] = 'a'; }, ".*") << "Exclude mask: " << std::bitset<16>(excludes) << std::hex
            << ", original ptr: " << tagged_ptr << ", res: " << res
            << std::endl;
}
