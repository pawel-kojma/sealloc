#include "sealloc/bin.h"

#include <assert.h>

#include "sealloc/container_ll.h"
#include "sealloc/logging.h"
#include "sealloc/random.h"
#include "sealloc/run.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

void bin_init(bin_t *bin, unsigned reg_size) {
  assert(reg_size >= 1);
  assert(reg_size <= LARGE_SIZE_MAX_REGION);
  assert(is_size_aligned(reg_size));

  ll_init(&bin->run_list_inactive);
  ll_init(&bin->run_list_active);
  if (IS_SIZE_SMALL(reg_size)) {
    bin->run_size_pages = 2;
  } else if (IS_SIZE_MEDIUM(reg_size)) {
    bin->run_size_pages = 2;
  } else {
    // Assuming large size class
    bin->run_size_pages = reg_size / PAGE_SIZE;
  }
  bin->reg_size = reg_size;
  bin->avail_regs = 0;
  bin->reg_mask_size_bits = ((bin->run_size_pages * PAGE_SIZE) / reg_size) * 2;
  bin->run_list_active_cnt = 0;
}

void bin_add_run(bin_t *bin, run_t *run) {
  assert(bin->reg_size != 0);
  assert(run->entry.key != NULL);
  assert(run->navail == (bin->reg_mask_size_bits / 2));
  assert(bin_get_run_by_addr(bin, run->entry.key) == NULL);
  bin->avail_regs += run->navail;
  bin->run_list_active_cnt++;
  ll_add(&bin->run_list_active, &run->entry);
}

void bin_delete_run(bin_t *bin, run_t *run) {
  assert(bin->reg_size != 0);
  assert(bin_get_run_by_addr(bin, run->entry.key) == run);
  ll_del(&bin->run_list_inactive, &run->entry);
}

run_t *bin_get_run_for_allocation(bin_t *bin) {
  assert(bin->reg_size != 0);
  assert(bin->avail_regs > 0);
  assert(bin->run_list_active_cnt > 0);

  unsigned run_idx = splitmix32() % bin->run_list_active_cnt;
  ll_entry_t *entry = bin->run_list_active.ll;
  se_debug("Before search, run_idx : %u", run_idx);
  for (; entry != NULL; entry = entry->link.fd) {
    if (run_idx == 0) {
      bin->avail_regs--;
      return CONTAINER_OF(entry, run_t, entry);
    }
    run_idx--;
  }
  se_error("List was too short, run_idx : %u", run_idx);
}

void bin_retire_run(bin_t *bin, run_t *run) {
  assert(bin->reg_size != 0);
  assert(run->navail == 0);
  ll_del(&bin->run_list_active, &run->entry);
  ll_add(&bin->run_list_inactive, &run->entry);
  bin->run_list_active_cnt--;
}

run_t *bin_get_run_by_addr(bin_t *bin, const void *run_ptr) {
  assert(bin->reg_size != 0);
  run_t *run =
      CONTAINER_OF(ll_find(&bin->run_list_active, run_ptr), run_t, entry);
  if (run == NULL)
    return CONTAINER_OF(ll_find(&bin->run_list_inactive, run_ptr), run_t, entry);
  return run;
}
