#include "sealloc/chunk.h"

#include <assert.h>
#include <string.h>

#include "sealloc/logging.h"
#include "sealloc/platform_api.h"
#include "sealloc/size_class.h"

#define RIGHT_CHILD(idx) (idx * 2 + 1)
#define LEFT_CHILD(idx) (idx * 2)
#define PARENT(idx) (idx / 2)
#define IS_ROOT(idx) (idx != 1)
#define IS_LEAF(idx) (idx >= ((CHUNK_NO_NODES + 1) / 2))
#define IS_RIGHT_CHILD(idx) (idx & 1)

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_USED = 1,
  NODE_DEPLETED = 2,
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

typedef struct jump_node {
  unsigned short prev : 9;
  unsigned short next : 9;
} jump_node_t;

#define JUMP_NODE_EMPTY ((jump_node_t)0)

static inline unsigned get_mask(unsigned bits) { return (1 << bits) - 1; }

static inline unsigned min(unsigned a, unsigned b) { return a > b ? b : a; }

static void set_jump_tree_item_next(uint8_t *mem, unsigned idx,
                                    unsigned short val);
static void set_jump_tree_item_prev(uint8_t *mem, unsigned idx,
                                    unsigned short val);
static jump_node_t get_jump_tree_item(uint8_t *mem, unsigned idx);

// Get tree node type at given index
static chunk_node_t get_buddy_tree_item(uint8_t *mem, size_t idx) {
  size_t word = (idx - 1) / 4;
  size_t off = (idx - 1) % 4;
  return (chunk_node_t)(mem[word] >> (2 * off)) & 3;
}

// Set tree node type to given type at index
static void set_buddy_tree_item(uint8_t *mem, size_t idx,
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
  memset(&chunk->jump_tree, 1, sizeof(chunk->jump_tree));
  unsigned short l = 1, r = 2;
  for (int i = 0; i < CHUNK_BUDDY_TREE_DEPTH + 1; i++) {
    chunk->jump_tree_first_index[i] = 1;
    set_jump_tree_item_prev(chunk->jump_tree, l, 0);
    set_jump_tree_item_next(chunk->jump_tree, r - 1, 0);
    chunk->avail_nodes_count[i] = l;
    l *= 2;
    r *= 2;
  }
}

void inline buddy_state_go_up(buddy_ctx_t *ctx) {
  ctx->state = IS_RIGHT_CHILD(ctx->idx) ? UP_RIGHT : UP_LEFT;
  ctx->ptr = IS_RIGHT_CHILD(ctx->idx) ? ctx->ptr - ctx->cur_size : ctx->ptr;
  ctx->idx = PARENT(ctx->idx);
  ctx->cur_size *= 2;
  ctx->depth_to_leaf++;
}

void inline buddy_state_go_right(buddy_ctx_t *ctx) {
  ctx->cur_size = ctx->cur_size / 2;
  ctx->ptr = ctx->ptr + ctx->cur_size;
  ctx->state = DOWN;
  ctx->idx = RIGHT_CHILD(ctx->idx);
  ctx->depth_to_leaf--;
}

void inline buddy_state_go_left(buddy_ctx_t *ctx) {
  ctx->idx = LEFT_CHILD(ctx->idx);
  ctx->cur_size = ctx->cur_size / 2;
  ctx->depth_to_leaf--;
}

// Sets info about what region size is allocated at which run ptr
void mark_reg_size(buddy_ctx_t *ctx, chunk_t *chunk, unsigned reg_size) {
  if (!IS_SIZE_SMALL(reg_size) && !IS_SIZE_MEDIUM(reg_size)) return;
  unsigned base = (CHUNK_NO_NODES + 1) / 2;
  // We know that if reg_size is small or medium, then we allocated at the leaf
  // 0-based index
  unsigned idx = ctx->idx - base;
  // Note that reg_size 4096 will eval to 0
  chunk->reg_size_small_medium[idx] =
      (reg_size / SMALL_SIZE_CLASS_ALIGNMENT) & UINT8_MAX;
}

void *visit_leaf_node(buddy_ctx_t *ctx, chunk_t *chunk, unsigned reg_size) {
  unsigned neigh_idx = ctx->idx + 1;
  chunk_node_t neigh, node = get_buddy_tree_item(chunk->buddy_tree, ctx->idx);
  platform_status_code_t code;
  switch (node) {
    case NODE_FREE:
      // Check if we can place a guard page.
      if (neigh_idx > CHUNK_NO_NODES)
        neigh = NODE_GUARD;
      else
        neigh = get_tree_item(chunk->buddy_tree, neigh_idx);
      if (neigh != NODE_USED) {
        if (neigh == NODE_FREE) {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED);
          // Guard neighbor
          if ((code = platform_guard((void *)(ctx->ptr + ctx->cur_size),
                                     CHUNK_LEAST_REGION_SIZE_BYTES)) !=
              PLATFORM_STATUS_OK) {
            se_error("Failed to guard page (ptr : %p, size : %zu): %s",
                     (void *)(ctx->ptr + ctx->cur_size),
                     CHUNK_LEAST_REGION_SIZE_BYTES, platform_strerror(code));
          }
          set_tree_item(chunk->buddy_tree, neigh_idx, NODE_GUARD);
          mark_nodes_split_guard(chunk->buddy_tree, PARENT(neigh_idx));
          chunk->free_mem -= (ctx->cur_size + CHUNK_LEAST_REGION_SIZE_BYTES);
        } else {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED);
          chunk->free_mem -= ctx->cur_size;
        }
        mark_reg_size(ctx, chunk, reg_size);
        coalesce_full_nodes(chunk->buddy_tree, ctx->idx);
        return (void *)ctx->ptr;
      }
      buddy_state_go_up(ctx);
      break;
    case NODE_GUARD:
    case NODE_USED:
    case NODE_DEPLETED:
      // Node is used/guarded, nothing we can do, go up.
      buddy_state_go_up(ctx);
      break;
    // Invalid states, leaf node cannot be split/unmapped/full.
    case NODE_SPLIT:
      se_error("Leaf node state is NODE_SPLIT");
    case NODE_FULL:
      se_error("Leaf node state is NODE_FULL");
    case NODE_UNMAPPED:
      se_error("Leaf node state is NODE_UNMAPPED");
  }
  return NULL;
}

void *visit_regular_node(buddy_ctx_t *ctx, chunk_t *chunk, unsigned run_size) {
  unsigned neigh_idx = get_rightmost_idx(ctx->idx, ctx->depth_to_leaf) + 1;
  chunk_node_t neigh, node = get_buddy_tree_item(chunk->buddy_tree, ctx->idx);
  platform_status_code_t code;
  switch (node) {
    case NODE_SPLIT:
      // If this is the deepest node that can satisfy request
      // but is split, so backtrack
      if (ctx->cur_size / 2 < run_size && run_size <= ctx->cur_size)
        buddy_state_go_up(ctx);
      // If size still fits, but node is split, then go to left child.
      else
        buddy_state_go_left(ctx);
      break;
    case NODE_FREE:
      // Let arena handle guard pages between chunks
      if (neigh_idx > CHUNK_NO_NODES)
        neigh = NODE_GUARD;
      else
        neigh = get_buddy_tree_item(chunk->buddy_tree, neigh_idx);
      // If run size still overfits, search deeper
      if (run_size <= ctx->cur_size / 2) {
        set_tree_item(chunk->buddy_tree, ctx->idx, NODE_SPLIT);
        set_tree_item(chunk->buddy_tree, LEFT_CHILD(ctx->idx), NODE_FREE);
        set_tree_item(chunk->buddy_tree, RIGHT_CHILD(ctx->idx), NODE_FREE);
        buddy_state_go_left(ctx);
      }
      // We cannot go deeper because cur_size / 2 < run_size
      // If anything we have to allocate here
      // Make sure neighbor node is guarded or free
      else if (neigh != NODE_USED) {
        if (neigh == NODE_FREE) {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED);
          set_tree_item(chunk->buddy_tree,
                        get_leftmost_idx(ctx->idx, ctx->depth_to_leaf),
                        NODE_USED);
          // Guard neighbor
          if ((code = platform_guard((void *)(ctx->ptr + ctx->cur_size),
                                     CHUNK_LEAST_REGION_SIZE_BYTES)) !=
              PLATFORM_STATUS_OK) {
            se_error("Failed to guard page (ptr : %p, size : %zu): %s",
                     (void *)(ctx->ptr + ctx->cur_size),
                     CHUNK_LEAST_REGION_SIZE_BYTES, platform_strerror(code));
          }

          set_tree_item(chunk->buddy_tree, neigh_idx, NODE_GUARD);
          mark_nodes_split_guard(chunk->buddy_tree, PARENT(neigh_idx));
          chunk->free_mem -= (ctx->cur_size + CHUNK_LEAST_REGION_SIZE_BYTES);
        } else {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED);
          set_tree_item(chunk->buddy_tree,
                        get_leftmost_idx(ctx->idx, ctx->depth_to_leaf),
                        NODE_USED);
          chunk->free_mem -= ctx->cur_size;
        }
        coalesce_full_nodes(chunk->buddy_tree, ctx->idx);
        return (void *)ctx->ptr;
      } else {
        // We cannot guard a neighbor node and deeper nodes are too small
        // Go up
        buddy_state_go_up(ctx);
      }
      break;
    // Node is used, nothing we can do, go up.
    case NODE_USED:
    // Node is full, meaning it comprises of many allocations
    // but there is no free memory, so go back
    case NODE_FULL:
    // Unavaliable memory, either guarded, already used or unmapped
    case NODE_DEPLETED:
    case NODE_GUARD:
    case NODE_UNMAPPED:
      buddy_state_go_up(ctx);
      break;
  }
  return NULL;
}

unsigned inline size2idx(unsigned run_size) {
  return ctz(run_size / CHUNK_LEAST_REGION_SIZE_BYTES);
}

void *chunk_allocate_with_node(chunk_t *chunk, jump_node_t node, unsigned idx,
                               unsigned base_level_idx, unsigned level,
                               unsigned reg_size) {
  // State of current node being unlinked
  jump_node_t current_node = node;

  // Global index of the current node
  unsigned current_global_idx = idx;

  // Global index of the leftmost node in the level
  unsigned current_global_base_level_idx = base_level_idx;

  // Global index of the rightmost node in the level
  unsigned current_end_level_idx = 2 * base_level_idx - 1;

  // Global indexes of next and previous free nodes in current level
  unsigned next_node_global_idx, prev_node_global_idx;

  /*
   * Każdy node ma prev i next pointujące na nastepny i poprzedni wolny element
   *
   * node.prev - offset do poprzedniego wolnego względem siebie
   * node.next - offset do nastepnego wolnego względem siebie
   */

  while (current_node.next != 0 && current_node.prev != 0) {
    if (current_node.next == 0) {
      // Right side of the tree, just set prev to 0
      set_next_jt_item(chunk->jump_tree, current_global_idx - current_node.prev,
                       0);
    }
    if (current_node.prev == 0) {
      // Left side of the tree, set starting point to next
      chunk->jump_tree_first_index[level] = current_idx + current_node.next;
    } else {
      next_node_global_idx = current_global_idx + current_node.next;
      prev_node_global_idx = current_global_idx - current_node.prev;

      // Increment next field of previous free node to point to the next
      incr_next_jt_item(chunk->jump_tree, prev_node_global_idx,
                        current_node.next);
      // Increment prev field of next free node to point to the prev
      incr_prev_jt_item(chunk->jump_tree, next_node_global_idx,
                        current_node.prev);
    }
    // Clear pointers in current node
    set_next_jt_item(chunk->jump_tree, current_global_idx, 0);
    set_prev_jt_item(chunk->jump_tree, current_global_idx, 0);

    // Go up
    current_global_idx = PARENT(current_global_idx);
    current_node = get_jt_item(chunk->jump_tree, current_global_idx);
  }

  // If idx is a leaf node then job is done
  if (IS_LEAF(idx)) {
    chunk->reg_size_small_medium[idx - base_level_idx] =
        (reg_size / SMALL_SIZE_CLASS_ALIGNMENT) & UINT8_MAX;
    return (void *)(chunk->entry.key +
                    (idx - base_level_idx) * CHUNK_LEAST_REGION_SIZE_BYTES);
  }


}

void *chunk_allocate_run(chunk_t *chunk, unsigned run_size, unsigned reg_size) {
  assert(CHUNK_LEAST_REGION_SIZE_BYTES <= run_size);
  assert(run_size <= CHUNK_SIZE_BYTES);
  assert(is_size_aligned(reg_size));

  // Check if we can allocate in this chunk
  const unsigned idx = CHUNK_BUDDY_TREE_DEPTH - size2idx(run_size);
  const unsigned avail_nodes = chunk->avail_nodes_count[idx];
  const unsigned all_nodes = 1 << idx;
  const unsigned level_base_idx = all_nodes;
  if (avail_nodes == 0) return NULL;

  unsigned rand_idx, current_idx;
  // Try to hit a free node
  if (((avail_nodes * 100) / all_nodes) > RANDOM_LOOKUP_TRESHOLD_PERCENTAGE) {
    for (uint8_t i = 0; i < RANDOM_LOOKUP_TRIES; i++) {
      rand_idx = splitmix32() % all_nodes;
      node = get_jump_tree_item(chunk->jump_tree, level_base_idx + rand_idx);
      if (node != 0)
        return chunk_allocate_run_with_node(chunk, node,
                                            level_base_idx + rand_idx);
    }
  } else {
    // Get random node index
    rand_idx = splitmix32() % avail_nodes;
    current_idx = base_level_idx + chunk->jump_tree_first_index[idx];
    for (jump_node_t node = get_jump_tree_item(chunk->jump_tree, current_idx);
         node.next != 0;
         node = get_jump_tree_item(chunk->jump_tree, current_idx + node.next)) {
      if (rand_idx == 0)
        return chunk_allocate_run_with_node(chunk, node, current_idx);
      rand_idx--;
    }
  }
  se_error("Should never reach here");
}

// Mark nodes as split up the tree when coalescing free nodes ends
static void mark_nodes_split(uint8_t *mem, unsigned idx) {
  chunk_node_t state;
  // Here we want to also change the root to split
  while (idx > 0) {
    state = get_buddy_tree_item(mem, idx);
    if (state == NODE_FULL) {
      set_tree_item(mem, idx, NODE_SPLIT);
    } else
      break;
    idx = PARENT(idx);
  }
}

// Coalesce nodes up the tree during deallocate to indicate that this branch is
// free, change full nodes to split ones
static void coalesce_free_nodes(uint8_t *mem, unsigned idx) {
  unsigned neigh_idx;
  chunk_node_t state;
  while (idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(idx) ? idx - 1 : idx + 1;
    state = get_buddy_tree_item(mem, neigh_idx);
    if (state == NODE_FREE) {
      set_buddy_tree_item(mem, PARENT(idx), NODE_FREE);
    } else {
      mark_nodes_split(mem, PARENT(idx));
      break;
    }
    idx = PARENT(idx);
  }
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
      set_tree_item(chunk->buddy_tree, PARENT(ctx->idx), NODE_UNMAPPED);
    } else
      break;
    buddy_state_go_up(ctx);
  }
}

// Coalesce nodes up the tree during allocation to indicate that this branch is
// free, change full node to split ones
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
    set_tree_item(chunk->buddy_tree, ctx->idx, NODE_UNMAPPED);
    coalesce_unmapped_nodes(ctx, chunk);
  }
}

bool chunk_deallocate_run(chunk_t *chunk, void *run_ptr) {
  uintptr_t ptr_dest = (uintptr_t)run_ptr;
  chunk_node_t node, neigh;
  buddy_ctx_t ctx = {
      .idx = 1,
      .state = DOWN,
      .depth_to_leaf = CHUNK_BUDDY_TREE_DEPTH,
      .cur_size = CHUNK_SIZE_BYTES,
      .ptr = (uintptr_t)chunk->entry.key,
  };
  node = get_buddy_tree_item(chunk->buddy_tree, ctx.idx);
  while (!(ptr_dest == ctx.ptr && node == NODE_USED)) {
    if (IS_LEAF(ctx.idx)) {
      se_error("No run found");
    }
    // Go to right child
    if (ptr_dest >= ctx.ptr + (ctx.cur_size / 2)) {
      buddy_state_go_right(&ctx);
    }
    // Go to left child
    else {
      buddy_state_go_left(&ctx);
    }
    node = get_buddy_tree_item(chunk->buddy_tree, ctx.idx);
  }
  // Mark the node as depleted as we wont be using this memory again
  set_buddy_tree_item(chunk->buddy_tree, ctx.idx, NODE_DEPLETED);
  // Set leftmost as guarded to indicate that other allocations do not need to
  // guard pages
  // Note that leaf node will be just NODE_DEPLETED since leftmost of leaf ==
  // leaf
  set_buddy_tree_item(chunk->buddy_tree,
                      get_leftmost_idx(ctx.idx, ctx.depth_to_leaf),
                      NODE_DEPLETED);

  // Guard the memory region, may be unnecessary because we might be unmapping
  // it later
  platform_status_code_t code;
  if ((code = platform_guard((void *)ctx.ptr, ctx.cur_size) !=
              PLATFORM_STATUS_OK)) {
    se_error("Failed to guard page (ptr : %p, size : %zu): %s", (void *)ctx.ptr,
             ctx.cur_size, platform_strerror(code));
  }

  unsigned neigh_idx = get_rightmost_idx(ctx.idx, ctx.depth_to_leaf) + 1;
  if (neigh_idx <= CHUNK_NO_NODES) {
    neigh = get_buddy_tree_item(chunk->buddy_tree, neigh_idx);
    if (neigh == NODE_GUARD) {
      // That means the guard page was not used before
      // Unguard it and make it free
      if ((code = platform_unguard((void *)(ctx.ptr + ctx.cur_size),
                                   CHUNK_LEAST_REGION_SIZE_BYTES)) !=
          PLATFORM_STATUS_OK) {
        se_error("Failed to unguard page (ptr : %p, size : %zu): %s",
                 (void *)(ctx.ptr + ctx.cur_size),
                 CHUNK_LEAST_REGION_SIZE_BYTES, platform_strerror(code));
      }
      set_buddy_tree_item(chunk->buddy_tree, neigh_idx, NODE_FREE);
      chunk->free_mem += CHUNK_LEAST_REGION_SIZE_BYTES;
      coalesce_free_nodes(chunk->buddy_tree, neigh_idx);
    }
  }

  coalesce_depleted_nodes(&ctx, chunk);
  // Check if we unmapped the entire mapping to return information to delete
  // the chunk metadata
  // If idx is root (idx == 1) after coalescing, we definetely unmapped the
  // entire chunk
  if (ctx.idx == 1) return true;
  return false;
}

// Fills run data based of ptr
void chunk_get_run_ptr(chunk_t *chunk, void *ptr, void **run_ptr,
                       unsigned *run_size, unsigned *reg_size) {
  assert(*run_ptr == NULL);
  assert(*run_size == 0);
  assert(*reg_size == 0);

  uintptr_t ptr_dest = (uintptr_t)ptr;
  chunk_node_t node;
  buddy_ctx_t ctx = {
      .idx = 1,
      .state = DOWN,
      .depth_to_leaf = CHUNK_BUDDY_TREE_DEPTH,
      .cur_size = CHUNK_SIZE_BYTES,
      .ptr = (uintptr_t)chunk->entry.key,
  };
  node = get_buddy_tree_item(chunk->buddy_tree, ctx.idx);
  while (node == NODE_SPLIT || node == NODE_FULL) {
    if (IS_LEAF(ctx.idx)) {
      return;
    }
    if (ptr_dest >= ctx.ptr + (ctx.cur_size / 2))
      buddy_state_go_right(&ctx);
    else
      buddy_state_go_left(&ctx);
    node = get_buddy_tree_item(chunk->buddy_tree, ctx.idx);
  }

  if (node != NODE_USED) return;
  *run_ptr = (void *)ctx.ptr;
  *run_size = ctx.cur_size;

  unsigned base = (CHUNK_NO_NODES + 1) / 2;
  unsigned idx = ctx.idx - base;
  uint8_t compressed_reg_size;

  if (IS_LEAF(ctx.idx)) {
    if (chunk->reg_size_small_medium[idx] != REG_MARK_BAD_VALUE) {
      compressed_reg_size = chunk->reg_size_small_medium[idx];
      if (compressed_reg_size == 0)
        *reg_size = (UINT8_MAX + 1) * SMALL_SIZE_CLASS_ALIGNMENT;
      else
        *reg_size = compressed_reg_size * SMALL_SIZE_CLASS_ALIGNMENT;
    }
  }
}

bool chunk_is_unmapped(chunk_t *chunk) {
  return get_tree_item(chunk->buddy_tree, 1) == NODE_UNMAPPED;
}
bool chunk_is_full(chunk_t *chunk) { return chunk->free_mem == 0; }
