#include <sealloc/bin.h>
#include <sealloc/container_ll.h>
#include <sealloc/logging.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>

void bin_init(bin_t *bin, uint16_t reg_size) {
  if (!IS_ALIGNED(reg_size, SIZE_CLASS_ALIGNMENT)) {
    se_error("reg_size is not aligned");
  }
  ll_init(&bin->run_list_inactive);
  bin->reg_size = reg_size;
  if (IS_SIZE_SMALL(reg_size) || IS_SIZE_MEDIUM(reg_size)) {
    bin->run_size_pages = 2;
  } else {
    // Assuming large size class
    bin->run_size_pages = reg_size / PAGE_SIZE;
  }
  bin->reg_mask_size_bits = ((bin->run_size_pages * PAGE_SIZE) / reg_size) * 2;
  bin->run_list_inactive_cnt = 0;
}

void bin_delete_run(bin_t *bin, run_t *run) {
  ll_del(&bin->run_list_inactive, &run->entry);
}

void bin_retire_current_run(bin_t *bin) {
  ll_add(&bin->run_list_inactive, &bin->runcur->entry);
  bin->runcur = NULL;
}

run_t *bin_get_non_full_run(bin_t *bin) { return bin->runcur; }

run_t *bin_get_run_by_addr(bin_t *bin, void *run_ptr) {
  if (bin->runcur != NULL && bin->runcur->entry.key == run_ptr) {
    return bin->runcur;
  }
  return CONTAINER_OF(ll_find(&bin->run_list_inactive, run_ptr), run_t, entry);
}
