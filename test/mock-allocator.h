// Formatting library for C++ - mock allocator
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_MOCK_ALLOCATOR_H_
#define FMT_MOCK_ALLOCATOR_H_

#include "gmock.h"

template <typename T>
class MockAllocator {
 public:
  MockAllocator() {}
  MockAllocator(const MockAllocator &) {}
  typedef T value_type;
  MOCK_METHOD1_T(allocate, T* (std::size_t n));
  MOCK_METHOD2_T(deallocate, void (T* p, std::size_t n));
};

template <typename Allocator>
class AllocatorRef {
 private:
  Allocator *alloc_;

 public:
  typedef typename Allocator::value_type value_type;

  explicit AllocatorRef(Allocator *alloc = nullptr) : alloc_(alloc) {}

  AllocatorRef(const AllocatorRef &other) : alloc_(other.alloc_) {}

  AllocatorRef& operator=(const AllocatorRef &other) {
    alloc_ = other.alloc_;
    return *this;
  }

#if FMT_USE_RVALUE_REFERENCES
 private:
  void move(AllocatorRef &other) {
    alloc_ = other.alloc_;
    other.alloc_ = nullptr;
  }

 public:
  AllocatorRef(AllocatorRef &&other) {
    move(other);
  }

  AllocatorRef& operator=(AllocatorRef &&other) {
    assert(this != &other);
    move(other);
    return *this;
  }
#endif

  Allocator *get() const { return alloc_; }

  value_type* allocate(std::size_t n) {
    return fmt::internal::allocate(*alloc_, n);
  }
  void deallocate(value_type* p, std::size_t n) { alloc_->deallocate(p, n); }
};

#endif  // FMT_MOCK_ALLOCATOR_H_
