#pragma once

#include <cstddef>
#include <iterator>

namespace HAPPY::Core {

// 1. The embedded node data
template <typename T>
struct IntrusiveNode {
  T* next_node = nullptr;
};

// 2. The iterable collection class
template <typename T>
class IntrusiveList {
 public:
  // --- Standard C++ Forward Iterator Implementation ---
  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit constexpr Iterator(pointer ptr) : current_(ptr) {}

    reference operator*() const { return *current_; }
    pointer operator->() const { return current_; }

    // Pre-increment (++it)
    Iterator& operator++() {
      if (current_) current_ = current_->next_node;
      return *this;
    }

    // Post-increment (it++)
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.current_ == b.current_;
    }

    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return a.current_ != b.current_;
    }

   private:
    pointer current_;
  };

  // --- Collection API ---

  // O(1) tail insertion. Zero memory allocation.
  void push_back(T* item) {
    item->next_node = nullptr;
    if (!head_) {
      head_ = tail_ = item;
    } else {
      tail_->next_node = item;
      tail_ = item;
    }
  }

  // Range-based for-loop support
  Iterator begin() const { return Iterator(head_); }
  Iterator end() const { return Iterator(nullptr); }

  bool empty() const { return head_ == nullptr; }

 private:
  T* head_ = nullptr;
  T* tail_ = nullptr;
};

}  // namespace HAPPY::Core