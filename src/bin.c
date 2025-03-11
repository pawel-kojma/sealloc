#include <sealloc/bin.h>
#include <sealloc/generator.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/run.h>

void bin_init(bin_t *bin, size_t reg_size) {
  if (!IS_ALIGNED(reg_size, PAGE_SIZE)) {
    se_error("reg_size is not aligned");
  }
  bin->runcur = NULL;
  bin->run_list = NULL;
  bin->reg_size = reg_size;
  if (IS_SIZE_SMALL(reg_size) || IS_SIZE_MEDIUM(reg_size)) {
    bin->run_size_pages = 1;
  } else {
    // Assuming large size class
    bin->run_size_pages = reg_size / PAGE_SIZE;
  }
  bin->reg_mask_size_bits = ((bin->run_size_pages * PAGE_SIZE) / reg_size) * 2;
  bin->run_list_cnt = 0;
}

void bin_add_run(bin_t *bin, run_t *run) {
  // For now assume runcur is depleted and needs to be moved
  run_t *runprev = bin->runcur;
  runprev->fd = bin->run_list;
  runprev->bk = NULL;
  bin->run_list->bk = runprev;
  bin->run_list = run;
  bin->runcur = run;
}

void bin_del_run(bin_t *bin, run_t *run) {
  if (bin->runcur == run) {
    se_error("run cannot be deleted, because run == runcur");
  }
  run->bk->fd = run->fd;
  run->fd->bk = run->bk;
  internal_free(run);
}
