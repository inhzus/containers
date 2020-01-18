//
// Copyright [2020] <inhzus>
//

#include <leveldb/db.h>
#include <gtest/gtest.h>

TEST(Leveldb, Init) {
  leveldb::DB *db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status = leveldb::DB::Open(options, "/tmp/init_db", &db);
  assert(status.ok());
//  std::string value;
//  auto s = db->Get(leveldb::ReadOptions(), "key", &value);
//  assert(s.IsNotFound());
//  s = db->Put(leveldb::WriteOptions(), "key", "ok");
//  assert(s.ok());
//  s = db->Get(leveldb::ReadOptions(), "key", &value);
//  assert(value == "ok");
}
