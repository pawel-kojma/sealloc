#include <gtest/gtest.h>

#include <cstring>
#include <random>

extern "C" {
#include <sealloc/bin.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
}

/* First 10 values of splitmix32 for seed=1
    1580013426
    914163893
    1798215561
    3547142611
    2321361971
    2156534659
    3121840663
    3040040295
    920009879
    3576304375
 */

using random_bytes_engine =
    std::independent_bits_engine<std::default_random_engine, 8, unsigned char>;

namespace {
class RunUtilsTestSmall : public ::testing::Test {
 protected:
  bin_t *bin;
  run_t *run;
  void *heap;
  void SetUp() override {
    init_splitmix32(1);
    bin = (bin_t *)malloc(sizeof(bin_t));
    bin->run_list_active = NULL;
    bin->run_list_active_cnt = 0;
    bin->run_list_inactive = NULL;
    bin->run_list_inactive_cnt = 0;
    bin->reg_size = 16;
    bin->run_size_pages = 1;
    bin->reg_mask_size_bits = (PAGE_SIZE / bin->reg_size) * 2;
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestSmall, RunInit) {
  heap = malloc(1);
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 256);
  EXPECT_EQ(run->run_heap, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->gen, 229);
  EXPECT_EQ(run->current_idx, 181);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestSmall, RunAllocate) {
  heap = malloc(1);
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits) - 1; i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x55);
  }
}

TEST_F(RunUtilsTestSmall, RunDeallocate) {
  heap = malloc(bin->run_size_pages * PAGE_SIZE);
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
    run_deallocate(run, bin, chunks[i]);
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestSmall, MemoryIntegrity) {
  random_bytes_engine engine{1};
  run_init(run, bin, heap);

  int elems = run->navail;
  void *chunks_dut[elems];
  std::vector<unsigned char> chunks_exp[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
    chunks_exp[i].reserve(bin->reg_size);
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
  ASSERT_DEATH(
      {
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
      },
      ".* Provided ptr not freeable .*");
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
    bin->run_list_active = NULL;
    bin->run_list_active_cnt = 0;
    bin->run_list_inactive = NULL;
    bin->run_list_inactive_cnt = 0;
    bin->reg_size = 1024;
    bin->run_size_pages = 1;
    bin->reg_mask_size_bits = (PAGE_SIZE / bin->reg_size) * 2;
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestMedium, RunInit) {
  heap = malloc(1);
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 4);
  EXPECT_EQ(run->run_heap, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->gen, 1);
  EXPECT_EQ(run->current_idx, 1);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestMedium, RunAllocate) {
  heap = malloc(1);
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits) - 1; i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x55);
  }
}

TEST_F(RunUtilsTestMedium, RunDeallocate) {
  heap = malloc(bin->run_size_pages * PAGE_SIZE);
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
    run_deallocate(run, bin, chunks[i]);
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestMedium, MemoryIntegrity) {
  random_bytes_engine engine{1};
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  std::vector<unsigned char> chunks_exp[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
    chunks_exp[i].reserve(bin->reg_size);
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
  ASSERT_DEATH(
      {
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
      },
      ".* Provided ptr not freeable .*");
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
    bin->run_list_active = NULL;
    bin->run_list_active_cnt = 0;
    bin->run_list_inactive = NULL;
    bin->run_list_inactive_cnt = 0;
    bin->reg_size = 8192;
    bin->run_size_pages = 2;
    bin->reg_mask_size_bits = 2;
    run = (run_t *)malloc(sizeof(run_t) +
                          BITS2BYTES_CEIL(bin->reg_mask_size_bits));
  }
};

TEST_F(RunUtilsTestLarge, RunInit) {
  heap = malloc(1);
  run_init(run, bin, heap);
  EXPECT_EQ(run->navail, 1);
  EXPECT_EQ(run->run_heap, heap);
  EXPECT_EQ(run->nfreed, 0);
  EXPECT_EQ(run->current_idx, 0);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    EXPECT_EQ(run->reg_bitmap[i], 0x0);
  }
}

TEST_F(RunUtilsTestLarge, RunAllocate) {
  heap = malloc(1);
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks[elems];
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  EXPECT_EQ(run->reg_bitmap[0], 0x01);
}

TEST_F(RunUtilsTestLarge, RunDeallocate) {
  heap = malloc(bin->run_size_pages * PAGE_SIZE);
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
    run_deallocate(run, bin, chunks[i]);
  }
  EXPECT_TRUE(run_is_freeable(run, bin));
}

TEST_F(RunUtilsTestLarge, MemoryIntegrity) {
  random_bytes_engine engine{1};
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  std::vector<unsigned char> chunks_exp[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
    chunks_exp[i].reserve(bin->reg_size);
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

TEST_F(RunUtilsTestLarge, DoubleFree) {
  run_init(run, bin, heap);
  int elems = run->navail;
  void *chunks_dut[elems];
  for (int i = 0; i < elems; i++) {
    chunks_dut[i] = run_allocate(run, bin);
  }
  EXPECT_TRUE(run_is_depleted(run));
  ASSERT_DEATH(
      {
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
        run_deallocate(run, bin, chunks_dut[13243 % elems]);
      },
      ".* Provided ptr not freeable .*");
}

}  // namespace
