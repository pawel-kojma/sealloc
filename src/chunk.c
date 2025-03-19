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

// Mark nodes as split up the tree when setting guard page on allocation
static void mark_nodes_split_guard(uint8_t *mem, size_t idx) {
  chunk_node_t state;
  // Here we want to also change the root to split
  while (idx > 0) {
    state = get_tree_item(mem, idx);
    if (state == NODE_SPLIT) break;
    set_tree_item(mem, idx, NODE_SPLIT);
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
    if (state == NODE_FULL || state == NODE_USED_FOUND_GUARD ||
        state == NODE_USED_SET_GUARD) {
      set_tree_item(mem, idx / 2, NODE_FULL);
    } else
      break;
    idx /= 2;
  }
}

static inline size_t get_rightmost_idx(size_t idx, size_t depth) {
  return (idx << depth) + (1 << depth) - 1;
}

static inline size_t get_leftmost_idx(size_t idx, size_t depth) {
  return (idx << depth);
}

// Set guard protections on memory range that the node at the given index spans
void guard_tree_item(void *heap, size_t idx) {}

void chunk_init(chunk_t *chunk, void *heap) {
  chunk->entry.key = heap;
  memset(chunk->reg_size_small_medium, 0xff,
         sizeof(chunk->reg_size_small_medium));
  chunk->buddy_tree[0] = (uint8_t)NODE_FREE;
  chunk->free_mem = CHUNK_SIZE_BYTES;
}

void *chunk_allocate_run(chunk_t *chunk, size_t run_size, size_t reg_size) {
  size_t idx, cur_size, depth, neigh_idx;
  uintptr_t ptr;
  chunk_node_t node, neigh;
  tstate_t state;
  idx = 1;
  state = DOWN;
  depth = CHUNK_BUDDY_TREE_DEPTH;
  cur_size = CHUNK_SIZE_BYTES;
  ptr = (uintptr_t)&chunk->entry.key;
  if (chunk->free_mem < run_size) return NULL;

  // Search stops when we come back to the root from the right child
  while (!(idx == 1 && state == UP_RIGHT)) {
    // If we are in a leaf node, visit and go up if needed.
    if (IS_LEAF(idx)) {
      node = get_tree_item(chunk->buddy_tree, idx);
      switch (node) {
        case NODE_FREE:
          // Check if we can place a guard page.
          if (idx + 1 > CHUNK_NO_NODES)
            neigh = NODE_GUARD;
          else
            neigh = get_tree_item(chunk->buddy_tree, idx + 1);
          if (neigh != NODE_USED_FOUND_GUARD && neigh != NODE_USED_SET_GUARD) {
            if (neigh == NODE_FREE) {
              set_tree_item(chunk->buddy_tree, idx, NODE_USED_SET_GUARD);
              guard_tree_item(chunk->entry.key, idx + 1);
              mark_nodes_split_guard(chunk->buddy_tree, idx + 1);
            } else {
              set_tree_item(chunk->buddy_tree, idx, NODE_USED_FOUND_GUARD);
            }
            chunk->free_mem -= cur_size;
            coalesce_full_nodes(chunk->buddy_tree, idx);
            return (void *)ptr;
          }
          // fallthrough if neighbor node is not free or guarded.
        case NODE_GUARD:
        case NODE_USED_FOUND_GUARD:
        case NODE_USED_SET_GUARD:
        case NODE_DEPLETED:
          // Node is used/guarded, nothing we can do, go up.
          state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
          ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
          idx = PARENT(idx);
          cur_size = cur_size * 2;
          depth++;
          break;
        // Invalid states, leaf node cannot be split/unmapped/full.
        case NODE_SPLIT:
          se_error("Leaf node state is NODE_SPLIT");
        case NODE_FULL:
          se_error("Leaf node state is NODE_FULL");
        case NODE_UNMAPPED:
          se_error("Leaf node state is NODE_UNMAPPED");
      }
      continue;
    }
    switch (state) {
      case DOWN:
        node = get_tree_item(chunk->buddy_tree, idx);
        switch (node) {
          case NODE_SPLIT:
            // If this is the deepest node that can satisfy request
            // but is split, so backtrack
            if (cur_size / 2 < run_size && run_size <= cur_size) {
              state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
              ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
              idx = PARENT(idx);
              cur_size = cur_size * 2;
              depth++;
            }
            // If size still fits, but node is split, then go to left child.
            else {
              idx = LEFT_CHILD(idx);
              cur_size = cur_size / 2;
              depth--;
            }
            break;
          case NODE_FREE:
            neigh_idx = get_rightmost_idx(idx, depth) + 1;
            // Let arena handle guard pages between chunks
            if (neigh_idx > CHUNK_NO_NODES)
              neigh = NODE_GUARD;
            else
              neigh = get_tree_item(chunk->buddy_tree, neigh_idx);
            // If run size still overfits, search deeper
            if (run_size <= cur_size / 2) {
              set_tree_item(chunk->buddy_tree, idx, NODE_SPLIT);
              set_tree_item(chunk->buddy_tree, LEFT_CHILD(idx), NODE_FREE);
              set_tree_item(chunk->buddy_tree, RIGHT_CHILD(idx), NODE_FREE);
              cur_size = cur_size / 2;
              idx = LEFT_CHILD(idx);
              depth--;
            }
            // We cannot go deeper because cur_size / 2 < run_size
            // If anything we have to allocate here
            // Make sure neighbor node is guarded or free
            else if (neigh != NODE_USED_FOUND_GUARD &&
                     neigh != NODE_USED_SET_GUARD) {
              if (neigh == NODE_FREE) {
                set_tree_item(chunk->buddy_tree, idx, NODE_USED_SET_GUARD);
                set_tree_item(chunk->buddy_tree, get_leftmost_idx(idx, depth),
                              NODE_USED_SET_GUARD);
                guard_tree_item(chunk->entry.key, idx + 1);
                mark_nodes_split_guard(chunk->buddy_tree, idx + 1);
              } else {
                set_tree_item(chunk->buddy_tree, idx, NODE_USED_FOUND_GUARD);
                set_tree_item(chunk->buddy_tree, get_leftmost_idx(idx, depth),
                              NODE_USED_FOUND_GUARD);
              }
              chunk->free_mem -= cur_size;
              coalesce_full_nodes(chunk->buddy_tree, idx);
              return (void *)ptr;
            } else {
              // We cannot guard a neighbor node and deeper nodes are too small
              // Go up
              state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
              ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
              idx = PARENT(idx);
              cur_size = cur_size * 2;
              depth++;
            }
            break;
          // Node is used, nothing we can do, go up.
          case NODE_USED_FOUND_GUARD:
          case NODE_USED_SET_GUARD:
          // Node is full, meaning it comprises of many allocations
          // but there is no free memory, so go back
          case NODE_FULL:
          // Unavaliable memory, either guarded, already used or unmapped
          case NODE_DEPLETED:
          case NODE_GUARD:
          case NODE_UNMAPPED:
            state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
            ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
            idx = PARENT(idx);
            cur_size = cur_size * 2;
            depth++;
            break;
        }
        break;
      case UP_LEFT:
        cur_size = cur_size / 2;
        ptr = ptr + cur_size;
        state = DOWN;
        idx = RIGHT_CHILD(idx);
        depth--;
        break;
      case UP_RIGHT:
        state = IS_RIGHT_CHILD(idx) ? UP_RIGHT : UP_LEFT;
        ptr = IS_RIGHT_CHILD(idx) ? ptr - cur_size : ptr;
        idx = PARENT(idx);
        cur_size = cur_size * 2;
        depth++;
        break;
    }
  }
  return NULL;
}
