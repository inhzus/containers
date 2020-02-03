//
// Copyright [2020] <inhzus>
//

#include "yaldb/skip_list.h"

#include <catch2/catch.hpp>

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

TEST_CASE("comparator as template", "[SkipList]") {
  struct cmp {
    bool operator()(int a, int b) {
      return a < b;
    }
  };
  [[maybe_unused]] _SkipList<int, cmp> skip_list;
}

TEST_CASE("insertion and related components of SkipList", "[SkipList]") {
  yaldb::SkipList<size_t> skip_list;
  constexpr size_t kLength = 1024;
  for (size_t i = 0; i < kLength; ++i) {
    auto it = skip_list.Insert(i);
    REQUIRE_FALSE(it == skip_list.end());
    REQUIRE(*it == i);
    REQUIRE(skip_list.Size() == i + 1);
  }
  auto it = skip_list.begin();
  for (size_t i = 0; i < kLength; ++i, ++it) {
    REQUIRE_FALSE(it == skip_list.end());
    REQUIRE(*it == i);
  }
  for (size_t i = 0; i < kLength; ++i) {
    REQUIRE_FALSE(skip_list.Find(i) == skip_list.end());
  }
  for (size_t i = 0; i < kLength; ++i) {
    REQUIRE_FALSE(skip_list.Erase(i) == skip_list.end());
    REQUIRE(skip_list.Size() == kLength - i - 1);
  }
  for (size_t i = 0; i < kLength; ++i) {
    REQUIRE(skip_list.Size() == i);
    REQUIRE_FALSE(skip_list.Insert(i) == skip_list.end());
  }
}

TEST_CASE("erasing element and related components of SkipList", "[SkipList]") {
  constexpr size_t kCount = 20, kValue = 10;
  SECTION("erase value") {
    yaldb::SkipList<size_t> skip_list;
    for (size_t i = 0; i < kValue; i++) {
      skip_list.Insert(i);
    }
    for (size_t i = 0; i < kCount; i++) {
      skip_list.Insert(kValue);
    }
    auto[first, last] = skip_list.EqualRange(kValue);
    std::advance(first, kCount);
    REQUIRE(first == last);
    REQUIRE_FALSE(skip_list.Erase(kValue) == skip_list.end());
    REQUIRE(skip_list.Size() == kValue);
  }SECTION("erase iterator") {
    yaldb::SkipList<size_t> skip_list;
    for (size_t i = 0; i < kValue; ++i) {
      skip_list.Insert(i);
    }
    for (size_t i = 0; i < kCount; ++i) {
      skip_list.Insert(kValue);
    }
    auto[first, last] = skip_list.EqualRange(kValue);
    size_t i = kCount + kValue;
    for (auto it = first++; it != last; it = first++) {
      REQUIRE(skip_list.Erase(it) != skip_list.end());
      REQUIRE(skip_list.Size() == --i);
    }
  }
}
