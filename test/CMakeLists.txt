enable_testing()
find_package(Threads REQUIRED)
find_package(leveldb REQUIRED)
add_executable(yaldb_test
        cache.cc
        leveldb.cc
        main.cc
        skip_list.cc)
target_link_libraries(yaldb_test leveldb::leveldb)
