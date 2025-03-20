#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include <sealloc/container_ll.h>
}

typedef struct TestStructHead {
  ll_head_t head;
} head_t;

typedef struct TestStructItem {
  ll_entry_t entry;
  int a;
  int b;
} test_t;

TEST(ContainerLL, AddFind) {
  head_t h;
  test_t t;
  t.a = 4;
  t.b = 8;
  t.entry.key = (void *)42;
  ll_init(&h.head);
  EXPECT_EQ(h.head.ll, nullptr);
  EXPECT_EQ(ll_find(&h.head, &t.entry), nullptr);
  ll_add(&h.head, &t.entry);
  EXPECT_EQ(ll_find(&h.head, (void *)42), &t.entry);
}

TEST(ContainerLL, AddFind2) {
  head_t h;
  test_t t1, t2, t3;
  t1.entry.key = (void *)42;
  t2.entry.key = (void *)41;
  t3.entry.key = (void *)43;
  ll_init(&h.head);
  ll_add(&h.head, &t1.entry);
  EXPECT_EQ(ll_find(&h.head, (void *)42), &t1.entry);
  EXPECT_EQ(ll_find(&h.head, (void *)50), nullptr);
}

TEST(ContainerLL, AddDel) {
  head_t h;
  test_t t1, t2, t3, t4;
  t1.entry.key = (void *)42;
  t2.entry.key = (void *)41;
  t3.entry.key = (void *)43;
  t4.entry.key = (void *)21;
  ll_init(&h.head);
  ll_add(&h.head, &t1.entry);
  ll_add(&h.head, &t2.entry);
  ll_add(&h.head, &t3.entry);
  ll_add(&h.head, &t4.entry);
  EXPECT_EQ(ll_find(&h.head, (void *)21), &t4.entry);
  ll_del(&h.head, &t4.entry);
  EXPECT_EQ(ll_find(&h.head, (void *)21), nullptr);
  ll_del(&h.head, &t1.entry);
  ll_del(&h.head, &t3.entry);
  ll_del(&h.head, &t2.entry);
  EXPECT_EQ(h.head.ll, nullptr);
}
