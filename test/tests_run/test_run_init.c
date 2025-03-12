#include <assert.h>
#include <sealloc/bin.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
#include <stdio.h>
#include <stdlib.h>

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

#define PAGESIZE 4096

void test_small(void) {
  bin_t *bin;
  run_t *run;
  void *heap = malloc(1);
  init_splitmix32(1);
  bin = (bin_t *)malloc(sizeof(bin_t));
  bin->runcur = NULL;
  bin->run_list = NULL;
  bin->reg_size = 16;
  bin->run_size_pages = 1;
  bin->reg_mask_size_bits = (PAGESIZE / bin->reg_size) * 2;

  run =
      (run_t *)malloc(sizeof(run_t) + BITS2BYTES_CEIL(bin->reg_mask_size_bits));

  run_init(run, bin, heap);
  assert(run->navail == 256);
  assert(run->nfreed == 0);
  assert(run->gen == 229);
  assert(run->current_idx == 181);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    assert(run->reg_bitmap[i] == 0x0);
  }
}

void test_medium(void) {
  bin_t *bin;
  run_t *run;
  void *heap = malloc(1);
  init_splitmix32(1);
  bin = (bin_t *)malloc(sizeof(bin_t));
  bin->runcur = NULL;
  bin->run_list = NULL;
  bin->reg_size = 1024;
  bin->run_size_pages = 1;
  bin->reg_mask_size_bits = (PAGESIZE / bin->reg_size) * 2;

  run =
      (run_t *)malloc(sizeof(run_t) + BITS2BYTES_CEIL(bin->reg_mask_size_bits));

  run_init(run, bin, heap);
  assert(run->navail == 4);
  assert(run->nfreed == 0);
  assert(run->gen == 1);
  assert(run->current_idx == 1);
  for (int i = 0; i < BITS2BYTES_CEIL(bin->reg_mask_size_bits); i++) {
    assert(run->reg_bitmap[i] == 0x0);
  }
}

int main(void) {
  test_small();
  test_medium();
  return 0;
}
