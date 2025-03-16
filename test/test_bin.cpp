#include <gtest/gtest.h>

#include <cstring>
#include <random>

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
  EXPECT_TRUE(bin->reg_mask_size_bits == 512 && bin->reg_size == 16 &&
              bin->run_size_pages == 1 && bin->run_list_active == NULL &&
              bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

TEST(BinUtils, BinInitMedium) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 2048);
  EXPECT_TRUE(bin->reg_mask_size_bits == 4 && bin->reg_size == 2048 &&
              bin->run_size_pages == 1 && bin->run_list_active == NULL &&
              bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

TEST(BinUtils, BinInitLarge) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16384);
  EXPECT_TRUE(bin->reg_mask_size_bits == 2 && bin->reg_size == 16384 &&
              bin->run_size_pages == 4 && bin->run_list_active == NULL &&
              bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

TEST(BinUtils, BinInitUnaligned) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  EXPECT_DEATH({ bin_init(bin, 16383); }, ".*reg_size is not aligned.*");
}

TEST(BinUtils, BinAddRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  run_t *run = alloc_run(bin);
  run_init(run, bin, NULL);
  bin_add_fresh_run(bin, run);
  EXPECT_EQ(run, bin_get_non_full_run(bin));
}

TEST(BinUtils, BinAddRunFillRun) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  run_t *run1 = alloc_run(bin), *run2 = alloc_run(bin);
  run_init(run1, bin, NULL);
  run_init(run2, bin, NULL);
  size_t elems = run1->navail;
  bin_add_fresh_run(bin, run1);
  bin_add_fresh_run(bin, run2);
  EXPECT_EQ(bin->run_list_active, run2);
  EXPECT_EQ(bin->run_list_active->fd, run1);
  EXPECT_EQ(bin->run_list_active->bk, nullptr);
  EXPECT_EQ(bin->run_list_active->fd->bk, run2);
  EXPECT_EQ(bin->run_list_active->fd->fd, nullptr);
  for (int i = 0; i < elems; i++) {
    (void *)run_allocate(run1, bin);
  }
}
