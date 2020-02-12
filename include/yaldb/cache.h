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
#include <vector>

#include "yaldb/thread_annotation.h"

namespace yaldb {

template<typename T>
class Cache;

template<typename T>
std::unique_ptr<Cache<T>> NewLRUCache(size_t capacity);

template<typename T>
class Cache {
 public:
  using PairType = std::pair<std::string, T>;
  using PairPtr = std::shared_ptr<PairType>;
  using DeleterType = std::function<void(PairType *)>;

  Cache() : deleter_(std::default_delete<PairType>()) {}
  explicit Cache(DeleterType deleter) :
      deleter_(deleter) {}
  virtual ~Cache() = default;
  virtual void Put(const std::string &key, T value) = 0;
  [[nodiscard]] virtual PairPtr Get(const std::string &key) = 0;
  [[nodiscard]] virtual PairPtr Del(const std::string &key) = 0;
 protected:
  DeleterType deleter_;
};

namespace impl {

template<typename T>
class LRUCache : public Cache<T> {
 public:
  using PairType = typename Cache<T>::PairType;
  using PairPtr = typename Cache<T>::PairPtr;
  using DeleterType = typename Cache<T>::DeleterType;

  LRUCache() : Cache<T>(), capacity_() {}
  explicit LRUCache(size_t capacity) : Cache<T>(), capacity_(capacity) {}
//  explicit LRUCache(DeleterType deleter) : Cache<T>(deleter), capacity_() {}
  LRUCache(size_t capacity, DeleterType deleter) :
      Cache<T>(deleter), capacity_(capacity) {}
  ~LRUCache() override = default;
  void Put(const std::string &key, T value) override;
  PairPtr Get(const std::string &key) override;
  PairPtr Del(const std::string &key) override;
  void set_capacity(size_t capacity) { capacity_ = capacity; }

 private:
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
//  list_.push_front(std::make_shared<PairType>(
//      std::make_pair(key, std::move(value))));
  auto *pair = new PairType(key, std::move(value));
  list_.push_front(PairPtr(pair, this->deleter_));
  // update / insert
  map_.insert_or_assign(key, list_.begin());

  assert(list_.size() == map_.size());
  if (list_.size() > capacity_) {
    // remove oldest item in the cache
//    auto back = std::prev(list_.end());
    typename ListType::iterator back;
    for (back = std::prev(list_.end());
        back->use_count() != 1 && back != list_.begin(); --back) {}
    if (back->use_count() != 1) return;
    [[maybe_unused]] auto back_slot = map_.find((*back)->first);
    assert(back_slot != map_.end() && back_slot->second == back);
    map_.erase(back_slot);
//    list_.pop_back();
    list_.erase(back);
  }
}
template<typename T>
typename LRUCache<T>::PairPtr
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
typename LRUCache<T>::PairPtr
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
  using PairType = typename Cache<T>::PairType;
  using PairPtr = typename Cache<T>::PairPtr;
  using DeleterType = typename Cache<T>::DeleterType;

  explicit SharedLRUCache(size_t capacity);
  SharedLRUCache(size_t capacity, DeleterType deleter);
  ~SharedLRUCache() override = default;
  void Put(const std::string &key, T value) override;
  PairPtr Get(const std::string &key) override;
  PairPtr Del(const std::string &key) override;
 private:
  static constexpr size_t kNumShardBits = 4u;
  static constexpr size_t kNumShards = 1u << kNumShardBits;

  static size_t ShardHash(const std::string &key);

  size_t capacity_;
  std::vector<LRUCache<T>> shard_;
};
template<typename T>
SharedLRUCache<T>::SharedLRUCache(size_t capacity) :
    capacity_(capacity) {
  const size_t shard_capacity = (capacity + kNumShards - 1) / kNumShards;
  shard_.resize(kNumShards, LRUCache<T>(shard_capacity));
}
template<typename T>
SharedLRUCache<T>::SharedLRUCache(size_t capacity, DeleterType deleter) :
    capacity_(capacity) {
  const size_t shard_capacity = (capacity + kNumShards - 1) / kNumShards;
  shard_.resize(kNumShards, LRUCache<T>(shard_capacity, deleter));
}
template<typename T>
void SharedLRUCache<T>::Put(const std::string &key, T value) {
  const size_t slot = ShardHash(key) & (kNumShards - 1);
  shard_[slot].Put(key, std::move(value));
}
template<typename T>
typename SharedLRUCache<T>::PairPtr
SharedLRUCache<T>::Get(const std::string &key) {
  const size_t slot = ShardHash(key) & (kNumShards - 1);
  return shard_[slot].Get(key);
}
template<typename T>
typename SharedLRUCache<T>::PairPtr
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
