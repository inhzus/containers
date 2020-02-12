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
  static constexpr size_t kCapacity = 1000;
  yaldb::impl::LRUCache<int> *cache_{
      new yaldb::impl::LRUCache<int>(
          kCapacity, std::bind(
              &CacheTest::Deleter, this, std::placeholders::_1))};
  std::vector<int> deleted_keys_;
  std::vector<int> deleted_values_;

  void Deleter(std::pair<std::string, int> *pair) {
    deleted_keys_.push_back(std::stoi(pair->first));
    deleted_values_.push_back(pair->second);
    std::default_delete<std::remove_pointer_t<decltype(pair)>>()(pair);
  }
//  std::unique_ptr<yaldb::Cache<int>> cache_{
//      std::make_unique<yaldb::impl::LRUCache<int>>(kCapacity)};
};

TEST_CASE_METHOD(CacheTest, "hit and miss of LRU cache", "[Cache]") {
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

  REQUIRE(1 == deleted_keys_.size());
  REQUIRE(100 == deleted_keys_.front());
  REQUIRE(101 == deleted_values_.front());
}

TEST_CASE_METHOD(CacheTest, "erasing element of LRU cache") {
  REQUIRE(deleted_keys_.empty());

  Put(100, 101);
  Put(200, 201);
  REQUIRE(101 == Del(100));
  REQUIRE(kNull == Get(100));
  REQUIRE(201 == Get(200));
  REQUIRE(1 == deleted_keys_.size());
  REQUIRE(100 == deleted_keys_.front());
  REQUIRE(101 == deleted_values_.front());

  REQUIRE(kNull == Del(100));
  REQUIRE(kNull == Get(100));
  REQUIRE(201 == Get(200));
  REQUIRE(1 == deleted_keys_.size());
}

TEST_CASE_METHOD(CacheTest, "entries of LRU cache are pinned") {
  Put(100, 101);
  auto h1 = cache_->Del(std::to_string(100));
  REQUIRE(101 == h1->second);

  Put(100, 102);
  auto h2 = cache_->Del(std::to_string(100));
  REQUIRE(102 == h2->second);
  REQUIRE(deleted_keys_.empty());

  h1.reset();
  REQUIRE(1 == deleted_keys_.size());
  REQUIRE(100 == deleted_keys_.front());
  REQUIRE(101 == deleted_values_.front());

  Del(100);
  REQUIRE(kNull == Get(100));
  REQUIRE(1 == deleted_keys_.size());

  h2.reset();
  REQUIRE(2 == deleted_keys_.size());
  REQUIRE(100 == deleted_keys_.back());
  REQUIRE(102 == deleted_values_.back());
}

TEST_CASE_METHOD(CacheTest, "eviction policy of LRU cache") {
  Put(100, 101);
  Put(200, 201);
  Put(300, 301);
  auto h = cache_->Get(std::to_string(300));

  for (int i = 0; i < 100 + static_cast<int>(kCapacity); ++i) {
    Put(1000 + i, 2000 + i);
    REQUIRE(2000 + i == Get(1000 + i));
    REQUIRE(101 == Get(100));
  }
  REQUIRE(101 == Get(100));
  REQUIRE(kNull == Get(200));
  REQUIRE(301 == Get(300));
  h.reset();
}

TEST_CASE_METHOD(CacheTest, "zero size LRU cache") {
  delete cache_;
  cache_ = new yaldb::impl::LRUCache<int>(0);
  Put(1, 100);
  REQUIRE(kNull == Get(1));
}
