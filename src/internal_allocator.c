#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

struct internal_allocator_data *internal_alloc_mappings_root;
static unsigned int internal_alloc_secret;

typedef enum internal_allocator_node_type {
  NODE_INVALID = 0,
  NODE_SPLIT = 1,
  NODE_USED = 2,
  NODE_FREE = 3,
} ia_node_t;

void internal_allocator_init(void) {
  struct internal_allocator_data *internal_alloc_mappings_root =
      mmap(NULL, sizeof(struct internal_allocator_data), PROT_READ | PROT_WRITE,
           MAP_PRIVATE, -1, 0);
  if (internal_alloc_mappings_root == MAP_FAILED) {
    sealloc_log("internal_allocator.internal_allocator_init: mmap failed");
  }
  internal_alloc_mappings_root->bk = NULL;
  internal_alloc_mappings_root->fd = NULL;
  internal_alloc_mappings_root->buddy_tree[0] = 3;
  internal_alloc_mappings_root->free_mem =
      sizeof(internal_alloc_mappings_root->memory);
}

static inline ia_node_t get_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (ia_node_t)(mem[word] >> (2 * off)) & 0b11;
}

typedef enum traverse_state {
  DOWN,
  UP_LEFT,
  UP_RIGHT,
} tstate_t;

void *internal_alloc(size_t size) {
  size_t idx, back_idx, cur_size, max_idx;
  uintptr_t ptr;
  ia_node_t node;
  tstate_t state;
  for (struct internal_allocator_data *root = internal_alloc_mappings_root;
       root != NULL; root = root->fd) {
    idx = 1;
    state = DOWN;
    cur_size = sizeof(root->memory);
    ptr = (uintptr_t)&root->memory;
    max_idx = INTERNAL_ALLOC_NO_NODES;
    if (root->free_mem < size) continue;
    while (idx != 1 && state != UP_RIGHT) {
      if (idx >= max_idx / 2) {
        visit(get_tree_item(root->memory, idx));
        state = (idx & 1) ? UP_RIGHT : UP_LEFT;
        idx = idx / 2;
        continue;
      }
      switch (state) {
        case DOWN:
          visit(get_tree_item(root->memory, idx));
          idx = idx * 2;
          break;
        case UP_LEFT:
          idx = idx * 2 + 1;
          state = DOWN;
          break;
        case UP_RIGHT:
          idx = idx / 2;
          state = (idx & 1) ? UP_RIGHT : UP_LEFT;
          break;
      }
    }
  }
  return NULL;
}
