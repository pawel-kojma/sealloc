#include "sealloc/internal_allocator.h"
#include "sealloc/logging.h"
#include "sealloc/platform_api.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define RIGHT_CHILD(idx) (idx * 2 + 1)
#define LEFT_CHILD(idx) (idx * 2)
#define PARENT(idx) (idx / 2)
#define IS_ROOT(idx) (idx != 1)
#define IS_LEAF(idx) (idx >= ((INTERNAL_ALLOC_NO_NODES + 1) / 2))
#define IS_RIGHT_CHILD(idx) (idx & 1)

static struct internal_allocator_data *internal_alloc_mappings_root;

typedef enum internal_allocator_node_type {
  NODE_FULL = 0,
  NODE_SPLIT = 1,
  NODE_USED = 2,
  NODE_FREE = 3,
} ia_node_t;

typedef enum tree_traverse_state {
  DOWN,
  UP_LEFT,
  UP_RIGHT,
} tstate_t;

// Get a fresh mapping from kernel for further allocations
// Link it to the beginning of the list
static int morecore(void) {
  struct internal_allocator_data *map;
  platform_status_code_t code =
      platform_map(NULL, sizeof(struct internal_allocator_data), (void **)&map);
  if (code != PLATFORM_STATUS_OK) {
    se_error("Failed to allocate mapping for internal allocator: %s.",
             platform_strerror(code));
  }

  se_debug("map=%p", (void *)map);
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
  platform_status_code_t code;
  if ((code = platform_map(NULL, sizeof(struct internal_allocator_data),
                           (void **)&internal_alloc_mappings_root)) !=
      PLATFORM_STATUS_OK) {
    se_error("Failed to allocate mapping for internal allocator: %s.", code);
  }
  internal_alloc_mappings_root->bk = NULL;
  internal_alloc_mappings_root->fd = NULL;
  internal_alloc_mappings_root->buddy_tree[0] = (uint8_t)NODE_FREE;
  internal_alloc_mappings_root->free_mem =
      sizeof(internal_alloc_mappings_root->memory);
  return 0;
}

// Get tree node type at given index
static ia_node_t get_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (ia_node_t)(mem[word] >> (2 * off)) & 3;
}

// Set tree node type to given type at index
static void set_tree_item(uint8_t *mem, size_t idx, ia_node_t node_state) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  uint8_t erase_bits = 3;  // 0b11
  // First, clear the bits we want to set in mem[word]
  // Second, set two node_state bits in their place
  mem[word] = (mem[word] & ~(erase_bits << (off * 2))) |
              ((uint8_t)node_state << (off * 2));
}

// Mark nodes as split up the tree when coalescing free nodes ends
static void mark_nodes_split(uint8_t *mem, size_t idx) {
  ia_node_t state;
  // Here we want to also change the root to split
  while (idx > 0) {
    state = get_tree_item(mem, idx);
    if (state == NODE_FULL) {
      set_tree_item(mem, idx, NODE_SPLIT);
    } else
      break;
    idx /= 2;
  }
}

// Coalesce nodes up the tree during allocation to indicate that this branch is
// free, change full node to split ones
static void coalesce_free_nodes(uint8_t *mem, size_t idx) {
  size_t neigh_idx;
  ia_node_t state;
  while (idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(idx) ? idx - 1 : idx + 1;
    state = get_tree_item(mem, neigh_idx);
    if (state == NODE_FREE) {
      set_tree_item(mem, idx / 2, NODE_FREE);
    } else {
      mark_nodes_split(mem, idx / 2);
      break;
    }
    idx /= 2;
  }
}

// Coalesce nodes up the tree during allocation to indicate that this branch is
// full
static void coalesce_full_nodes(uint8_t *mem, size_t idx) {
  size_t neigh_idx;
  ia_node_t state;
  while (idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(idx) ? idx - 1 : idx + 1;
    state = get_tree_item(mem, neigh_idx);
    if (state == NODE_FULL || state == NODE_USED) {
      set_tree_item(mem, idx / 2, NODE_FULL);
    } else
      break;
    idx /= 2;
  }
}

// Try to allocate with given root
void *internal_alloc_with_root(struct internal_allocator_data *root,
                               size_t size) {
  size_t idx, cur_size;
  uintptr_t ptr;
  ia_node_t node;
  tstate_t state;
  idx = 1;
  state = DOWN;
  cur_size = sizeof(root->memory);
  ptr = (uintptr_t)&root->memory;
  if (root->free_mem < size) return NULL;

  // Search stops when we come back to the root from the right child
  while (!(idx == 1 && state == UP_RIGHT)) {
    // If we are in a leaf node, visit and go up.
    if (IS_LEAF(idx)) {
      node = get_tree_item(root->buddy_tree, idx);
      switch (node) {
        case NODE_FREE:
          se_debug("LeafNode(idx=%zu, state=NODE_FREE)", idx);
          // There is no further splitting, allocate here.
          set_tree_item(root->buddy_tree, idx, NODE_USED);
          root->free_mem -= cur_size;
          coalesce_full_nodes(root->buddy_tree, idx);
          return (void *)ptr;
          break;
        case NODE_USED:
          se_debug("LeafNode(idx=%zu, state=NODE_USED)", idx);
          // Node is used, nothing we can do, go up.
          state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
          ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
          idx = PARENT(idx);
          cur_size = cur_size * 2;
          break;
        case NODE_SPLIT:
          // Invalid state, leaf node cannot be split.
          se_error("Leaf node state is NODE_SPLIT");
        case NODE_FULL:
          // Full state, should never happen.
          se_error("Leaf node state is NODE_FULL");
      }
      continue;
    }
    switch (state) {
      case DOWN:
        node = get_tree_item(root->buddy_tree, idx);
        switch (node) {
          case NODE_SPLIT:
            // If this is the deepest node that can satisfy request
            // but is split, so backtrack
            if (cur_size / 2 < size && size <= cur_size) {
              state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
              ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
              idx = PARENT(idx);
              cur_size = cur_size * 2;
            }
            // If size still fits, but node is split, then go to left child.
            else {
              idx = LEFT_CHILD(idx);
              cur_size = cur_size / 2;
            }
            break;
          case NODE_FREE:
            // If size fits current node and the node is free,
            // then allocate it.
            if (cur_size / 2 < size && size <= cur_size) {
              set_tree_item(root->buddy_tree, idx, NODE_USED);
              root->free_mem -= cur_size;
              coalesce_full_nodes(root->buddy_tree, idx);
              return (void *)ptr;
            }
            // We know node above us must have been split.
            // In this case: size <= cur_size / 2
            // Split the node and go to the left child.
            else {
              set_tree_item(root->buddy_tree, idx, NODE_SPLIT);
              set_tree_item(root->buddy_tree, LEFT_CHILD(idx), NODE_FREE);
              set_tree_item(root->buddy_tree, RIGHT_CHILD(idx), NODE_FREE);
              cur_size = cur_size / 2;
              idx = LEFT_CHILD(idx);
            }
            break;
          case NODE_USED:
            // Node is used, nothing we can do, go up.
            state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
            ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
            idx = PARENT(idx);
            cur_size = cur_size * 2;
            break;
          case NODE_FULL:
            // Node is full, meaning it comprises of many allocations
            // but there no free memory, bo back
            state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
            ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
            idx = PARENT(idx);
            cur_size = cur_size * 2;
            break;
        }
        break;
      case UP_LEFT:
        cur_size = cur_size / 2;
        ptr = ptr + cur_size;
        state = DOWN;
        idx = RIGHT_CHILD(idx);
        break;
      case UP_RIGHT:
        state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
        ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
        idx = PARENT(idx);
        cur_size = cur_size * 2;
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
    se_debug("Trying to allocate with root = %p", (void *)root);
    alloc = internal_alloc_with_root(root, size);
    if (alloc != NULL) return alloc;
  }

  // No mapping can satisfy the request, try to get more memory
  se_debug("Trying to allocate more memory with memcore()");
  int res = morecore();

  // Cannot get more memory
  if (res < 0) {
    se_debug("No more memory");
    return NULL;
  }

  // Allocate with fresh memory mapping
  se_debug("Allocating from fresh mapping");
  return internal_alloc_with_root(internal_alloc_mappings_root, size);
}

// Free the chunk pointed by ptr
void internal_free(void *ptr) {
  size_t idx = 1, cur_size = INTERNAL_ALLOC_CHUNK_SIZE_BYTES;
  uintptr_t ptr_cur, ptr_dest = (uintptr_t)ptr;
  ia_node_t state;
  struct internal_allocator_data *root = NULL;

  // Find mapping that contains the ptr
  for (root = internal_alloc_mappings_root; root != NULL; root = root->fd) {
    if ((uintptr_t)&root->memory <= ptr_dest &&
        ptr_dest <= (uintptr_t)root->memory + sizeof(root->memory))
      break;
  }

  // No mapping found, panic
  if (!root) {
    se_error("root == NULL");
  }

  ptr_cur = (uintptr_t)&root->memory;
  state = get_tree_item(root->buddy_tree, idx);

  // First, we have to find the right node
  while (!(ptr_cur == ptr_dest && state == NODE_USED)) {
    /*
     * We are in a leaf node
     * and still haven't found the chunk.
     */
    if (IS_LEAF(idx)) {
      se_error("No chunk found");
    }
    cur_size /= 2;
    // Go to right child
    if (ptr_dest >= ptr_cur + cur_size) {
      idx = RIGHT_CHILD(idx);
      ptr_cur += cur_size;
    }
    // Go to left child
    else {
      idx = LEFT_CHILD(idx);
    }
    state = get_tree_item(root->buddy_tree, idx);
  }

  // Mark the node free
  set_tree_item(root->buddy_tree, idx, NODE_FREE);

  // Update free_mem field
  root->free_mem += cur_size;

  // Second phase, go up and coalesce free nodes
  coalesce_free_nodes(root->buddy_tree, idx);
}
