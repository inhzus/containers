//
// Copyright [2020] <inhzus>
//
#ifndef YALDB_CACHE_H_
#define YALDB_CACHE_H_

#include <cassert>

#include <functional>
#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <string>
#include <unordered_map>
#include <utility>

#include "yaldb/thread_annotation.h"

namespace yaldb {

template<typename T>
class Cache;

template<typename T>
std::unique_ptr<Cache<T>> NewLRUCache(size_t capacity);

template<typename T>
class Cache {
 public:
  virtual ~Cache() = default;
  virtual void Put(const std::string &key, T value) = 0;
  [[nodiscard]] virtual std::shared_ptr<std::pair<std::string, T>>
  Get(const std::string &key) = 0;
  [[nodiscard]] virtual std::shared_ptr<std::pair<std::string, T>>
  Del(const std::string &key) = 0;
 private:
};

namespace impl {

template<typename T>
class LRUCache : public Cache<T> {
 public:
  LRUCache() : capacity_() {}
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}
  ~LRUCache() override = default;
  void Put(const std::string &key, T value) override;
  std::shared_ptr<std::pair<std::string, T>>
  Get(const std::string &key) override;
  std::shared_ptr<std::pair<std::string, T>>
  Del(const std::string &key) override;
  void set_capacity(size_t capacity) { capacity_ = capacity; }

 private:
  using PairType = std::pair<std::string, T>;
//  using PairPtr = std::shared_ptr<PairType>;
  using ListType = std::list<std::shared_ptr<PairType>>;
  using MapType = std::unordered_map<std::string, typename ListType::iterator>;
  size_t capacity_;
  std::mutex mutex_;
  // store the shared pointers to pair of key and value
  ListType list_ GUARDED_BY(mutex_);
  MapType map_ GUARDED_BY(mutex_);
};
template<typename T>
void LRUCache<T>::Put(const std::string &key, T value) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (auto found = map_.find(key); found != map_.end()) {
    // key->value pair inserted before
    // erase the old record in the list
    list_.erase(found->second);
  }
  // push new record into the list
  list_.push_front(std::make_shared<PairType>(
      std::make_pair(key, std::move(value))));
  // update / insert
  map_.insert_or_assign(key, list_.begin());

  assert(list_.size() == map_.size());
  if (list_.size() > capacity_) {
    // remove oldest item in the cache
    auto back = std::prev(list_.end());
    [[maybe_unused]] auto back_slot = map_.find((*back)->first);
    assert(back_slot != map_.end() && back_slot->second == back);
    map_.erase(back_slot);
    list_.pop_back();
  }
}
template<typename T>
std::shared_ptr<std::pair<std::string, T>>
LRUCache<T>::Get(const std::string &key) {
  std::lock_guard<std::mutex> guard(mutex_);
  auto found = map_.find(key);
  if (found == map_.end()) return nullptr;
  std::shared_ptr<PairType> value = *(found->second);
  list_.erase(found->second);
  list_.push_front(value);
  map_.insert_or_assign(key, list_.begin());
  return list_.front();
}
template<typename T>
std::shared_ptr<std::pair<std::string, T>>
LRUCache<T>::Del(const std::string &key) {
  std::lock_guard<std::mutex> guard(mutex_);
  auto found = map_.find(key);
  if (found == map_.end()) return nullptr;
  std::shared_ptr<PairType> value = *(found->second);
  list_.erase(found->second);
  map_.erase(found);
  return value;
}

template<typename T>
class SharedLRUCache : public Cache<T> {
 public:
  explicit SharedLRUCache(size_t capacity);
  ~SharedLRUCache() override = default;
  void Put(const std::string &key, T value) override;
  std::shared_ptr<std::pair<std::string, T>>
  Get(const std::string &key) override;
  std::shared_ptr<std::pair<std::string, T>>
  Del(const std::string &key) override;
 private:
  static constexpr size_t kNumShardBits = 4u;
  static constexpr size_t kNumShards = 1u << kNumShardBits;

  static size_t ShardHash(const std::string &key);

  LRUCache<T> shard_[kNumShards];
  size_t capacity_;
};
template<typename T>
SharedLRUCache<T>::SharedLRUCache(size_t capacity) : capacity_(capacity) {
  const size_t shard_capacity = (capacity + kNumShards - 1) / kNumShards;
  for (size_t i = 0; i < kNumShards; ++i) {
    shard_->set_capacity(shard_capacity);
  }
}
template<typename T>
void SharedLRUCache<T>::Put(const std::string &key, T value) {
  const size_t slot = ShardHash(key) & (kNumShards - 1);
  shard_[slot].Put(key, std::move(value));
}
template<typename T>
std::shared_ptr<std::pair<std::string, T>>
SharedLRUCache<T>::Get(const std::string &key) {
  const size_t slot = ShardHash(key) & (kNumShards - 1);
  return shard_[slot].Get(key);
}
template<typename T>
std::shared_ptr<std::pair<std::string, T>>
SharedLRUCache<T>::Del(const std::string &key) {
  const size_t slot = ShardHash(key) & (kNumShards - 1);
  return shard_[slot].Del(key);
}
template<typename T>
size_t SharedLRUCache<T>::ShardHash(const std::string &key) {
  size_t hash = 0;
  for (char ch : key) {
    hash = hash * 101 + ch;
  }
  return hash;
}

}  // namespace impl

template<typename T>
std::unique_ptr<Cache<T>> NewLRUCache(size_t capacity) {
  return std::unique_ptr<Cache<T>>(new impl::SharedLRUCache<T>(capacity));
}

}  // namespace yaldb

#endif  // YALDB_CACHE_H_
