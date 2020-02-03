//
// Copyright [2020] <inhzus>
//

#include <leveldb/db.h>
#include <catch2/catch.hpp>

TEST_CASE("leveldb init", "[leveldb]") {
  leveldb::DB *db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/tmp/init_db", &db);
  REQUIRE(status.ok());
}
