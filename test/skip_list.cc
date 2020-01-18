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
  yaldb::SkipList<int> skip_list;
  ASSERT_TRUE(skip_list.Insert(1) != skip_list.end());
  ASSERT_TRUE(skip_list.Insert(2) != skip_list.end());
  ASSERT_TRUE(skip_list.Insert(1) == skip_list.end());
}
