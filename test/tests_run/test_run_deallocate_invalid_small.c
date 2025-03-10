#include <assert.h>
#include <sealloc/bin.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGESIZE 4096

void test_small(void) {
  bin_t *bin;
  run_t *run;
  void *chunks[256];
  void *heap = malloc(4096);
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
  int elems = run->nfree;
  for (int i = 0; i < elems; i++) {
    chunks[i] = run_allocate(run, bin);
  }
  assert(run_is_full(run) == true);
  run_deallocate(run, bin, chunks[17]);
  run_deallocate(run, bin, chunks[17]);
}

int main(void) {
  test_small();
  return 0;
}
