#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/chunk.h>
}

constexpr uint8_t tree_mem[] = {};

TEST(ChunkUtils, GetNodeInfo){

    chunk_t chunk;
    std::memset(&chunk.buddy_tree, 0, sizeof(chunk.buddy_tree));
    
}
