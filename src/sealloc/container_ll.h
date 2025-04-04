/*!
 * @file container_ll.h
 * @brief Linked lists container API for aggregating metadata.
 *
 */
#ifndef SEALLOC_CONTAINER_LL_H_
#define SEALLOC_CONTAINER_LL_H_

struct ll_entry;
struct ll_item;
typedef struct ll_entry ll_entry_t;
typedef struct ll_item ll_item_t;

/*!
 * @brief Size of chunk_node_t in bits
 */
struct ll_item {
  ll_entry_t *fd;
  ll_entry_t *bk;
};

/*!
 * @brief Size of chunk_node_t in bits
 */
struct ll_entry {
  ll_item_t link;
  void *key;
};

/*!
 * @brief Size of chunk_node_t in bits
 */
typedef struct ll_head {
  ll_entry_t *ll;
} ll_head_t;

/*!
 * @brief Initializes head of the list
 *
 * @param[in,out] head Allocated head of the list.
 */
void ll_init(ll_head_t *head);

/*!
 * @brief Adds entry to the list
 *
 * @param[in,out] head Allocated head of the list.
 * @param[in,out] item item to add
 * @pre item->key must be the same type as other elements of the list
 * @pre item cannot already be in the list
 */
void ll_add(ll_head_t *head, ll_entry_t *item);

/*!
 * @brief Deletes entry from the list
 *
 * @param[in,out] head Allocated head of the list.
 * @param[in,out] item item to add
 * @pre item must be in the list starting at head
 */
void ll_del(ll_head_t *head, ll_entry_t *item);

/*!
 * @brief Searches for entry with entry.key == key
 *
 * @param[in,out] head Allocated head of the list.
 * @param[in,out] item item to add
 * @return entry that satisfies entry.key == key
 */
ll_entry_t *ll_find(const ll_head_t *head, const void *key);

#endif /* SEALLOC_CONTAINER_LL_H_ */
