enable_testing()

add_library(test_cflags INTERFACE)
target_compile_features(test_cflags INTERFACE cxx_std_14)

add_executable(
  hello_test
  hello_test.cpp
)
target_link_libraries(
  hello_test
  GTest::gtest_main
  sealloc
  test_cflags
)

include(GoogleTest)
gtest_discover_tests(hello_test)

