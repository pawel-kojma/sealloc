#include <sealloc/bin.h>
#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <sealloc/random.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>

void bin_init(bin_t *bin, size_t reg_size) {
  if (!IS_ALIGNED(reg_size, PAGE_SIZE)) {
    se_error("reg_size is not aligned");
  }
  bin->run_list_active = NULL;
  bin->run_list_inactive = NULL;
  bin->reg_size = reg_size;
  if (IS_SIZE_SMALL(reg_size) || IS_SIZE_MEDIUM(reg_size)) {
    bin->run_size_pages = 1;
  } else {
    // Assuming large size class
    bin->run_size_pages = reg_size / PAGE_SIZE;
  }
  bin->reg_mask_size_bits = ((bin->run_size_pages * PAGE_SIZE) / reg_size) * 2;
  bin->run_list_active_cnt = 0;
  bin->run_list_dead_cnt = 0;
}

void bin_add_fresh_run(bin_t *bin, run_t *run) {
  run->fd = bin->run_list_active;
  run->bk = NULL;
  bin->run_list_active->bk = run;
  bin->run_list_active = run;
}

void bin_del_dead_run(bin_t *bin, run_t *run) {
  bool found = false;
  if (!run_is_depleted(run)) {
    se_error("Run is active");
  }
  if (bin->run_list_active == NULL) return;
  if (run == bin->run_list_active) {
    bin->run_list_active = NULL;
    return;
  }
  for (run_t *r = bin->run_list_active; r != NULL; r = r->fd) {
    if (r == run) {
      found = true;
      if (run->bk) run->bk->fd = run->fd;
      if (run->fd) run->fd->bk = run->bk;
    }
  }
  if (!found) {
    se_error("Run could not be found.");
  }
}

run_t *bin_get_non_full_run(bin_t *bin) {
  if (bin->run_list_active == 0) {
    return NULL;
  }
  uint32_t idx = splitmix32() % bin->run_list_active_cnt;
  run_t *r = bin->run_list_active;
  for (; r != NULL; r = r->fd) {
    if (idx == 0) break;
    idx--;
  }
  return r;
}
