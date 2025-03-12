#include <assert.h>
#include <sealloc/bin.h>
#include <sealloc/run.h>
#include <stdio.h>
#include <stdlib.h>

void test_small(void) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16);
  assert(bin->reg_mask_size_bits == 512 && bin->reg_size == 16 &&
         bin->run_size_pages == 1 && bin->run_list_active == NULL &&
         bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

void test_medium(void) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 2048);
  assert(bin->reg_mask_size_bits == 4 && bin->reg_size == 2048 &&
         bin->run_size_pages == 1 && bin->run_list_active == NULL &&
         bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

void test_large(void) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16384);
  assert(bin->reg_mask_size_bits == 2 && bin->reg_size == 16384 &&
         bin->run_size_pages == 4 && bin->run_list_active == NULL &&
         bin->run_list_active_cnt == 0 && bin->run_list_inactive == NULL);
}

void test_unaligned(void) {
  bin_t *bin = (bin_t *)malloc(sizeof(bin_t));

  bin_init(bin, 16383);
}

int main(void) {
  test_small();
  test_medium();
  test_large();
  test_unaligned();
  return 0;
}
