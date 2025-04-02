#include <gtest/gtest.h>

extern "C" {
#include <sealloc/size_class.h>
#include <sealloc/utils.h>
}

TEST(SizeClass, Small) {
  EXPECT_TRUE(IS_SIZE_SMALL(SMALL_SIZE_MIN_REGION));
  EXPECT_TRUE(IS_SIZE_SMALL(SMALL_SIZE_MAX_REGION));

  EXPECT_EQ(ALIGNUP_SMALL_SIZE(SMALL_SIZE_MIN_REGION + 1),
            2 * SMALL_SIZE_CLASS_ALIGNMENT);
  EXPECT_EQ(
      ALIGNUP_SMALL_SIZE(SMALL_SIZE_MIN_REGION + SMALL_SIZE_CLASS_ALIGNMENT),
      2 * SMALL_SIZE_CLASS_ALIGNMENT);
}

TEST(SizeClass, Medium) {
  EXPECT_TRUE(IS_SIZE_MEDIUM(MEDIUM_SIZE_MIN_REGION));
  EXPECT_TRUE(IS_SIZE_MEDIUM(MEDIUM_SIZE_MAX_REGION));

  EXPECT_EQ(ALIGNUP_MEDIUM_SIZE(MEDIUM_SIZE_MIN_REGION + 1),
            2 * MEDIUM_SIZE_CLASS_ALIGNMENT);
  EXPECT_EQ(
      ALIGNUP_MEDIUM_SIZE(MEDIUM_SIZE_MIN_REGION + MEDIUM_SIZE_CLASS_ALIGNMENT),
      2 * MEDIUM_SIZE_CLASS_ALIGNMENT);
}

TEST(SizeClass, LargeAlignUp) {
  EXPECT_TRUE(IS_SIZE_LARGE(LARGE_SIZE_MIN_REGION));
  EXPECT_TRUE(IS_SIZE_LARGE(LARGE_SIZE_MAX_REGION));

  EXPECT_EQ(alignup_large_size(LARGE_SIZE_MIN_REGION + 1),
            2 * LARGE_SIZE_MIN_REGION);

  unsigned next;
  for (unsigned i = 1; i <= NO_LARGE_SIZE_CLASSES; i <<= 1) {
    next = LARGE_SIZE_MIN_REGION * i;
    EXPECT_EQ(alignup_large_size(next - 42), next);
  }
}

TEST(SizeClass, LargeSizeToIdx) {
  unsigned next, v = 1;
  for (unsigned i = 0; i < NO_LARGE_SIZE_CLASSES; i++) {
    next = LARGE_SIZE_MIN_REGION * v;
    v <<= 1;
    EXPECT_EQ(size_to_idx_large(next), i);
  }
}
