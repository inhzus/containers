//
// Copyright [2020] <inhzus>
//

#include "yaldb/cache.h"
#include "catch2/catch.hpp"

class CacheTest {
 public:
//  CacheTest() : cache_(new yaldb::impl::LRUCache<int>(kCapacity)) {}
  void Put(int key, int value) {
    cache_->Put(std::to_string(key), value);
  }
  int Get(int key) {
    auto value = cache_->Get(std::to_string(key));
    return value == nullptr ? kNull : value->second;
  }
  int Del(int key) {
    auto value = cache_->Del(std::to_string(key));
    return value == nullptr ? kNull : value->second;
  }
 protected:
  static constexpr int kNull = -1;
  static constexpr size_t kCapacity = 8;
  yaldb::impl::LRUCache<int> *cache_{new yaldb::impl::LRUCache<int>(kCapacity)};
//  std::unique_ptr<yaldb::Cache<int>> cache_{
//      std::make_unique<yaldb::impl::LRUCache<int>>(kCapacity)};
};

TEST_CASE_METHOD(CacheTest, "LRU cache hit and miss", "[Cache]") {
  REQUIRE(Get(1) == kNull);

  Put(100, 101);
  REQUIRE(101 == Get(100));
  REQUIRE(kNull == Get(200));
  REQUIRE(kNull == Get(300));

  Put(200, 201);
  REQUIRE(101 == Get(100));
  REQUIRE(201 == Get(200));
  REQUIRE(kNull == Get(300));

  Put(100, 102);
  REQUIRE(102 == Get(100));
  REQUIRE(201 == Get(200));
  REQUIRE(kNull == Get(300));
}
