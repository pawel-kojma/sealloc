enable_testing()

set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
add_library(test_cflags INTERFACE)
target_compile_features(test_cflags INTERFACE cxx_std_20)
target_compile_options(test_cflags INTERFACE $<${gcc_like_c}:-O0 -ggdb>)

list(APPEND sealloc_internal_srcs
    sealloc.c
    platform.c
    chunk.c
    bin.c
    internal_allocator.c
    random.c
    run.c
    container_ll.c
    arena.c
    size_class.c)
list(TRANSFORM sealloc_internal_srcs PREPEND "${PROJECT_SOURCE_DIR}/src/")

add_library(sealloc_internal SHARED ${sealloc_internal_srcs})
target_include_directories(sealloc_internal PUBLIC "${PROJECT_SOURCE_DIR}/include/internal/")
target_compile_options(sealloc_internal PRIVATE
    $<${gcc_like_c}:-Wall -Werror -Wextra -pedantic -Wformat=0>
    ${EXTRA_COMPILE_OPTIONS}
)
target_compile_definitions(sealloc_internal PRIVATE LOGGING)

list(APPEND test_libs 
  GTest::gtest_main
  sealloc_internal
  test_cflags
)

list(APPEND tests_e2e 
            free_huge              
            free_invalid           
            free_regular           
            malloc_huge
            malloc_huge_multi
            malloc_memory_integrity
            malloc_multi           
            malloc_single          
            realloc_invalid
            realloc_regular
            realloc_huge
            realloc_multi
            calloc
)
list(APPEND test_srcs
    test_ia test_run test_container_ll test_bin test_chunk test_arena test_size_class)
include(GoogleTest)

foreach(test_src ${test_srcs})
    add_executable(${test_src} "${test_src}.cpp")
    target_link_libraries(${test_src} ${test_libs})
    gtest_discover_tests(${test_src})
endforeach()

foreach(test_e2e ${tests_e2e})
    add_executable(${test_e2e} "./test_sealloc/${test_e2e}.cpp")
    set_target_properties(${test_e2e} PROPERTIES OUTPUT_NAME "e2e_${test_e2e}")
    target_link_libraries(${test_e2e} ${test_libs})
    gtest_discover_tests(${test_e2e})
endforeach()

