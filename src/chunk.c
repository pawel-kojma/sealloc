#include <sealloc/chunk.h>
#include <sealloc/logging.h>
#include <sealloc/platform_api.h>
#include <sealloc/run.h>
#include <sealloc/utils.h>
#include <string.h>

#define RIGHT_CHILD(idx) (idx * 2 + 1)
#define LEFT_CHILD(idx) (idx * 2)
#define PARENT(idx) (idx / 2)
#define IS_ROOT(idx) (idx != 1)
#define IS_LEAF(idx) (idx >= ((CHUNK_NO_NODES + 1) / 2))
#define IS_RIGHT_CHILD(idx) (idx & 1)

typedef enum chunk_node {
  NODE_FREE = 0,
  NODE_SPLIT = 1,
  NODE_USED_SET_GUARD = 2,
  NODE_USED_FOUND_GUARD = 3,
  NODE_FULL = 4,
  NODE_GUARD = 5,
  NODE_DEPLETED = 6,
  NODE_UNMAPPED = 7
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

// Get tree node type at given index
static chunk_node_t get_tree_item(uint8_t *mem, unsigned idx) {
  unsigned bits_to_skip = (idx - 1) * NODE_STATE_BITS;
  unsigned word_idx = bits_to_skip / 8, off = bits_to_skip % 8;
  uint8_t fst_part = (mem[word_idx] >> off) & get_mask(min(8 - off, 3));
  uint8_t snd_part = 0;
  if (NODE_STATE_BITS + off > 8) {
    snd_part = mem[word_idx + 1] & get_mask(off + NODE_STATE_BITS - 8);
    return (chunk_node_t)((snd_part << (8 - off)) | fst_part);
  }
  return (chunk_node_t)fst_part;
}

// Set tree node type to given type at index
static void set_tree_item(uint8_t *mem, unsigned idx, chunk_node_t node_state) {
  unsigned bits_to_skip = (idx - 1) * NODE_STATE_BITS;
  unsigned word_idx = bits_to_skip / 8, off = bits_to_skip % 8;
  unsigned remaining, fst_bits;
  uint8_t state = (uint8_t)node_state;

  if (NODE_STATE_BITS + off > 8) {
    remaining = NODE_STATE_BITS + off - 8;
    fst_bits = NODE_STATE_BITS - remaining;

    // Set bits in the first word
    mem[word_idx] =
        (mem[word_idx] & ~(get_mask(fst_bits) << off)) | (state << off);
    // Set remaining bits in the second word
    mem[word_idx + 1] =
        (mem[word_idx + 1] & ~get_mask(remaining)) | (state >> fst_bits);
  } else {
    // First, clear the bits we want to set in mem[word]
    // Second, set two node_state bits in their place
    mem[word_idx] = (mem[word_idx] & ~(get_mask(NODE_STATE_BITS) << off)) |
                    ((uint8_t)node_state << off);
  }
}

// Mark nodes as split up the tree when setting guard page on allocation
static void mark_nodes_split_guard(uint8_t *mem, unsigned idx) {
  chunk_node_t state;
  // Here we want to also change the root to split
  while (idx > 0) {
    state = get_tree_item(mem, idx);
    if (state == NODE_SPLIT) break;
    set_tree_item(mem, idx, NODE_SPLIT);
    idx = PARENT(idx);
  }
}

// Coalesce nodes up the tree during allocation to indicate that this branch is
// full
static void coalesce_full_nodes(uint8_t *mem, unsigned idx) {
  unsigned neigh_idx;
  chunk_node_t state;
  while (idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(idx) ? idx - 1 : idx + 1;
    state = get_tree_item(mem, neigh_idx);
    if (state == NODE_FULL || state == NODE_USED_FOUND_GUARD ||
        state == NODE_USED_SET_GUARD) {
      set_tree_item(mem, PARENT(idx), NODE_FULL);
    } else
      break;
    idx = PARENT(idx);
  }
}

static inline unsigned get_rightmost_idx(unsigned idx, unsigned depth) {
  return (idx << depth) + (1 << depth) - 1;
}

static inline unsigned get_leftmost_idx(unsigned idx, unsigned depth) {
  return (idx << depth);
}

void chunk_init(chunk_t *chunk, void *heap) {
  chunk->entry.key = heap;
  memset(chunk->reg_size_small_medium, 0xff,
         sizeof(chunk->reg_size_small_medium));
  chunk->buddy_tree[0] = (uint8_t)NODE_FREE;
  chunk->free_mem = CHUNK_SIZE_BYTES;
  memset(&chunk->buddy_tree, 0, sizeof(chunk->buddy_tree));
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

// Sets info about what region size is allocated at which run ptr
void mark_reg_size(buddy_ctx_t *ctx, chunk_t *chunk, unsigned reg_size) {
  if (!IS_SIZE_SMALL(reg_size) && !IS_SIZE_MEDIUM(reg_size)) return;
  unsigned base = (CHUNK_NO_NODES + 1) / 2;
  // We know that if reg_size is small or medium, then we allocated at the leaf
  // 0-based index
  unsigned idx = ctx->idx - base;
  // Note that reg_size 4096 will eval to 0
  chunk->reg_size_small_medium[idx] =
      (reg_size / SIZE_CLASS_ALIGNMENT) & UINT8_MAX;
}

void *visit_leaf_node(buddy_ctx_t *ctx, chunk_t *chunk, unsigned reg_size) {
  unsigned neigh_idx = ctx->idx + 1;
  chunk_node_t neigh, node = get_tree_item(chunk->buddy_tree, ctx->idx);
  switch (node) {
    case NODE_FREE:
      // Check if we can place a guard page.
      if (neigh_idx > CHUNK_NO_NODES)
        neigh = NODE_GUARD;
      else
        neigh = get_tree_item(chunk->buddy_tree, neigh_idx);
      if (neigh != NODE_USED_FOUND_GUARD && neigh != NODE_USED_SET_GUARD) {
        if (neigh == NODE_FREE) {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED_SET_GUARD);
          // Guard neighbor
          if (platform_guard((void *)(ctx->ptr + ctx->cur_size),
                             CHUNK_LEAST_REGION_SIZE_BYTES) < 0) {
            se_error("Failed call platform_guard(%p, %u)",
                     (void *)(ctx->ptr + ctx->cur_size),
                     CHUNK_LEAST_REGION_SIZE_BYTES);
          }
          set_tree_item(chunk->buddy_tree, neigh_idx, NODE_GUARD);
          mark_nodes_split_guard(chunk->buddy_tree, PARENT(neigh_idx));
        } else {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED_FOUND_GUARD);
        }
        chunk->free_mem -= ctx->cur_size;
        mark_reg_size(ctx, chunk, reg_size);
        coalesce_full_nodes(chunk->buddy_tree, ctx->idx);
        return (void *)ctx->ptr;
      }
      // fallthrough if neighbor node is not free or guarded.
    case NODE_GUARD:
    case NODE_USED_FOUND_GUARD:
    case NODE_USED_SET_GUARD:
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
  chunk_node_t neigh, node = get_tree_item(chunk->buddy_tree, ctx->idx);
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
        neigh = get_tree_item(chunk->buddy_tree, neigh_idx);
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
      else if (neigh != NODE_USED_FOUND_GUARD && neigh != NODE_USED_SET_GUARD) {
        if (neigh == NODE_FREE) {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED_SET_GUARD);
          set_tree_item(chunk->buddy_tree,
                        get_leftmost_idx(ctx->idx, ctx->depth_to_leaf),
                        NODE_USED_SET_GUARD);
          // Guard neighbor
          if (platform_guard((void *)(ctx->ptr + ctx->cur_size),
                             CHUNK_LEAST_REGION_SIZE_BYTES) < 0) {
            se_error("Failed call platform_guard(%p, %u)",
                     (void *)(ctx->ptr + ctx->cur_size),
                     CHUNK_LEAST_REGION_SIZE_BYTES);
          }
          set_tree_item(chunk->buddy_tree, neigh_idx, NODE_GUARD);
          mark_nodes_split_guard(chunk->buddy_tree, PARENT(neigh_idx));
        } else {
          set_tree_item(chunk->buddy_tree, ctx->idx, NODE_USED_FOUND_GUARD);
          set_tree_item(chunk->buddy_tree,
                        get_leftmost_idx(ctx->idx, ctx->depth_to_leaf),
                        NODE_USED_FOUND_GUARD);
        }
        chunk->free_mem -= ctx->cur_size;
        coalesce_full_nodes(chunk->buddy_tree, ctx->idx);
        return (void *)ctx->ptr;
      } else {
        // We cannot guard a neighbor node and deeper nodes are too small
        // Go up
        buddy_state_go_up(ctx);
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
      buddy_state_go_up(ctx);
      break;
  }
  return NULL;
}

void *chunk_allocate_run(chunk_t *chunk, unsigned run_size, unsigned reg_size) {
  void *ptr;
  buddy_ctx_t ctx = {
      .idx = 1,
      .state = DOWN,
      .depth_to_leaf = CHUNK_BUDDY_TREE_DEPTH,
      .cur_size = CHUNK_SIZE_BYTES,
      .ptr = (uintptr_t)chunk->entry.key,
  };
  if (chunk->free_mem < run_size) return NULL;

  // Search stops when we come back to the root from the right child
  while (!(ctx.idx == 1 && ctx.state == UP_RIGHT)) {
    // If we are in a leaf node, visit and go up if needed.
    if (IS_LEAF(ctx.idx)) {
      ptr = visit_leaf_node(&ctx, chunk, reg_size);
      if (ptr != NULL) return ptr;
      continue;
    }
    switch (ctx.state) {
      case DOWN:
        ptr = visit_regular_node(&ctx, chunk, run_size);
        if (ptr != NULL) return ptr;
        break;
      case UP_LEFT:
        buddy_state_go_right(&ctx);
        break;
      case UP_RIGHT:
        buddy_state_go_up(&ctx);
        break;
    }
  }
  return NULL;
}

// Mark nodes as split up the tree when coalescing free nodes ends
static void mark_nodes_split(uint8_t *mem, unsigned idx) {
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

// Coalesce nodes up the tree during deallocate to indicate that this branch is
// free, change full nodes to split ones
static void coalesce_free_nodes(uint8_t *mem, unsigned idx) {
  unsigned neigh_idx;
  chunk_node_t state;
  while (idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(idx) ? idx - 1 : idx + 1;
    state = get_tree_item(mem, neigh_idx);
    if (state == NODE_FREE) {
      set_tree_item(mem, PARENT(idx), NODE_FREE);
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
    node = get_tree_item(chunk->buddy_tree, neigh_idx);
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
  while (ctx->idx > 1) {
    neigh_idx = IS_RIGHT_CHILD(ctx->idx) ? ctx->idx - 1 : ctx->idx + 1;
    node = get_tree_item(chunk->buddy_tree, neigh_idx);
    if (node == NODE_DEPLETED) {
      set_tree_item(chunk->buddy_tree, PARENT(ctx->idx), NODE_DEPLETED);
    } else
      break;
    buddy_state_go_up(ctx);
  }

  // We've merged as much depleted nodes as possible
  // Check if depth passed the unmap threshold
  if (ctx->depth_to_leaf >= CHUNK_UNMAP_THRESHOLD) {
    // We passed the threshold, unmap
    if (platform_unmap((void *)ctx->ptr, ctx->cur_size) < 0) {
      se_error("Failed call platform_unmap(%p, %u)", (void *)ctx->ptr,
               ctx->cur_size);
    }
    coalesce_unmapped_nodes(ctx, chunk);
  }
}

bool chunk_deallocate_run(chunk_t *chunk, run_t *run) {
  uintptr_t ptr_dest = (uintptr_t)run->entry.key;
  chunk_node_t node;
  buddy_ctx_t ctx = {
      .idx = 1,
      .state = DOWN,
      .depth_to_leaf = CHUNK_BUDDY_TREE_DEPTH,
      .cur_size = CHUNK_SIZE_BYTES,
      .ptr = (uintptr_t)chunk->entry.key,
  };
  node = get_tree_item(chunk->buddy_tree, ctx.idx);
  while (!(ptr_dest == ctx.ptr &&
           (node == NODE_USED_FOUND_GUARD || node == NODE_USED_SET_GUARD))) {
    if (IS_LEAF(ctx.idx)) {
      se_error("No run found");
    }
    ctx.cur_size /= 2;
    // Go to right child
    if (ptr_dest >= ctx.ptr + ctx.cur_size) {
      ctx.idx = RIGHT_CHILD(ctx.idx);
      ctx.ptr += ctx.cur_size;
    }
    // Go to left child
    else {
      ctx.idx = LEFT_CHILD(ctx.idx);
    }
    node = get_tree_item(chunk->buddy_tree, ctx.idx);
  }

  // Mark the node as depleted as we wont be using this memory again
  set_tree_item(chunk->buddy_tree, ctx.idx, NODE_DEPLETED);
  // Set leftmost as guarded to indicate that other allocations do not need to
  // guard pages
  set_tree_item(chunk->buddy_tree, get_leftmost_idx(ctx.idx, ctx.depth_to_leaf),
                NODE_GUARD);
  // Guard the memory region, may be unnecessary because we might be unmapping
  // it later
  if (platform_guard((void *)ctx.ptr, ctx.cur_size) < 0)
    se_error("Failed call platform_guard(%p, %u)", (void *)ctx.ptr,
             ctx.cur_size);

  unsigned neigh_idx = get_rightmost_idx(ctx.idx, ctx.depth_to_leaf) + 1;
  chunk_node_t neigh;
  if (neigh_idx < CHUNK_NO_NODES) {
    neigh = get_tree_item(chunk->buddy_tree, neigh_idx);
    if (neigh == NODE_USED_SET_GUARD) {
      // That means the guard page was not used before
      // Unguard it and make it free
      if (platform_unguard((void *)(ctx.ptr + ctx.cur_size),
                           CHUNK_LEAST_REGION_SIZE_BYTES) < 0) {
        se_error("Failed call platform_unguard(%p, %u)",
                 (void *)(ctx.ptr + ctx.cur_size),
                 CHUNK_LEAST_REGION_SIZE_BYTES);
      }
      set_tree_item(chunk->buddy_tree, neigh_idx, NODE_FREE);
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
