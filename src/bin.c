#include "sealloc/bin.h"

#include <assert.h>

#include "sealloc/container_ll.h"
#include "sealloc/run.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

void bin_init(bin_t *bin, unsigned reg_size) {
  assert(reg_size >= 1);
  assert(reg_size <= LARGE_SIZE_MAX_REGION);
  assert(is_size_aligned(reg_size));

  ll_init(&bin->run_list_inactive);
  if (IS_SIZE_SMALL(reg_size)) {
    bin->run_size_pages = 2;
  } else if (IS_SIZE_MEDIUM(reg_size)) {
    bin->run_size_pages = 2;
  } else {
    // Assuming large size class
    bin->run_size_pages = reg_size / PAGE_SIZE;
  }
  bin->reg_size = reg_size;
  bin->reg_mask_size_bits = ((bin->run_size_pages * PAGE_SIZE) / reg_size) * 2;
  bin->run_list_inactive_cnt = 0;
}

void bin_delete_run(bin_t *bin, run_t *run) {
  assert(bin->reg_size != 0);
  assert(bin_get_run_by_addr(bin, run->entry.key) == run);
  ll_del(&bin->run_list_inactive, &run->entry);
}

void bin_retire_current_run(bin_t *bin) {
  assert(bin->reg_size != 0);
  assert(bin->runcur != NULL);
  ll_add(&bin->run_list_inactive, &bin->runcur->entry);
  bin->runcur = NULL;
}

run_t *bin_get_run_by_addr(bin_t *bin, const void *const run_ptr) {
  assert(bin->reg_size != 0);
  if (bin->runcur != NULL && bin->runcur->entry.key == run_ptr) {
    return bin->runcur;
  }
  return CONTAINER_OF(ll_find(&bin->run_list_inactive, run_ptr), run_t, entry);
}
