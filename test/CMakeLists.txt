enable_testing()

set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
add_library(test_cflags INTERFACE)
target_compile_features(test_cflags INTERFACE cxx_std_20)
target_compile_options(test_cflags INTERFACE $<${gcc_like_c}:-O0 -ggdb>)

list(APPEND test_libs 
  GTest::gtest_main
  sealloc
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

