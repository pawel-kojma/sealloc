enable_testing()

add_library(test_cflags INTERFACE)
target_compile_features(test_cflags INTERFACE cxx_std_20)

list(APPEND test_libs 
  GTest::gtest_main
  sealloc
  test_cflags
)

add_executable(test_ia test_ia.cpp)
add_executable(test_run test_run.cpp)
add_executable(test_bin test_bin.cpp)
target_link_libraries(test_ia ${test_libs})
target_link_libraries(test_run ${test_libs})
target_link_libraries(test_bin ${test_libs})
target_compile_options(test_bin PRIVATE -O0 -ggdb)

include(GoogleTest)
gtest_discover_tests(test_ia)
gtest_discover_tests(test_run)
gtest_discover_tests(test_bin)

