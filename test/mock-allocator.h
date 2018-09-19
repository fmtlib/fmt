// Formatting library for C++ - mock allocator
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_MOCK_ALLOCATOR_H_
#define FMT_MOCK_ALLOCATOR_H_

#include "gmock.h"
#include "fmt/format.h"

template <typename T>
class mock_allocator {
 public:
  mock_allocator() {}
  mock_allocator(const mock_allocator &) {}
  typedef T value_type;
  MOCK_METHOD1_T(allocate, T* (std::size_t n));
  MOCK_METHOD2_T(deallocate, void (T* p, std::size_t n));
};

template <typename Allocator>
class allocator_ref {
 private:
  Allocator *alloc_;

  void move(allocator_ref &other) {
    alloc_ = other.alloc_;
    other.alloc_ = nullptr;
  }

 public:
  typedef typename Allocator::value_type value_type;

  explicit allocator_ref(Allocator *alloc = nullptr) : alloc_(alloc) {}

  allocator_ref(const allocator_ref &other) : alloc_(other.alloc_) {}
  allocator_ref(allocator_ref &&other) { move(other); }

  allocator_ref& operator=(allocator_ref &&other) {
    assert(this != &other);
    move(other);
    return *this;
  }

  allocator_ref& operator=(const allocator_ref &other) {
    alloc_ = other.alloc_;
    return *this;
  }

 public:
  Allocator *get() const { return alloc_; }

  value_type* allocate(std::size_t n) {
    return fmt::internal::allocate(*alloc_, n);
  }
  void deallocate(value_type* p, std::size_t n) { alloc_->deallocate(p, n); }
};

#endif  // FMT_MOCK_ALLOCATOR_H_
