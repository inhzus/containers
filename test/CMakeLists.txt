add_executable(yaldb_test
        leveldb.cc
        main.cc
        skip_list.cc)
enable_testing()
find_package(GTest REQUIRED)
find_package(leveldb REQUIRED)
target_link_libraries(yaldb_test GTest::GTest GTest::Main leveldb::leveldb)

