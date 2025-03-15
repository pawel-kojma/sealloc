add_library(sealloc bin.c internal_allocator.c random.c run.c)
target_include_directories(sealloc PUBLIC "${PROJECT_SOURCE_DIR}/include")
