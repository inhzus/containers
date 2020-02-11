//
// Copyright [2020] <inhzus>
//

#include "yaldb/cache.h"
#include "catch2/catch.hpp"

TEST_CASE("cache construct", "[Cache]") {
  auto cache = std::make_unique<yaldb::impl::LRUCache<int>>(1);
  cache->Put("a", std::make_shared<int>(1));
  cache->Put("b", std::make_shared<int>(2));
  REQUIRE(cache->Get("a") == nullptr);
  REQUIRE(cache->Del("b").operator*() == 2);
}
