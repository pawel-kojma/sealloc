#include "sealloc/chunk.h"

#include <assert.h>
#include <string.h>

#include "sealloc/logging.h"
#include "sealloc/platform_api.h"
#include "sealloc/random.h"
#include "sealloc/size_class.h"
#include "sealloc/utils.h"

#define RIGHT_CHILD(idx) (((idx) * 2) + 1)
#define LEFT_CHILD(idx) ((idx) * 2)
#define PARENT(idx) ((idx) / 2)
#define IS_ROOT(idx) ((idx) == 1)
#define IS_LEAF(idx) ((idx) >= ((CHUNK_NO_NODES + 1) / 2))
#define IS_RIGHT_CHILD(idx) ((idx) & 1)

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_USED = 1,
  NODE_DEPLETED = 2,
  NODE_UNMAPPED = 3,
} chunk_node_t;

typedef enum search_state {
  DOWN,
  UP_LEFT,
  UP_RIGHT,
} search_state_t;

typedef struct buddy_state {
  unsigned idx;            // Current node index
  unsigned cur_size;       // Chunk size spanned by current node
  unsigned depth_to_leaf;  // Number of levels left to leaf nodes
  uintptr_t ptr;           // Points to region of current node
  search_state_t state;    // Current search state
} buddy_ctx_t;

static inline unsigned get_mask(unsigned bits) { return (1 << bits) - 1; }

static inline unsigned min(unsigned a, unsigned b) { return a > b ? b : a; }

static inline void set_jt_item_next(jump_node_t *mem, unsigned idx,
                                    unsigned short val) {
  mem[idx - 1].next = val;
}
static inline void set_jt_item_prev(jump_node_t *mem, unsigned idx,
                                    unsigned short val) {
  mem[idx - 1].prev = val;
}
static inline void incr_jt_item_next(jump_node_t *mem, unsigned idx,
                                     unsigned short val) {
  mem[idx - 1].next += val;
}
static inline void incr_jt_item_prev(jump_node_t *mem, unsigned idx,
                                     unsigned short val) {
  mem[idx - 1].prev += val;
}
static inline jump_node_t get_jt_item(jump_node_t *mem, unsigned idx) {
  return mem[idx - 1];
}

// Get tree node type at given index
static inline chunk_node_t get_buddy_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (chunk_node_t)(mem[word] >> (2 * off)) & 3;
}

// Set tree node type to given type at index
static inline void set_buddy_tree_item(uint8_t *mem, size_t idx,
                                       chunk_node_t node_state) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  uint8_t erase_bits = 3;  // 0b11
  // First, clear the bits we want to set in mem[word]
  // Second, set two node_state bits in their place
  mem[word] = (mem[word] & ~(erase_bits << (off * 2))) |
              ((uint8_t)node_state << (off * 2));
}

static inline unsigned get_rightmost_idx(unsigned idx, unsigned depth) {
  return (idx << depth) + (1 << depth) - 1;
}

static inline unsigned get_leftmost_idx(unsigned idx, unsigned depth) {
  return (idx << depth);
}

void chunk_init(chunk_t *chunk, void *heap) {
  assert(heap != NULL);
  chunk->entry.key = heap;
  chunk->entry.link.fd = NULL;
  chunk->entry.link.bk = NULL;
  memset(chunk->reg_size_small_medium, REG_MARK_BAD_VALUE,
         sizeof(chunk->reg_size_small_medium));
  set_buddy_tree_item(chunk->buddy_tree, 1, NODE_FREE);
  memset(&chunk->buddy_tree, 0, sizeof(chunk->buddy_tree));
  for (int i = 0; i < CHUNK_NO_NODES; i++) {
    chunk->jump_tree[i].next = 1;
    chunk->jump_tree[i].prev = 1;
  }
  unsigned short l = 1;
  for (int i = 0; i < CHUNK_BUDDY_TREE_DEPTH + 1; i++) {
    chunk->jump_tree_first_index[i] = l;
    set_jt_item_prev(chunk->jump_tree, l, 0);
    set_jt_item_next(chunk->jump_tree, 2 * l - 1, 0);
    chunk->avail_nodes_count[i] = l;
    l *= 2;
  }
  chunk->avail_nodes_count[0] = 0;
  chunk->jump_tree_first_index[0] = 0;
}

void buddy_state_go_up(buddy_ctx_t *ctx) {
  ctx->state = IS_RIGHT_CHILD(ctx->idx) ? UP_RIGHT : UP_LEFT;
  ctx->ptr = IS_RIGHT_CHILD(ctx->idx) ? ctx->ptr - ctx->cur_size : ctx->ptr;
  ctx->idx = PARENT(ctx->idx);
  ctx->cur_size *= 2;
  ctx->depth_to_leaf++;
}

void buddy_state_go_right(buddy_ctx_t *ctx) {
  ctx->cur_size = ctx->cur_size / 2;
  ctx->ptr = ctx->ptr + ctx->cur_size;
  ctx->state = DOWN;
  ctx->idx = RIGHT_CHILD(ctx->idx);
  ctx->depth_to_leaf--;
}

void buddy_state_go_left(buddy_ctx_t *ctx) {
  ctx->idx = LEFT_CHILD(ctx->idx);
  ctx->cur_size = ctx->cur_size / 2;
  ctx->depth_to_leaf--;
}

unsigned size2idx(unsigned run_size) {
  return ctz(run_size / CHUNK_LEAST_REGION_SIZE_BYTES);
}

void *chunk_allocate_with_node(chunk_t *chunk, jump_node_t node,
                               const unsigned idx, const unsigned level,
                               const unsigned reg_size) {
  // State of current node being unlinked
  jump_node_t current_node = node;

  // Global index of the current node
  unsigned current_global_idx = idx;

  // Global index of first element on current level
  unsigned base_level_idx = 1 << level;

  // Global indexes of next and previous free nodes in current level
  unsigned next_node_global_idx, prev_node_global_idx;

  // 0-based level index
  unsigned current_level = level;

  // Update up the tree
  while ((current_node.next != 0 || current_node.prev != 0) ||
         (chunk->jump_tree_first_index[current_level] == current_global_idx)) {
    if (current_node.next == 0) {
      // Right side of the tree, just set prev to 0
      set_jt_item_next(chunk->jump_tree, current_global_idx - current_node.prev,
                       0);
    } else if (current_node.prev == 0) {
      // Left side of the tree
      // Set starting point to next
      // Set prev of next to 0 since none on the left are free
      chunk->jump_tree_first_index[current_level] =
          current_global_idx + current_node.next;
      set_jt_item_prev(chunk->jump_tree, current_global_idx + current_node.next,
                       0);
    } else {
      next_node_global_idx = current_global_idx + current_node.next;
      prev_node_global_idx = current_global_idx - current_node.prev;

      // Increment next field of previous free node to point to the next
      incr_jt_item_next(chunk->jump_tree, prev_node_global_idx,
                        current_node.next);
      // Increment prev field of next free node to point to the prev
      incr_jt_item_prev(chunk->jump_tree, next_node_global_idx,
                        current_node.prev);
    }
    // Clear pointers in current node
    set_jt_item_next(chunk->jump_tree, current_global_idx, 0);
    set_jt_item_prev(chunk->jump_tree, current_global_idx, 0);

    // Update available nodes count
    chunk->avail_nodes_count[current_level]--;
    if (chunk->avail_nodes_count[current_level] == 0)
      chunk->jump_tree_first_index[current_level] = 0;

    // Go up
    current_level--;
    current_global_idx = PARENT(current_global_idx);
    current_node = get_jt_item(chunk->jump_tree, current_global_idx);
  }

  // If idx is a leaf node then job is done
  if (IS_LEAF(idx)) {
    if (reg_size != CHUNK_LEAST_REGION_SIZE_BYTES)
      chunk->reg_size_small_medium[idx - base_level_idx] = reg_size;
    set_buddy_tree_item(chunk->buddy_tree, idx, NODE_USED);
    return (void *)((uintptr_t)chunk->entry.key +
                    (idx - base_level_idx) * CHUNK_LEAST_REGION_SIZE_BYTES);
  }

  // In this case, we are higher up the tree so we have to invalidate nodes
  // below us
  // 1. Set pointers in jump tree to skip all nodes in current subtree
  // 2. memset 0 entire subtree

  // Indexes of the leftmost and rightmost node on current level in the subtree
  unsigned left_idx = LEFT_CHILD(idx);
  unsigned right_idx = RIGHT_CHILD(idx);
  unsigned current_level_nodes_span = 2;
  current_level = level + 1;
  jump_node_t node_left, node_right;
  while (current_level <= CHUNK_BUDDY_TREE_DEPTH) {
    node_left = get_jt_item(chunk->jump_tree, left_idx);
    node_right = get_jt_item(chunk->jump_tree, right_idx);

    if (node_left.prev == 0 && node_right.next == 0) {
      // Entire level is cleared, invalidate start idx
      chunk->jump_tree_first_index[current_level] = 0;
    } else if (node_left.prev == 0) {
      // node_left was the first free node on this level
      // point first indxes to after right node
      // set prev of after right node to 0 since there are none free nodes on
      // the left
      chunk->jump_tree_first_index[current_level] = right_idx + node_right.next;
      set_jt_item_prev(chunk->jump_tree, right_idx + node_right.next, 0);
    } else if (node_right.next == 0) {
      // node_right was the last free node on this level
      // set next of before left node to 0
      set_jt_item_next(chunk->jump_tree, left_idx - node_left.prev, 0);
    } else {
      incr_jt_item_next(chunk->jump_tree, left_idx - node_left.prev,
                        (current_level_nodes_span - 1) + node_right.next);
      incr_jt_item_prev(chunk->jump_tree, right_idx + node_right.next,
                        (current_level_nodes_span - 1) + node_left.prev);
    }

    // Update available nodes
    chunk->avail_nodes_count[current_level] -= current_level_nodes_span;

    left_idx = LEFT_CHILD(left_idx);
    right_idx = RIGHT_CHILD(right_idx);
    current_level++;
    current_level_nodes_span *= 2;
  }

  // Purging subtree
  unsigned start_clear_idx = LEFT_CHILD(idx);
  current_level_nodes_span = 2;
  // memset 0 entire subtree
  for (current_level = level + 1; current_level <= CHUNK_BUDDY_TREE_DEPTH;
       current_level++) {
    memset((void *)&chunk->jump_tree[start_clear_idx - 1], 0,
           current_level_nodes_span * sizeof(jump_node_t));
    current_level_nodes_span *= 2;
    start_clear_idx = LEFT_CHILD(start_clear_idx);
  }

  // Update buddy and return ptr
  set_buddy_tree_item(chunk->buddy_tree, idx, NODE_USED);
  unsigned offset = get_leftmost_idx(idx, CHUNK_BUDDY_TREE_DEPTH - level) -
                    ((CHUNK_NO_NODES + 1) / 2);
  return (void *)((uintptr_t)chunk->entry.key +
                  offset * CHUNK_LEAST_REGION_SIZE_BYTES);
}

void *chunk_allocate_run(chunk_t *chunk, unsigned run_size, unsigned reg_size) {
  assert(CHUNK_LEAST_REGION_SIZE_BYTES <= run_size);
  assert(run_size <= CHUNK_SIZE_BYTES);
  assert(is_size_aligned(reg_size));

  // Check if we can allocate in this chunk
  const unsigned level = CHUNK_BUDDY_TREE_DEPTH - size2idx(run_size);
  const unsigned avail_nodes = chunk->avail_nodes_count[level];
  const unsigned all_nodes = 1 << level;
  const unsigned level_base_idx = all_nodes;
  jump_node_t node;
  if (avail_nodes == 0) return NULL;

  unsigned rand_idx, current_idx;
  // Try to hit a free node
  if (((avail_nodes * 100) / all_nodes) > RANDOM_LOOKUP_TRESHOLD_PERCENTAGE) {
    for (uint8_t i = 0; i < RANDOM_LOOKUP_TRIES; i++) {
      rand_idx = splitmix32() % all_nodes;
      node = get_jt_item(chunk->jump_tree, level_base_idx + rand_idx);
      if (node.prev != 0 || node.next != 0) {
        return chunk_allocate_with_node(chunk, node, level_base_idx + rand_idx,
                                        level, reg_size);
      }
    }
  }
  // Get random node index
  rand_idx = splitmix32() % avail_nodes;
  current_idx = chunk->jump_tree_first_index[level];
  node = get_jt_item(chunk->jump_tree, current_idx);
  while (rand_idx > 0) {
    rand_idx--;
    current_idx += node.next;
    node = get_jt_item(chunk->jump_tree, current_idx);
  }
  return chunk_allocate_with_node(chunk, node, current_idx, level, reg_size);
}

// Merge unmapped nodes to indicate that the pages corresponding to those nodes
// were unmapped
static void coalesce_unmapped_nodes(buddy_ctx_t *ctx, chunk_t *chunk) {
  unsigned neigh_idx;
  chunk_node_t node;
  while (ctx->idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(ctx->idx) ? ctx->idx - 1 : ctx->idx + 1;
    node = get_buddy_tree_item(chunk->buddy_tree, neigh_idx);
    if (node == NODE_UNMAPPED) {
      set_buddy_tree_item(chunk->buddy_tree, PARENT(ctx->idx), NODE_UNMAPPED);
    } else
      break;
    buddy_state_go_up(ctx);
  }
}

static void coalesce_depleted_nodes(buddy_ctx_t *ctx, chunk_t *chunk) {
  unsigned neigh_idx;
  chunk_node_t node;
  platform_status_code_t code;
  while (ctx->idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(ctx->idx) ? ctx->idx - 1 : ctx->idx + 1;
    node = get_buddy_tree_item(chunk->buddy_tree, neigh_idx);
    if (node == NODE_DEPLETED) {
      set_buddy_tree_item(chunk->buddy_tree, PARENT(ctx->idx), NODE_DEPLETED);
    } else
      break;
    buddy_state_go_up(ctx);
  }

  // We've merged as much depleted nodes as possible
  // Check if depth passed the unmap threshold
  if (ctx->depth_to_leaf >= CHUNK_UNMAP_THRESHOLD) {
    // We passed the threshold, unmap
    if ((code = platform_unmap((void *)ctx->ptr, ctx->cur_size)) !=
        PLATFORM_STATUS_OK) {
      se_error("Failed unmap page (ptr : %p, size : %u): %s.", (void *)ctx->ptr,
               ctx->cur_size, platform_strerror(code));
    }
    set_buddy_tree_item(chunk->buddy_tree, ctx->idx, NODE_UNMAPPED);
    coalesce_unmapped_nodes(ctx, chunk);
  }
}

bool chunk_deallocate_run(chunk_t *chunk, void *run_ptr) {
  uintptr_t ptr_dest = (uintptr_t)run_ptr;
  chunk_node_t node;
  unsigned first_leaf_idx = (CHUNK_NO_NODES + 1) / 2;
  unsigned ptr_offset = ((ptr_dest - (uintptr_t)chunk->entry.key) /
                         CHUNK_LEAST_REGION_SIZE_BYTES);
  unsigned idx = first_leaf_idx + ptr_offset;
  unsigned depth_to_leaf = 0;
  node = get_buddy_tree_item(chunk->buddy_tree, idx);
  while (node != NODE_USED) {
    if (IS_ROOT(idx) || IS_RIGHT_CHILD(idx)) {
      se_error("No run found");
    }
    if (node == NODE_DEPLETED) {
      se_error("run is depleted?");
    }
    idx = PARENT(idx);
    depth_to_leaf++;
    node = get_buddy_tree_item(chunk->buddy_tree, idx);
  }
  set_buddy_tree_item(chunk->buddy_tree, idx, NODE_DEPLETED);
  buddy_ctx_t ctx = {.idx = idx,
                     .cur_size = CHUNK_LEAST_REGION_SIZE_BYTES << depth_to_leaf,
                     .depth_to_leaf = depth_to_leaf,
                     .ptr = ptr_dest,
                     .state = DOWN};
  coalesce_depleted_nodes(&ctx, chunk);
  if (ctx.idx == 1) return true;
  return false;
}

// Fills run data based of ptr
void chunk_get_run_ptr(chunk_t *chunk, void *ptr, void **run_ptr,
                       unsigned *run_size, unsigned *reg_size) {
  assert(*run_ptr == NULL);
  assert(*run_size == 0);
  assert(*reg_size == 0);
  const uintptr_t chunk_ptr = (uintptr_t)chunk->entry.key;
  if ((uintptr_t)ptr < chunk_ptr ||
      chunk_ptr + CHUNK_SIZE_BYTES <= (uintptr_t)ptr) {
    return;
  }
  const uintptr_t rel_ptr = (uintptr_t)ptr - chunk_ptr;
  const uintptr_t block_offset = rel_ptr / CHUNK_LEAST_REGION_SIZE_BYTES;
  const unsigned first_leaf_idx = (CHUNK_NO_NODES + 1) / 2;
  uintptr_t target_run_ptr =
      chunk_ptr + (block_offset * CHUNK_LEAST_REGION_SIZE_BYTES);

  chunk_node_t node;
  unsigned cur_size = CHUNK_LEAST_REGION_SIZE_BYTES;
  unsigned idx = first_leaf_idx + block_offset;

  node = get_buddy_tree_item(chunk->buddy_tree, idx);

  // This case is not that common
  if (node != NODE_USED) {
    while (node != NODE_USED) {
      if (IS_ROOT(idx) || IS_RIGHT_CHILD(idx)) {
        return;
      }
      if (node == NODE_DEPLETED) {
        // This may happen when we have chunk-in-chunk or huge-in-chunk case
        // We have to return and check rest of chunks/huge allocs
        return;
      }
      idx = PARENT(idx);
      cur_size *= 2;
      node = get_buddy_tree_item(chunk->buddy_tree, idx);
    }

    *run_ptr = (void *)target_run_ptr;
    *run_size = cur_size;
    return;
  }
  *run_ptr = (void *)target_run_ptr;
  *run_size = cur_size;

  if (chunk->reg_size_small_medium[block_offset] != REG_MARK_BAD_VALUE) {
    *reg_size = chunk->reg_size_small_medium[block_offset];
  }
}

bool chunk_is_unmapped(chunk_t *chunk) {
  return get_buddy_tree_item(chunk->buddy_tree, 1) == NODE_UNMAPPED;
}
bool chunk_is_full(chunk_t *chunk) {
  return chunk->avail_nodes_count[CHUNK_BUDDY_TREE_DEPTH] == 0;
}
