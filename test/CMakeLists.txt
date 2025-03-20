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

add_executable(test_ia test_ia.cpp)
add_executable(test_run test_run.cpp)
add_executable(test_container_ll test_container_ll.cpp)
add_executable(test_bin test_bin.cpp)
target_link_libraries(test_ia ${test_libs})
target_link_libraries(test_run ${test_libs})
target_link_libraries(test_container_ll ${test_libs})
target_link_libraries(test_bin ${test_libs})

include(GoogleTest)
gtest_discover_tests(test_ia)
gtest_discover_tests(test_run)
gtest_discover_tests(test_container_ll)
gtest_discover_tests(test_bin)

