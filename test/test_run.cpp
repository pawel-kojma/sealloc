#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <random>

extern "C" {
#include <sealloc/bin.h>
#include <sealloc/chunk.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

/* First 10 values of splitmix32 for seed=1
$0 = 1580013426
$1 = 350525680
$2 = 3524174333
$3 = 3011703609
$4 = 643872864
$5 = 2282937712
$6 = 2300340400
$7 = 3453518249
$8 = 3797083327
$9 = 2737271936
*/

using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, 8, unsigned char>;

// Get state of a region from bitmap
static bstate_t get_bitmap_item(uint8_t *mem, size_t idx) {
  size_t word = idx / 4;
  size_t off = idx % 4;
  return (bstate_t)(mem[word] >> (2 * off) & 3);
}

namespace {
class RunUtilsTestSmall : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    ll_init(&bin->run_list_inactive);
    ll_init(&bin->run_list_active);
    bin->run_list_active_cnt = 0;
    bin->avail_regs = 0;
    bin->reg_size = SMALL_SIZE_MIN_REGION;
    bin->run_size_pages = RUN_SIZE_SMALL_PAGES;
    bin->reg_mask_size_bits =
        ((bin->run_size_pages * PAGE_SIZE) / bin->reg_size) * 2;
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestSmall, RunInit) {
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 1024);
  EXPECT_EQ(run->entry.key, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->gen, 741);
  EXPECT_EQ(run->current_idx, 240);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestSmall, RunAllocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < elems; i++) {
    EXPECT_EQ(get_bitmap_item(run->reg_bitmap, i), STATE_ALLOC);
  }
}

TEST_F(RunUtilsTestSmall, RunDeallocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  run_init(run, bin, heap);
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_FALSE(run_is_freeable(run, bin));
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < elems; i++) {
    EXPECT_TRUE(run_deallocate(run, bin, chunks[i]));
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestSmall, MemoryIntegrity) {
  void *heap = malloc(RUN_SIZE_SMALL_BYTES);
  run_init(run, bin, heap);
  std::independent_bits_engine<std::default_random_engine, 8, unsigned char>
      engine{1};
  int elems = run->navail;
  void *chunks_dut[elems];
  std::vector<unsigned char> chunks_exp[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
    chunks_exp[i].resize(bin->reg_size);
    std::generate(chunks_exp[i].begin(), chunks_exp[i].end(), std::ref(engine));
    std::memcpy(chunks_dut[i], chunks_exp[i].data(), bin->reg_size);
  }
  for (int i = 0; i < elems; i++) {
    EXPECT_PRED3([](const void *a, const void *b,
                    size_t s) { return std::memcmp(a, b, s) == 0; },
                 chunks_dut[i], chunks_exp[i].data(), bin->reg_size)
        << "Chunks of size " << bin->reg_size << " at index " << i << " differ";
  }
}

TEST_F(RunUtilsTestSmall, DoubleFree) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  EXPECT_TRUE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
  EXPECT_FALSE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
}

}  // namespace

namespace {
class RunUtilsTestMedium : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    ll_init(&bin->run_list_inactive);
    ll_init(&bin->run_list_active);
    bin->run_list_active_cnt = 0;
    bin->avail_regs = 0;
    bin->reg_size = MEDIUM_SIZE_MIN_REGION;
    bin->run_size_pages = RUN_SIZE_MEDIUM_PAGES;
    bin->reg_mask_size_bits =
        ((bin->run_size_pages * PAGE_SIZE) / bin->reg_size) * 2;
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestMedium, RunInit) {
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 16);
  EXPECT_EQ(run->entry.key, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->gen, 5);
  EXPECT_EQ(run->current_idx, 0);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestMedium, RunAllocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < elems; i++) {
    EXPECT_EQ(get_bitmap_item(run->reg_bitmap, i), STATE_ALLOC);
  }
}

TEST_F(RunUtilsTestMedium, RunDeallocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  run_init(run, bin, heap);
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_FALSE(run_is_freeable(run, bin));
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < elems; i++) {
    EXPECT_TRUE(run_deallocate(run, bin, chunks[i]));
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestMedium, MemoryIntegrity) {
  heap = malloc(RUN_SIZE_MEDIUM_BYTES);
  run_init(run, bin, heap);
  int elems = run->navail;
  std::independent_bits_engine<std::default_random_engine, 8, unsigned char>
      engine{1};
  void *chunks_dut[elems];
  std::vector<unsigned char> chunks_exp[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
    chunks_exp[i].resize(bin->reg_size);
    std::generate(chunks_exp[i].begin(), chunks_exp[i].end(), std::ref(engine));
    std::memcpy(chunks_dut[i], chunks_exp[i].data(), bin->reg_size);
  }

  for (int i = 0; i < elems; i++) {
    EXPECT_PRED3([](const void *a, const void *b,
                    size_t s) { return std::memcmp(a, b, s) == 0; },
                 chunks_dut[i], chunks_exp[i].data(), bin->reg_size)
        << "Chunks of size " << bin->reg_size << " at index " << i << " differ";
  }
}

TEST_F(RunUtilsTestMedium, DoubleFree) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  EXPECT_TRUE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
  EXPECT_FALSE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
}

}  // namespace

namespace {
class RunUtilsTestLarge : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    ll_init(&bin->run_list_inactive);
    ll_init(&bin->run_list_active);
    bin->run_list_active_cnt = 0;
    bin->avail_regs = 0;
    bin->reg_size = LARGE_SIZE_MIN_REGION;
    bin->run_size_pages = 4;
    bin->reg_mask_size_bits = 2;
    heap = malloc(bin->run_size_pages * PAGE_SIZE);
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestLarge, RunInit) {
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 1);
  EXPECT_EQ(run->entry.key, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->current_idx, 0);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestLarge, RunAllocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  EXPECT_EQ((bstate_t)run->reg_bitmap[0], STATE_ALLOC);
}

TEST_F(RunUtilsTestLarge, RunDeallocate) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  run_init(run, bin, heap);
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_FALSE(run_is_freeable(run, bin));
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < elems; i++) {
    EXPECT_TRUE(run_deallocate(run, bin, chunks[i]));
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestLarge, DoubleFree) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  EXPECT_TRUE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
  EXPECT_FALSE(run_deallocate(run, bin, chunks_dut[13243 % elems]));
}

}  // namespace
