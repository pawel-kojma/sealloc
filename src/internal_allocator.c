#include <sealloc/internal_allocator.h>
#include <sealloc/logging.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

static struct internal_allocator_data *internal_alloc_mappings_root;

typedef enum internal_allocator_node_type {
  NODE_INVALID = 0,
  NODE_SPLIT = 1,
  NODE_USED = 2,
  NODE_FREE = 3,
} ia_node_t;

typedef enum traverse_state {
  DOWN,
  UP_LEFT,
  UP_RIGHT,
} tstate_t;

// Get a fresh mapping from kernel for further allocations
// Link it to the beginning of the list
static int morecore(void) {
  struct internal_allocator_data *map = (struct internal_allocator_data *)mmap(
      NULL, sizeof(struct internal_allocator_data), PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (internal_alloc_mappings_root == MAP_FAILED) {
    sealloc_log("internal_allocator.morecore: mmap failed");
    return -1;
  }
  internal_alloc_mappings_root->bk = map;
  map->fd = internal_alloc_mappings_root;
  map->bk = NULL;
  map->buddy_tree[0] = (uint8_t)NODE_FREE;
  map->free_mem = sizeof(internal_alloc_mappings_root->memory);
  internal_alloc_mappings_root = map;
  return 0;
}

// Initialize internal allocator with fresh mapping
int internal_allocator_init(void) {
  internal_alloc_mappings_root = (struct internal_allocator_data *)mmap(
      NULL, sizeof(struct internal_allocator_data), PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (internal_alloc_mappings_root == MAP_FAILED) {
    sealloc_log("internal_allocator.internal_allocator_init: mmap failed");
    return -1;
  }
  internal_alloc_mappings_root->bk = NULL;
  internal_alloc_mappings_root->fd = NULL;
  internal_alloc_mappings_root->buddy_tree[0] = (uint8_t)NODE_FREE;
  internal_alloc_mappings_root->free_mem =
      sizeof(internal_alloc_mappings_root->memory);
  return 0;
}

static ia_node_t get_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (ia_node_t)(mem[word] >> (2 * off)) & 3;
}

static void set_tree_item(uint8_t *mem, size_t idx, ia_node_t node_state) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  uint8_t erase_bits = 3;  // 0b11
  // First, clear the bits we want to set in mem[word]
  // Second, set two node_state bits in their place
  mem[word] = (mem[word] & ~(erase_bits << (off * 2))) |
              ((uint8_t)node_state << (off * 2));
}

void *internal_alloc_with_root(struct internal_allocator_data *root,
                               size_t size) {
  size_t idx, cur_size, max_idx;
  uintptr_t ptr;
  ia_node_t node;
  tstate_t state;
  idx = 1;
  state = DOWN;
  cur_size = sizeof(root->memory);
  ptr = (uintptr_t)&root->memory;
  max_idx = INTERNAL_ALLOC_NO_NODES;
  if (root->free_mem < size) return NULL;

  // Search stops when we come back to the root from the right child
  while (!(idx == 1 && state == UP_RIGHT)) {
    // If we are in a leaf node, visit and go up.
    if (idx >= (max_idx + 1) / 2) {
      node = get_tree_item(root->buddy_tree, idx);
      switch (node) {
        case NODE_FREE:
          // There is no further splitting, allocate here.
          set_tree_item(root->buddy_tree, idx, NODE_USED);
          return (void *)ptr;
          break;
        case NODE_USED:
          // Node is used, nothing we can do, go up.
          state = (idx & 1) ? UP_RIGHT : UP_LEFT;
          ptr = (idx & 1) ? ptr - cur_size : ptr;
          idx = idx / 2;
          cur_size = cur_size * 2;
          break;
        case NODE_SPLIT:
          // Invalid state, leaf node cannot be split.
          sealloc_log(
              "internal_allocator.internal_alloc: leaf node state is "
              "NODE_SPLIT");
          exit(1);
        case NODE_INVALID:
          // Invalid state, should never happen.
          sealloc_log(
              "internal_allocator.internal_alloc: leaf node state is "
              "NODE_INVALID");
          exit(1);
      }
      continue;
    }
    switch (state) {
      case DOWN:
        node = get_tree_item(root->buddy_tree, idx);
        switch (node) {
          case NODE_SPLIT:
            // If size cannot fit into this chunk, then backtrack.
            if (size >= cur_size) {
              state = (idx & 1) ? UP_RIGHT : UP_LEFT;
              ptr = (idx & 1) ? ptr - cur_size : ptr;
              idx = idx / 2;
              cur_size = cur_size * 2;
            }
            // If size still fits, but node is split, then go to left child.
            else {
              idx = idx * 2;
              cur_size = cur_size / 2;
            }
            break;
          case NODE_FREE:
            // If size fits current node and the node is free,
            // then allocate it.
            if (cur_size / 2 < size && size <= cur_size) {
              set_tree_item(root->buddy_tree, idx, NODE_USED);
              return (void *)ptr;
            }
            // We know node above us must have been split.
            // In this case: size < cur_size / 2
            // Split the node and go to the left child.
            else {
              set_tree_item(root->buddy_tree, idx, NODE_SPLIT);
              set_tree_item(root->buddy_tree, idx * 2, NODE_FREE);
              set_tree_item(root->buddy_tree, idx * 2 + 1, NODE_FREE);
              cur_size = cur_size / 2;
              idx = idx * 2;
            }
            break;
          case NODE_USED:
            // Node is used, nothing we can do, go up.
            state = (idx & 1) ? UP_RIGHT : UP_LEFT;
            ptr = (idx & 1) ? ptr - cur_size : ptr;
            idx = idx / 2;
            cur_size = cur_size * 2;
            break;
          case NODE_INVALID:
            // Invalid state, should never happen.
            sealloc_log("internal_allocator.internal_alloc: NODE_INVALID");
            exit(1);
        }
        break;
      case UP_LEFT:
        cur_size = cur_size / 2;
        ptr = ptr + cur_size;
        idx = idx * 2 + 1;
        state = DOWN;
        break;
      case UP_RIGHT:
        cur_size = cur_size * 2;
        idx = idx / 2;
        state = (idx & 1) ? UP_RIGHT : UP_LEFT;
        break;
    }
  }
  return NULL;
}

void *internal_alloc(size_t size) {
  void *alloc;

  // Loop over memory mappings and try to satisfy the request
  for (struct internal_allocator_data *root = internal_alloc_mappings_root;
       root != NULL; root = root->fd) {
    alloc = internal_alloc_with_root(root, size);
    if (alloc != NULL) return alloc;
  }
  // No mapping can satisfy the request, try to get more memory
  int res = morecore();

  // Cannot get more memory
  if (res < 0) return NULL;

  // Allocate with fresh memory mapping
  return internal_alloc_with_root(internal_alloc_mappings_root, size);
}

void internal_free(void *ptr) {
  size_t idx = 1, cur_size = INTERNAL_ALLOC_CHUNK_SIZE_BYTES, neigh_idx;
  ptrdiff_t ptr_cur, ptr_dest = (ptrdiff_t)ptr;
  struct internal_allocator_data *root = NULL;
  for (root = internal_alloc_mappings_root; root != NULL; root = root->fd) {
    if ((ptrdiff_t)&root->memory <= ptr_dest &&
        ptr_dest <= (ptrdiff_t)root->memory + sizeof(root->memory))
      break;
  }
  if (!root) {
    sealloc_log("internal_allocator.internal_free: root == NULL");
  }
  ptr_cur = (ptrdiff_t)&root->memory;
  ia_node_t state = get_tree_item(root->buddy_tree, idx);
  // First, we have to find the right node
  // TODO: check if we are at the leaf node
  while (!(ptr_cur == ptr_dest && state == NODE_USED)) {
    cur_size /= 2;
    // Go to right child
    if (ptr_dest >= ptr_cur + cur_size) {
      idx = idx * 2 + 1;
      ptr_cur += cur_size;
    }
    // Go to left child
    else {
      idx = idx * 2;
    }
    state = get_tree_item(root->buddy_tree, idx);
  }
  set_tree_item(root->buddy_tree, idx, NODE_FREE);
  // Second phase, go up and coalesce free nodes
  while (idx != 1) {
    neigh_idx = (idx & 1) ? idx - 1 : idx + 1;
    state = get_tree_item(root->buddy_tree, neigh_idx);
    if (state == NODE_FREE) {
      set_tree_item(root->buddy_tree, idx / 2, NODE_FREE);
    }
    idx /= 2;
  }
}
