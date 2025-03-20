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
  EXPECT_TRUE(bin->reg_mask_size_bits == 1024 && bin->reg_size == 16 &&
              bin->run_size_pages == 2 && bin->run_list_inactive.ll == NULL);
}

TEST(BinUtils, BinInitMedium) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 2048);
  EXPECT_TRUE(bin->reg_mask_size_bits == 8 && bin->reg_size == 2048 &&
              bin->run_size_pages == 2 && bin->run_list_inactive.ll == NULL);
}

TEST(BinUtils, BinInitLarge) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16384);
  EXPECT_TRUE(bin->reg_mask_size_bits == 2 && bin->reg_size == 16384 &&
              bin->run_size_pages == 4 && bin->run_list_inactive.ll == NULL);
}

TEST(BinUtils, BinInitUnaligned) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  EXPECT_DEATH({ bin_init(bin, 16383); }, ".*reg_size is not aligned.*");
}

TEST(BinUtils, BinAddRunFillRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin);
  void *run_heap = malloc(16);
  EXPECT_NE(run_heap, nullptr);
  run_init(run1, bin, run_heap);
  size_t elems = run1->navail;
  bin->runcur = run1;
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    (void)run_allocate(run1, bin);
  }
  bin_retire_current_run(bin);
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
  bin->runcur = run1;
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    ch[i] = run_allocate(run1, bin);
  }
  bin_retire_current_run(bin);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), run1);
  for (int i = 0; i < elems; i++) {
    run_deallocate(run1, bin, ch[i]);
  }
  EXPECT_TRUE(run_is_freeable(run1, bin));
  bin_delete_run(bin, run1);
  EXPECT_EQ(bin_get_run_by_addr(bin, run1->entry.key), nullptr);
}
