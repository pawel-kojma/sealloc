#include <sealloc/bin.h>
#include <sealloc/generator.h>
#include <sealloc/run.h>
#include <string.h>

int run_init(run_t *run, bin_t *bin, uint64_t secret, void *heap) {
  run->run_heap = heap;
  run->nfree = bin->reg_mask_size * bin->reg_size;

  // We need generator.h definitions first 
  // Then we can choose random generator for out run size
  // run->gen = GENERATORS[rand(secret)];

  // TODO: check if it wont be optimized out by compiler
  memset(run->reg_bitmap, 0, bin->reg_mask_size);
}
