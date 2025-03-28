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
list(APPEND test_srcs
    test_ia test_run test_container_ll test_bin test_chunk)
include(GoogleTest)

foreach(test_src ${test_srcs})
    add_executable(${test_src} "${test_src}.cpp")
    target_link_libraries(${test_src} ${test_libs})
    gtest_discover_tests(${test_src})
endforeach()

