/* Linked lists container API for aggregating structures */

#ifndef SEALLOC_CONTAINER_LL_H_
#define SEALLOC_CONTAINER_LL_H_

struct ll_entry;
struct ll_item;
typedef struct ll_entry ll_entry_t;
typedef struct ll_item ll_item_t;

struct ll_item {
  ll_entry_t *fd;
  ll_entry_t *bk;
};

struct ll_entry {
  ll_item_t link;
  void *key;
};

typedef struct ll_head {
  ll_entry_t *ll;
} ll_head_t;

void ll_init(ll_head_t *head);
void ll_add(ll_head_t *head, ll_entry_t *item);
void ll_del(ll_head_t *head, ll_entry_t *item);
ll_entry_t *ll_find(ll_head_t *head, void *key);

#endif /* SEALLOC_CONTAINER_LL_H_ */
