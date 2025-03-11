#include <sealloc/bin.h>
#include <sealloc/generator.h>
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
