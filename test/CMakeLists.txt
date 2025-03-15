enable_testing()

add_library(test_cflags INTERFACE)
target_compile_features(test_cflags INTERFACE cxx_std_20)


add_executable(test_ia
  test_ia.cpp
)
target_link_libraries(test_ia
  GTest::gtest_main
  sealloc
  test_cflags
)

include(GoogleTest)
gtest_discover_tests(test_ia)

