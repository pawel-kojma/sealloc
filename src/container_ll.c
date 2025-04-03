#include "sealloc/container_ll.h"
#include <stddef.h>

void ll_init(ll_head_t *head) { head->ll = NULL; }
void ll_add(ll_head_t *head, ll_entry_t *item) {
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
ll_entry_t *ll_find(ll_head_t *head, void *key) {
  ll_entry_t *res = head->ll;
  for (; res != NULL; res = res->link.fd) {
    if (res->key == key) {
      return res;
    }
  }
  return NULL;
}
