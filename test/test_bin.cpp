#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/bin.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
}

run_t *alloc_run(bin_t *bin) {
  return (run_t *)malloc(sizeof(run_t) +
                         BITS2BYTES_CEIL(bin->reg_mask_size_bits));
}

TEST(BinUtils, BinInitSmall) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  EXPECT_EQ(bin->reg_mask_size_bits, 1024);
  EXPECT_EQ(bin->reg_size, 16);
  EXPECT_EQ(bin->run_size_pages, 2);
  EXPECT_EQ(bin->run_list_inactive.ll, nullptr);
  EXPECT_EQ(bin->run_list_active.ll, nullptr);
  EXPECT_EQ(bin->avail_regs, 0);
  EXPECT_EQ(bin->run_list_active_cnt, 0);
}

TEST(BinUtils, BinInitMedium) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 2048);
  EXPECT_EQ(bin->reg_mask_size_bits, 8);
  EXPECT_EQ(bin->reg_size, 2048);
  EXPECT_EQ(bin->run_size_pages, 2);
  EXPECT_EQ(bin->run_list_inactive.ll, nullptr);
  EXPECT_EQ(bin->run_list_active.ll, nullptr);
  EXPECT_EQ(bin->avail_regs, 0);
  EXPECT_EQ(bin->run_list_active_cnt, 0);
}

TEST(BinUtils, BinInitLarge) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16384);
  EXPECT_EQ(bin->reg_mask_size_bits, 2);
  EXPECT_EQ(bin->reg_size, 16384);
  EXPECT_EQ(bin->run_size_pages, 4);
  EXPECT_EQ(bin->run_list_inactive.ll, nullptr);
  EXPECT_EQ(bin->run_list_active.ll, nullptr);
  EXPECT_EQ(bin->avail_regs, 0);
  EXPECT_EQ(bin->run_list_active_cnt, 0);
}

TEST(BinUtils, BinAddRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin);
  void *run_heap = malloc(16);
  run_init(run1, bin, run_heap);
  size_t elems = run1->navail;
  bin_add_run(bin, run1);
  EXPECT_EQ(bin->avail_regs, elems);
  EXPECT_EQ(bin->run_list_active_cnt, 1);
}

TEST(BinUtils, BinAddRunFillRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin);
  void *run_heap = malloc(16);
  EXPECT_NE(run_heap, nullptr);
  run_init(run1, bin, run_heap);
  size_t elems = run1->navail;
  bin_add_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    (void)run_allocate(run1, bin);
  }
  bin_retire_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
}

TEST(BinUtils, BinDelRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));
  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin);
  void *run_heap = malloc(16);
  EXPECT_NE(run_heap, nullptr);
  run_init(run1, bin, run_heap);
  size_t elems = run1->navail;
  void *ch[elems];
  bin_add_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    ch[i] = run_allocate(run1, bin);
  }
  bin_retire_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    run_deallocate(run1, bin, ch[i]);
  }
  EXPECT_TRUE(run_is_freeable(run1, bin));
  bin_delete_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), nullptr);
}

TEST(BinUtils, BinGetRunForAllocationSingle) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));
  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin);
  void *run_heap = malloc(16);
  EXPECT_NE(run_heap, nullptr);
  run_init(run1, bin, run_heap);
  bin_add_run(bin, run1);
  EXPECT_EQ(bin_get_run_for_allocation(bin), run1);
}

TEST(BinUtils, BinGetRunForAllocationMulti) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));
  bin_init(bin, 16);
  run_t *runs[30];
  void *run_heap;
  for (int i = 0; i < 30; i++) {
    runs[i] = alloc_run(bin);
    run_heap = malloc(16);
    EXPECT_NE(run_heap, nullptr);
    run_init(runs[i], bin, run_heap);
    bin_add_run(bin, runs[i]);
  }
  EXPECT_EQ(bin_get_run_for_allocation(bin), runs[6]);
}
