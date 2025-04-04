#include "sealloc/container_ll.h"

#include <assert.h>
#include <stddef.h>

#include "sealloc/logging.h"

void ll_init(ll_head_t *head) { head->ll = NULL; }
void ll_add(ll_head_t *head, ll_entry_t *item) {
  se_debug("fd: %p, bk: %p", item->link.fd, item->link.bk);
  assert(item->link.fd == NULL);
  assert(item->link.bk == NULL);
  assert(ll_find(head, item->key) == NULL);

  if (head->ll == NULL) {
    head->ll = item;
    item->link.fd = NULL;
    item->link.bk = NULL;
  } else {
    ll_entry_t *fst = head->ll;
    item->link.bk = NULL;
    item->link.fd = fst;
    fst->link.bk = item;
    head->ll = item;
  }
}
void ll_del(ll_head_t *head, ll_entry_t *item) {
  assert(ll_find(head, item->key) == item);

  if (head->ll == item) {
    if (head->ll->link.fd == NULL)
      head->ll = NULL;
    else {
      head->ll = head->ll->link.fd;
      head->ll->link.bk = NULL;
    }
  }
  if (item->link.fd != NULL) item->link.fd->link.bk = item->link.bk;
  if (item->link.bk != NULL) item->link.bk->link.fd = item->link.fd;
}
ll_entry_t *ll_find(const ll_head_t *head, const void *key) {
  ll_entry_t *res = head->ll;
  for (; res != NULL; res = res->link.fd) {
    if (res->key == key) {
      return res;
    }
  }
  return NULL;
}
