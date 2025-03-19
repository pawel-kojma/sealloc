#include <sealloc/chunk.h>
#include <sealloc/logging.h>
#include <sealloc/utils.h>
#include <string.h>

#define RIGHT_CHILD(idx) (idx * 2 + 1)
#define LEFT_CHILD(idx) (idx * 2)
#define PARENT(idx) (idx / 2)
#define IS_ROOT(idx) (idx != 1)
#define IS_LEAF(idx) (idx >= ((CHUNK_NO_NODES + 1) / 2))
#define IS_RIGHT_CHILD(idx) (idx & 1)

typedef enum chunk_node_type {
  NODE_FULL = 0,
  NODE_SPLIT = 1,
  NODE_USED_SET_GUARD = 2,
  NODE_USED_FOUND_GUARD = 3,
  NODE_FREE = 4,
  NODE_GUARD = 5,
  NODE_DEPLETED = 6,
  NODE_UNMAPPED = 7
} chunk_node_t;

typedef enum tree_traverse_state {
  DOWN,
  UP_LEFT,
  UP_RIGHT,
} tstate_t;

// Get tree node type at given index
static uint8_t get_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (mem[word] >> (2 * off)) & 3;
}

// Set tree node type to given type at index
static void set_tree_item(uint8_t *mem, size_t idx, chunk_node_t node_state) {
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
  chunk_node_t state;
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
// full
static void coalesce_full_nodes(uint8_t *mem, size_t idx) {
  size_t neigh_idx;
  chunk_node_t state;
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

// Coalesce nodes up the tree during allocation to indicate that this branch is
// free, change full node to split ones
static void coalesce_free_nodes(uint8_t *mem, size_t idx) {
  size_t neigh_idx;
  chunk_node_t state;
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

void chunk_init(chunk_t *chunk, void *heap) {
  chunk->entry.key = heap;
  memset(chunk->reg_size_small_medium, 0, sizeof(chunk->reg_size_small_medium));
  chunk->buddy_tree[0] = (uint8_t)NODE_FREE;
  chunk->free_mem = CHUNK_SIZE_BYTES;
}

void *chunk_allocate_run(chunk_t *chunk, size_t run_size, size_t reg_size) {
  size_t idx, cur_size, actual_size;
  uintptr_t ptr;
  chunk_node_t node;
  tstate_t state;
  idx = 1;
  state = DOWN;
  cur_size = CHUNK_SIZE_BYTES;
  ptr = (uintptr_t)&chunk->entry.key;
  actual_size = run_size + CHUNK_LEAST_REGION_SIZE_BYTES;
  if (chunk->free_mem < actual_size) return NULL;

  // Search stops when we come back to the root from the right child
  while (!(idx == 1 && state == UP_RIGHT)) {
    // If we are in a leaf node, visit and go up.
    if (IS_LEAF(idx)) {
      node = get_tree_item(chunk->buddy_tree, idx);
      switch (node) {
        case NODE_FREE:
          // There is no further splitting, allocate here.
          set_tree_item(chunk->buddy_tree, idx, NODE_USED);
          chunk->free_mem -= cur_size;
          coalesce_full_nodes(chunk->buddy_tree, idx);
          return (void *)ptr;
          break;
        case NODE_GUARD:
        case NODE_USED:
          // Node is used/guarded, nothing we can do, go up.
          state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
          ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
          idx = PARENT(idx);
          cur_size = cur_size * 2;
          break;
        case NODE_SPLIT:
          // Invalid state, leaf node cannot be split.
          se_error("Leaf node state is NODE_SPLIT");
      }
      continue;
    }
    switch (state) {
      case DOWN:
        node = get_tree_item(chunk->buddy_tree, idx);
        switch (node) {
          case NODE_SPLIT:
            // If this is the deepest node that can satisfy request
            // but is split, then backtrack
            if (cur_size / 2 < actual_size && actual_size <= cur_size) {
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
            if (cur_size / 2 < actual_size && actual_size <= cur_size) {
              set_tree_item(chunk->buddy_tree, idx, NODE_SPLIT);
              set_tree_item(chunk->buddy_tree, LEFT_CHILD(idx), NODE_USED);
              guard_leftmost_tree_item(chunk->buddy_tree, RIGHT_CHILD(idx));
              chunk->free_mem -=
                  ((cur_size / 2) + CHUNK_LEAST_REGION_SIZE_BYTES);
              return (void *)ptr;
            }
            // We know node above us must have been split.
            // In this case: size <= cur_size / 2
            // Split the node and go to the left child.
            else {
              set_tree_item(chunk->buddy_tree, idx, NODE_SPLIT);
              set_tree_item(chunk->buddy_tree, LEFT_CHILD(idx), NODE_FREE);
              set_tree_item(chunk->buddy_tree, RIGHT_CHILD(idx), NODE_FREE);
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
            // but there no free memory, so go back
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
