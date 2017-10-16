/*
 Mock allocator.

 Copyright (c) 2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FMT_MOCK_ALLOCATOR_H_
#define FMT_MOCK_ALLOCATOR_H_

#include "gmock/gmock.h"

template <typename T>
class MockAllocator {
 public:
  MockAllocator() {}
  MockAllocator(const MockAllocator &) {}
  typedef T value_type;
  MOCK_METHOD2_T(allocate, T *(std::size_t n, const void *h));
  MOCK_METHOD2_T(deallocate, void (T *p, std::size_t n));
};

template <typename Allocator>
class AllocatorRef {
 private:
  Allocator *alloc_;

 public:
  typedef typename Allocator::value_type value_type;

  explicit AllocatorRef(Allocator *alloc = 0) : alloc_(alloc) {}

  AllocatorRef(const AllocatorRef &other) : alloc_(other.alloc_) {}

  AllocatorRef& operator=(const AllocatorRef &other) {
    alloc_ = other.alloc_;
    return *this;
  }

#if FMT_USE_RVALUE_REFERENCES
 private:
  void move(AllocatorRef &other) {
    alloc_ = other.alloc_;
    other.alloc_ = 0;
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

  value_type *allocate(std::size_t n,  const void *h) {
#if FMT_USE_ALLOCATOR_TRAITS
    return std::allocator_traits<Allocator>::allocate(*alloc_, n, h);
#else
    return alloc_->allocate(n, h);
#endif
  }
  void deallocate(value_type *p, std::size_t n) { alloc_->deallocate(p, n); }
};

#endif  // FMT_MOCK_ALLOCATOR_H_
