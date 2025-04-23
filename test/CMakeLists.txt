enable_testing()

set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
add_library(test_cflags INTERFACE)
add_library(test_cxxflags INTERFACE)
target_compile_features(test_cxxflags INTERFACE cxx_std_20)
target_compile_options(test_cflags INTERFACE $<${gcc_like_c}:-O0 -ggdb3>)

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
    logging.c
    size_class.c
    utils.c
)
list(TRANSFORM sealloc_internal_srcs PREPEND "${PROJECT_SOURCE_DIR}/src/")

add_library(sealloc_internal SHARED ${sealloc_internal_srcs})
target_include_directories(sealloc_internal PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(sealloc_internal test_cflags)
target_compile_options(sealloc_internal PRIVATE
    $<${gcc_like_c}:-Wall -Werror -Wextra -pedantic -Wformat=0>
)
target_compile_definitions(sealloc_internal PRIVATE 
    "$<$<BOOL:${Log}>:LOGGING>"
    DEBUG
)

list(APPEND test_libs 
  GTest::gtest_main
  sealloc_internal
  test_cxxflags
)

list(APPEND tests_integration 
            free_huge              
            free_invalid           
            free_regular           
            malloc_huge
            malloc_huge_multi
            malloc_memory_integrity
            malloc_bin_state_correct
            malloc_single          
            realloc_invalid
            realloc_regular
            realloc_huge
            realloc_multi
            calloc
            fuzz_api
            locate_metadata
)
list(APPEND test_srcs
    test_ia test_run test_container_ll test_bin test_chunk test_arena test_size_class)


list(APPEND tests_e2e
            arbitrary_free_near             
            double_free_close               
            double_free_far                 
            heap_spray_prevention           
            overflow_bigger_alloc_prevention
            overflow_large_detection        
            overflow_medium_detection       
            overflow_simple_prevention      
            overflow_small_detection        
            randomized_allocations          
            uaf_close                       
            uaf_many_no_duplicates
)

include(GoogleTest)

foreach(test_src ${test_srcs})
    add_executable(${test_src} "${test_src}.cpp")
    set_target_properties(${test_src} PROPERTIES OUTPUT_NAME "ut_${test_src}")
    target_link_libraries(${test_src} ${test_libs})
    gtest_discover_tests(${test_src})
endforeach()

foreach(test_integration ${tests_integration})
    add_executable(${test_integration} "./test_integration/${test_integration}.cpp")
    set_target_properties(${test_integration} PROPERTIES OUTPUT_NAME "integration_${test_integration}")
    target_link_libraries(${test_integration} ${test_libs})
    gtest_discover_tests(${test_integration})
endforeach()

foreach(test_e2e ${tests_e2e})
    add_executable(${test_e2e} "./test_e2e/${test_e2e}.c")
    set_target_properties(${test_e2e} PROPERTIES OUTPUT_NAME "e2e_${test_e2e}")
    target_link_libraries(${test_e2e} test_cflags)
endforeach()
