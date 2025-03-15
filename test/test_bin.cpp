#include <gtest/gtest.h>

#include <cstring>
#include <random>

extern "C" {
#include <sealloc/bin.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
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

  ASSERT_DEATH({ bin_init(bin, 16383); }, ".*reg_size is not aligned.*");
}
