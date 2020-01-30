//
// Copyright [2020] <inhzus>
//

#include "yaldb/skip_list.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <functional>

template<typename T, typename F, std::enable_if_t<
    std::is_invocable_v<F, const T &, const T &> &&
        std::is_same_v<std::invoke_result_t<F, const T &, const T &>, bool>,
    int> = 0>
class _SkipList {
 public:
  _SkipList() {
    printf("constructor: %s\n", F()(1, 2) ? "true" : "false");
  }
};

TEST(SkipList, Template) {
  struct cmp {
    bool operator()(int a, int b) {
      return a < b;
    }
  };
  _SkipList<int, cmp> skip_list;
}

TEST(SkipList, Insert) {
  yaldb::SkipList<size_t> skip_list;
  const size_t kLength = 1024;
  for (size_t i = 0; i < kLength; ++i) {
    auto it = skip_list.Insert(i);
    ASSERT_NE(it, skip_list.end());
    ASSERT_EQ(*it, i);
    ASSERT_EQ(skip_list.size(), i + 1);
  }
  auto it = skip_list.begin();
  for (size_t i = 0; i < kLength; ++i, ++it) {
    ASSERT_NE(it, skip_list.end());
    ASSERT_EQ(*it, i);
  }
  for (size_t i = 0; i < kLength; ++i) {
    ASSERT_NE(skip_list.Find(i), skip_list.end());
  }
//  for (size_t i = 0; i < kLength; ++i) {
//    ASSERT_EQ(skip_list.Insert(i), skip_list.end());
//  }
  for (size_t i = 0; i < kLength; ++i) {
    ASSERT_NE(skip_list.Erase(i), skip_list.end());
    ASSERT_EQ(skip_list.size(), kLength - i - 1);
  }
  for (size_t i = 0; i < kLength; ++i) {
    ASSERT_EQ(skip_list.size(), i);
    ASSERT_NE(skip_list.Insert(i), skip_list.end());
  }
}

TEST(SkipList, Erase) {
  yaldb::SkipList<size_t> skip_list;
  const size_t value = 10;
  for (size_t i = 0; i < 10; ++i) {
    skip_list.Insert(value);
  }
  ASSERT_NE(skip_list.Erase(value), skip_list.end());
  ASSERT_EQ(skip_list.size(), 0);
}
