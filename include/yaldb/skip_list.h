//
// Copyright [2020] <inhzus>
//

#ifndef YALDB_SKIP_LIST_H_
#define YALDB_SKIP_LIST_H_

#include <cassert>
#include <ctime>
#include <functional>
#include <random>
#include <memory>
#include <utility>

namespace yaldb {

namespace impl {

template<typename T>
struct SkipListNode {
  T value;
  SkipListNode *back;
  size_t level;
  struct NodeLink {
    SkipListNode *n;
    NodeLink() : n(nullptr) {}
  } links[1];

  static void *operator new(const std::size_t size) {
    return ::operator new(size);
  }
  static void operator delete(void *ptr) {
    ::operator delete(ptr);
  }
  static void *operator new(const std::size_t size, const size_t level) {
    assert(level > 0);
    return ::operator new(size + (level - 1) * sizeof(NodeLink));
  }
  static void operator delete(void *ptr, const size_t) {
    return ::operator delete(ptr);
  }
  SkipListNode(T value, const size_t level, SkipListNode *back) :
      value(std::move(value)), back(back), level(level) {
    std::uninitialized_fill_n(links, level, NodeLink());
  }
};

template<typename T>
class SkipListIterator {
 private:
  SkipListNode<std::remove_const_t<T>> *node_;

 public:
  explicit SkipListIterator(SkipListNode<std::remove_const_t<T>> *node)
      : node_(node) {}
  T &operator*() { return node_->value; }
  const T &operator*() const { return node_->value; }
  T *operator->() { return &node_->value; }
  const T *operator->() const { return &node_->value; }
  SkipListIterator &operator++() {
    node_ = node_->links[0].n;
    return *this;
  }
  SkipListIterator &operator--() {
    node_ = node_->back;
    return *this;
  }
  SkipListIterator operator++(int) {  // NOLINT
    SkipListIterator it(*this);
    ++(*this);
    return it;
  }
  SkipListIterator operator--(int) {  // NOLINT
    SkipListIterator it(*this);
    --(*this);
    return it;
  }
  template<typename U>
  bool operator==(const SkipListIterator<U> &it) {
    return node_ == it.node_;
  }
  template<typename U>
  bool operator!=(const SkipListIterator<U> &it) {
    return node_ != it.node_;
  }
};

}  // namespace impl

// template<typename T, bool(*Less)(T, T)>
template<typename T, typename Comp = std::less<T>>
class SkipList {
 public:
  using node_type = impl::SkipListNode<T>;
  using iterator = impl::SkipListIterator<T>;
  using const_iterator = impl::SkipListIterator<const T>;

 private:
  [[nodiscard]] size_t RandomLevel() const;

  static constexpr double kRandomRatio = 0.5;
  static constexpr size_t kMaxLevel = 32;

  Comp comp_;
  mutable std::mt19937 rand_gen_;
  size_t length_;
  node_type *head_;
  node_type *tail_;

 public:
  explicit SkipList(Comp comp = std::less<T>());  // NOLINT

  iterator begin() { return iterator(head_); }
  [[nodiscard]] const_iterator begin() const { return const_iterator(head_); }
  iterator end() { return iterator(tail_); }
  [[nodiscard]] const_iterator end() const { return const_iterator(tail_); }

  iterator Insert(T value);
};

template<typename T, typename Comp>
size_t SkipList<T, Comp>::RandomLevel() const {
  size_t level = 1;
  std::uniform_real_distribution<double> dis(0, 1);
  while (dis(rand_gen_) < kRandomRatio && level < kMaxLevel) {
    ++level;
  }
  return level;
}

template<typename T, typename Comp>
SkipList<T, Comp>::SkipList(Comp comp) :
    comp_(std::move(comp)), length_(0) {
  static_assert(std::is_invocable_v<Comp, const T &, const T &>);
  static_assert(std::is_same_v<
      bool, std::invoke_result_t<Comp, const T &, const T &>>);
  std::random_device rd;
  rand_gen_ = std::mt19937(rd());
  head_ = new(kMaxLevel)node_type(T(), kMaxLevel, nullptr);
  tail_ = new(kMaxLevel)node_type(T(), kMaxLevel, head_);
  for (size_t i = 0; i < kMaxLevel; ++i) {
    head_->links[i].n = tail_;
  }
}

template<typename T, typename Comp>
typename SkipList<T, Comp>::iterator
SkipList<T, Comp>::Insert(T value) {
  auto prev = std::make_unique<node_type *[]>(kMaxLevel);
  node_type *cur = head_;
  for (size_t i = kMaxLevel - 1; i != size_t() - 1; --i) {
    while (cur->links[i].n != tail_ && comp_(cur->links[i].n->value, value)) {
      cur = cur->links[i].n;
    }
    prev[i] = cur;
  }
  if (cur->links[0].n != tail_ &&
      !comp_(cur->links[0].n->value, value) &&
      !comp_(value, cur->links[0].n->value)) {
    return end();
  }
  size_t level = RandomLevel();
  auto *node = new(level)node_type(std::move(value), level, cur);
  for (size_t i = 0; i < level; ++i) {
    node->links[i].n = prev[i]->links[i].n;
    prev[i]->links[i].n = node;
  }
  node->links[0].n->back = cur;
  return iterator(node);
}

void StaticTest() {
  auto f = [](int, int) {
    return true;
  };
  SkipList<int, decltype(f)> s(f);
}

}  // namespace yaldb

#endif  // YALDB_SKIP_LIST_H_