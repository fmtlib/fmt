/*
 Formatting library for C++

 Copyright (c) 2012 - 2016, Victor Zverovich
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

#ifndef FMT_FORMAT_H_
#define FMT_FORMAT_H_

#include <array>
#include <cassert>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <utility>

#ifdef _SECURE_SCL
# define FMT_SECURE_SCL _SECURE_SCL
#else
# define FMT_SECURE_SCL 0
#endif

#if FMT_SECURE_SCL
# include <iterator>
#endif

#ifdef _MSC_VER
# define FMT_MSC_VER _MSC_VER
#else
# define FMT_MSC_VER 0
#endif

#if FMT_MSC_VER && FMT_MSC_VER <= 1500
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int64          intmax_t;
#else
#include <stdint.h>
#endif

#if !defined(FMT_HEADER_ONLY) && defined(_WIN32)
# ifdef FMT_EXPORT
#  define FMT_API __declspec(dllexport)
# elif defined(FMT_SHARED)
#  define FMT_API __declspec(dllimport)
# endif
#endif
#ifndef FMT_API
# define FMT_API
#endif

#ifdef __GNUC__
# define FMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
# define FMT_GCC_EXTENSION __extension__
# if FMT_GCC_VERSION >= 406
#  pragma GCC diagnostic push
// Disable the warning about "long long" which is sometimes reported even
// when using __extension__.
#  pragma GCC diagnostic ignored "-Wlong-long"
// Disable the warning about declaration shadowing because it affects too
// many valid cases.
#  pragma GCC diagnostic ignored "-Wshadow"
// Disable the warning about implicit conversions that may change the sign of
// an integer; silencing it otherwise would require many explicit casts.
#  pragma GCC diagnostic ignored "-Wsign-conversion"
# endif
# if __cplusplus >= 201103L || defined __GXX_EXPERIMENTAL_CXX0X__
#  define FMT_HAS_GXX_CXX11 1
# endif
#else
# define FMT_GCC_EXTENSION
#endif

#if defined(__INTEL_COMPILER)
# define FMT_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
# define FMT_ICC_VERSION __ICL
#endif

#if defined(__clang__) && !defined(FMT_ICC_VERSION)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
# pragma clang diagnostic ignored "-Wpadded"
#endif

#ifdef __GNUC_LIBSTD__
# define FMT_GNUC_LIBSTD_VERSION (__GNUC_LIBSTD__ * 100 + __GNUC_LIBSTD_MINOR__)
#endif

#ifdef __has_feature
# define FMT_HAS_FEATURE(x) __has_feature(x)
#else
# define FMT_HAS_FEATURE(x) 0
#endif

#ifdef __has_builtin
# define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
# define FMT_HAS_BUILTIN(x) 0
#endif

#ifdef __has_cpp_attribute
# define FMT_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define FMT_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifndef FMT_USE_RVALUE_REFERENCES
// Don't use rvalue references when compiling with clang and an old libstdc++
// as the latter doesn't provide std::move.
# if defined(FMT_GNUC_LIBSTD_VERSION) && FMT_GNUC_LIBSTD_VERSION <= 402
#  define FMT_USE_RVALUE_REFERENCES 0
# else
#  define FMT_USE_RVALUE_REFERENCES \
    (FMT_HAS_FEATURE(cxx_rvalue_references) || \
        (FMT_GCC_VERSION >= 403 && FMT_HAS_GXX_CXX11) || FMT_MSC_VER >= 1600)
# endif
#endif

#if FMT_USE_RVALUE_REFERENCES
# include <utility>  // for std::move
#endif

// Check if exceptions are disabled.
#if defined(__GNUC__) && !defined(__EXCEPTIONS)
# define FMT_EXCEPTIONS 0
#endif
#if FMT_MSC_VER && !_HAS_EXCEPTIONS
# define FMT_EXCEPTIONS 0
#endif
#ifndef FMT_EXCEPTIONS
# define FMT_EXCEPTIONS 1
#endif

#ifndef FMT_THROW
# if FMT_EXCEPTIONS
#  define FMT_THROW(x) throw x
# else
#  define FMT_THROW(x) assert(false)
# endif
#endif

// Define FMT_USE_NOEXCEPT to make fmt use noexcept (C++11 feature).
#ifndef FMT_USE_NOEXCEPT
# define FMT_USE_NOEXCEPT 0
#endif

#ifndef FMT_NOEXCEPT
# if FMT_EXCEPTIONS
#  if FMT_USE_NOEXCEPT || FMT_HAS_FEATURE(cxx_noexcept) || \
    (FMT_GCC_VERSION >= 408 && FMT_HAS_GXX_CXX11) || \
    FMT_MSC_VER >= 1900
#   define FMT_NOEXCEPT noexcept
#  else
#   define FMT_NOEXCEPT throw()
#  endif
# else
#  define FMT_NOEXCEPT
# endif
#endif

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#ifndef FMT_USE_DELETED_FUNCTIONS
# define FMT_USE_DELETED_FUNCTIONS 0
#endif

#if FMT_USE_DELETED_FUNCTIONS || FMT_HAS_FEATURE(cxx_deleted_functions) || \
  (FMT_GCC_VERSION >= 404 && FMT_HAS_GXX_CXX11) || FMT_MSC_VER >= 1800
# define FMT_DELETED_OR_UNDEFINED = delete
# define FMT_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName& operator=(const TypeName&) = delete
#else
# define FMT_DELETED_OR_UNDEFINED
# define FMT_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    TypeName& operator=(const TypeName&)
#endif

#ifndef FMT_USE_USER_DEFINED_LITERALS
// All compilers which support UDLs also support variadic templates. This
// makes the fmt::literals implementation easier. However, an explicit check
// for variadic templates is added here just in case.
// For Intel's compiler both it and the system gcc/msc must support UDLs.
# define FMT_USE_USER_DEFINED_LITERALS \
   FMT_USE_RVALUE_REFERENCES && \
   (FMT_HAS_FEATURE(cxx_user_literals) || \
     (FMT_GCC_VERSION >= 407 && FMT_HAS_GXX_CXX11) || FMT_MSC_VER >= 1900) && \
   (!defined(FMT_ICC_VERSION) || FMT_ICC_VERSION >= 1500)
#endif

#ifndef FMT_ASSERT
# define FMT_ASSERT(condition, message) assert((condition) && message)
#endif

#if FMT_GCC_VERSION >= 400 || FMT_HAS_BUILTIN(__builtin_clz)
# define FMT_BUILTIN_CLZ(n) __builtin_clz(n)
#endif

#if FMT_GCC_VERSION >= 400 || FMT_HAS_BUILTIN(__builtin_clzll)
# define FMT_BUILTIN_CLZLL(n) __builtin_clzll(n)
#endif

// Some compilers masquerade as both MSVC and GCC-likes or otherwise support
// __builtin_clz and __builtin_clzll, so only define FMT_BUILTIN_CLZ using the
// MSVC intrinsics if the clz and clzll builtins are not available.
#if FMT_MSC_VER && !defined(FMT_BUILTIN_CLZLL)
# include <intrin.h>  // _BitScanReverse, _BitScanReverse64

namespace fmt {
namespace internal {
# pragma intrinsic(_BitScanReverse)
inline uint32_t clz(uint32_t x) {
  unsigned long r = 0;
  _BitScanReverse(&r, x);

  assert(x != 0);
  // Static analysis complains about using uninitialized data
  // "r", but the only way that can happen is if "x" is 0,
  // which the callers guarantee to not happen.
# pragma warning(suppress: 6102)
  return 31 - r;
}
# define FMT_BUILTIN_CLZ(n) fmt::internal::clz(n)

# ifdef _WIN64
#  pragma intrinsic(_BitScanReverse64)
# endif

inline uint32_t clzll(uint64_t x) {
  unsigned long r = 0;
# ifdef _WIN64
  _BitScanReverse64(&r, x);
# else
  // Scan the high 32 bits.
  if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32)))
    return 63 - (r + 32);

  // Scan the low 32 bits.
  _BitScanReverse(&r, static_cast<uint32_t>(x));
# endif

  assert(x != 0);
  // Static analysis complains about using uninitialized data
  // "r", but the only way that can happen is if "x" is 0,
  // which the callers guarantee to not happen.
# pragma warning(suppress: 6102)
  return 63 - r;
}
# define FMT_BUILTIN_CLZLL(n) fmt::internal::clzll(n)
}
}
#endif

namespace fmt {
namespace internal {
struct dummy_int {
  int data[2];
  operator int() const { return 0; }
};
typedef std::numeric_limits<fmt::internal::dummy_int> fputil;

// Dummy implementations of system functions such as signbit and ecvt called
// if the latter are not available.
inline dummy_int signbit(...) { return dummy_int(); }
inline dummy_int _ecvt_s(...) { return dummy_int(); }
inline dummy_int isinf(...) { return dummy_int(); }
inline dummy_int _finite(...) { return dummy_int(); }
inline dummy_int isnan(...) { return dummy_int(); }
inline dummy_int _isnan(...) { return dummy_int(); }

// A helper function to suppress bogus "conditional expression is constant"
// warnings.
template <typename T>
inline T const_check(T value) { return value; }
}
}  // namespace fmt

namespace std {
// Standard permits specialization of std::numeric_limits. This specialization
// is used to resolve ambiguity between isinf and std::isinf in glibc:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=48891
// and the same for isnan and signbit.
template <>
class numeric_limits<fmt::internal::dummy_int> :
    public std::numeric_limits<int> {
 public:
  // Portable version of isinf.
  template <typename T>
  static bool isinfinity(T x) {
    using namespace fmt::internal;
    // The resolution "priority" is:
    // isinf macro > std::isinf > ::isinf > fmt::internal::isinf
    if (const_check(sizeof(isinf(x)) == sizeof(bool) ||
                    sizeof(isinf(x)) == sizeof(int))) {
      return isinf(x) != 0;
    }
    return !_finite(static_cast<double>(x));
  }

  // Portable version of isnan.
  template <typename T>
  static bool isnotanumber(T x) {
    using namespace fmt::internal;
    if (const_check(sizeof(isnan(x)) == sizeof(bool) ||
                    sizeof(isnan(x)) == sizeof(int))) {
      return isnan(x) != 0;
    }
    return _isnan(static_cast<double>(x)) != 0;
  }

  // Portable version of signbit.
  static bool isnegative(double x) {
    using namespace fmt::internal;
    if (const_check(sizeof(signbit(x)) == sizeof(int)))
      return signbit(x) != 0;
    if (x < 0) return true;
    if (!isnotanumber(x)) return false;
    int dec = 0, sign = 0;
    char buffer[2];  // The buffer size must be >= 2 or _ecvt_s will fail.
    _ecvt_s(buffer, sizeof(buffer), x, 0, &dec, &sign);
    return sign != 0;
  }
};
}  // namespace std

namespace fmt {

// Fix the warning about long long on older versions of GCC
// that don't support the diagnostic pragma.
FMT_GCC_EXTENSION typedef long long LongLong;
FMT_GCC_EXTENSION typedef unsigned long long ULongLong;

#if FMT_USE_RVALUE_REFERENCES
using std::move;
#endif

template <typename Char>
class basic_writer;

typedef basic_writer<char> writer;
typedef basic_writer<wchar_t> wwriter;

template <typename Context>
class basic_format_arg;

template <typename Char>
class ArgFormatter;

template <typename Char>
class PrintfArgFormatter;

template <typename Char>
class basic_format_context;

typedef basic_format_context<char> format_context;
typedef basic_format_context<wchar_t> wformat_context;

/**
  \rst
  A string reference. It can be constructed from a C string or ``std::string``.

  You can use one of the following typedefs for common character types:

  +------------+-------------------------+
  | Type       | Definition              |
  +============+=========================+
  | StringRef  | BasicStringRef<char>    |
  +------------+-------------------------+
  | WStringRef | BasicStringRef<wchar_t> |
  +------------+-------------------------+

  This class is most useful as a parameter type to allow passing
  different types of strings to a function, for example::

    template <typename... Args>
    std::string format(StringRef format_str, const Args & ... args);

    format("{}", 42);
    format(std::string("{}"), 42);
  \endrst
 */
template <typename Char>
class BasicStringRef {
 private:
  const Char *data_;
  std::size_t size_;

 public:
  BasicStringRef() : data_(0), size_(0) {}

  /** Constructs a string reference object from a C string and a size. */
  BasicStringRef(const Char *s, std::size_t size) : data_(s), size_(size) {}

  /**
    \rst
    Constructs a string reference object from a C string computing
    the size with ``std::char_traits<Char>::length``.
    \endrst
   */
  BasicStringRef(const Char *s)
    : data_(s), size_(std::char_traits<Char>::length(s)) {}

  /**
    \rst
    Constructs a string reference from an ``std::string`` object.
    \endrst
   */
  BasicStringRef(const std::basic_string<Char> &s)
  : data_(s.c_str()), size_(s.size()) {}

  /**
    \rst
    Converts a string reference to an ``std::string`` object.
    \endrst
   */
  std::basic_string<Char> to_string() const {
    return std::basic_string<Char>(data_, size_);
  }

  /** Returns a pointer to the string data. */
  const Char *data() const { return data_; }

  /** Returns the string size. */
  std::size_t size() const { return size_; }

  // Lexicographically compare this string reference to other.
  int compare(BasicStringRef other) const {
    std::size_t size = size_ < other.size_ ? size_ : other.size_;
    int result = std::char_traits<Char>::compare(data_, other.data_, size);
    if (result == 0)
      result = size_ == other.size_ ? 0 : (size_ < other.size_ ? -1 : 1);
    return result;
  }

  friend bool operator==(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) == 0;
  }
  friend bool operator!=(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) != 0;
  }
  friend bool operator<(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) < 0;
  }
  friend bool operator<=(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) <= 0;
  }
  friend bool operator>(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) > 0;
  }
  friend bool operator>=(BasicStringRef lhs, BasicStringRef rhs) {
    return lhs.compare(rhs) >= 0;
  }
};

typedef BasicStringRef<char> StringRef;
typedef BasicStringRef<wchar_t> WStringRef;

/**
  \rst
  A reference to a null terminated string. It can be constructed from a C
  string or ``std::string``.

  You can use one of the following typedefs for common character types:

  +-------------+--------------------------+
  | Type        | Definition               |
  +=============+==========================+
  | CStringRef  | BasicCStringRef<char>    |
  +-------------+--------------------------+
  | WCStringRef | BasicCStringRef<wchar_t> |
  +-------------+--------------------------+

  This class is most useful as a parameter type to allow passing
  different types of strings to a function, for example::

    template <typename... Args>
    std::string format(CStringRef format_str, const Args & ... args);

    format("{}", 42);
    format(std::string("{}"), 42);
  \endrst
 */
template <typename Char>
class BasicCStringRef {
 private:
  const Char *data_;

 public:
  /** Constructs a string reference object from a C string. */
  BasicCStringRef(const Char *s) : data_(s) {}

  /**
    \rst
    Constructs a string reference from an ``std::string`` object.
    \endrst
   */
  BasicCStringRef(const std::basic_string<Char> &s) : data_(s.c_str()) {}

  /** Returns the pointer to a C string. */
  const Char *c_str() const { return data_; }
};

typedef BasicCStringRef<char> CStringRef;
typedef BasicCStringRef<wchar_t> WCStringRef;

/** A formatting error such as invalid format string. */
class format_error : public std::runtime_error {
 public:
  explicit format_error(CStringRef message)
  : std::runtime_error(message.c_str()) {}
  ~format_error() throw();
};

namespace internal {

// MakeUnsigned<T>::Type gives an unsigned type corresponding to integer type T.
template <typename T>
struct MakeUnsigned { typedef T Type; };

#define FMT_SPECIALIZE_MAKE_UNSIGNED(T, U) \
  template <> \
  struct MakeUnsigned<T> { typedef U Type; }

FMT_SPECIALIZE_MAKE_UNSIGNED(char, unsigned char);
FMT_SPECIALIZE_MAKE_UNSIGNED(signed char, unsigned char);
FMT_SPECIALIZE_MAKE_UNSIGNED(short, unsigned short);
FMT_SPECIALIZE_MAKE_UNSIGNED(int, unsigned);
FMT_SPECIALIZE_MAKE_UNSIGNED(long, unsigned long);
FMT_SPECIALIZE_MAKE_UNSIGNED(LongLong, ULongLong);

// Casts nonnegative integer to unsigned.
template <typename Int>
inline typename MakeUnsigned<Int>::Type to_unsigned(Int value) {
  FMT_ASSERT(value >= 0, "negative value");
  return static_cast<typename MakeUnsigned<Int>::Type>(value);
}

// The number of characters to store in the MemoryBuffer object itself
// to avoid dynamic memory allocation.
enum { INLINE_BUFFER_SIZE = 500 };

#if FMT_SECURE_SCL
// Use checked iterator to avoid warnings on MSVC.
template <typename T>
inline stdext::checked_array_iterator<T*> make_ptr(T *ptr, std::size_t size) {
  return stdext::checked_array_iterator<T*>(ptr, size);
}
#else
template <typename T>
inline T *make_ptr(T *ptr, std::size_t) { return ptr; }
#endif
}  // namespace internal

/**
  \rst
  A buffer supporting a subset of ``std::vector``'s operations.
  \endrst
 */
template <typename T>
class Buffer {
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(Buffer);

 protected:
  T *ptr_;
  std::size_t size_;
  std::size_t capacity_;

  Buffer(T *ptr = 0, std::size_t capacity = 0)
    : ptr_(ptr), size_(0), capacity_(capacity) {}

  /**
    \rst
    Increases the buffer capacity to hold at least *size* elements updating
    ``ptr_`` and ``capacity_``.
    \endrst
   */
  virtual void grow(std::size_t size) = 0;

 public:
  virtual ~Buffer() {}

  /** Returns the size of this buffer. */
  std::size_t size() const { return size_; }

  /** Returns the capacity of this buffer. */
  std::size_t capacity() const { return capacity_; }

  /**
    Resizes the buffer. If T is a POD type new elements may not be initialized.
   */
  void resize(std::size_t new_size) {
    if (new_size > capacity_)
      grow(new_size);
    size_ = new_size;
  }

  /**
    \rst
    Reserves space to store at least *capacity* elements.
    \endrst
   */
  void reserve(std::size_t capacity) {
    if (capacity > capacity_)
      grow(capacity);
  }

  void clear() FMT_NOEXCEPT { size_ = 0; }

  void push_back(const T &value) {
    if (size_ == capacity_)
      grow(size_ + 1);
    ptr_[size_++] = value;
  }

  /** Appends data to the end of the buffer. */
  template <typename U>
  void append(const U *begin, const U *end);

  T &operator[](std::size_t index) { return ptr_[index]; }
  const T &operator[](std::size_t index) const { return ptr_[index]; }
};

template <typename T>
template <typename U>
void Buffer<T>::append(const U *begin, const U *end) {
  std::size_t new_size = size_ + internal::to_unsigned(end - begin);
  if (new_size > capacity_)
    grow(new_size);
  std::uninitialized_copy(begin, end,
                          internal::make_ptr(ptr_, capacity_) + size_);
  size_ = new_size;
}

namespace internal {

// A memory buffer for trivially copyable/constructible types with the first
// SIZE elements stored in the object itself.
template <typename T, std::size_t SIZE, typename Allocator = std::allocator<T> >
class MemoryBuffer : private Allocator, public Buffer<T> {
 private:
  T data_[SIZE];

  // Deallocate memory allocated by the buffer.
  void deallocate() {
    if (this->ptr_ != data_) Allocator::deallocate(this->ptr_, this->capacity_);
  }

 protected:
  void grow(std::size_t size);

 public:
  explicit MemoryBuffer(const Allocator &alloc = Allocator())
      : Allocator(alloc), Buffer<T>(data_, SIZE) {}
  ~MemoryBuffer() { deallocate(); }

#if FMT_USE_RVALUE_REFERENCES
 private:
  // Move data from other to this buffer.
  void move(MemoryBuffer &other) {
    Allocator &this_alloc = *this, &other_alloc = other;
    this_alloc = std::move(other_alloc);
    this->size_ = other.size_;
    this->capacity_ = other.capacity_;
    if (other.ptr_ == other.data_) {
      this->ptr_ = data_;
      std::uninitialized_copy(other.data_, other.data_ + this->size_,
                              make_ptr(data_, this->capacity_));
    } else {
      this->ptr_ = other.ptr_;
      // Set pointer to the inline array so that delete is not called
      // when deallocating.
      other.ptr_ = other.data_;
    }
  }

 public:
  MemoryBuffer(MemoryBuffer &&other) {
    move(other);
  }

  MemoryBuffer &operator=(MemoryBuffer &&other) {
    assert(this != &other);
    deallocate();
    move(other);
    return *this;
  }
#endif

  // Returns a copy of the allocator associated with this buffer.
  Allocator get_allocator() const { return *this; }
};

template <typename T, std::size_t SIZE, typename Allocator>
void MemoryBuffer<T, SIZE, Allocator>::grow(std::size_t size) {
  std::size_t new_capacity = this->capacity_ + this->capacity_ / 2;
  if (size > new_capacity)
      new_capacity = size;
  T *new_ptr = this->allocate(new_capacity);
  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(this->ptr_, this->ptr_ + this->size_,
                          make_ptr(new_ptr, new_capacity));
  std::size_t old_capacity = this->capacity_;
  T *old_ptr = this->ptr_;
  this->capacity_ = new_capacity;
  this->ptr_ = new_ptr;
  // deallocate may throw (at least in principle), but it doesn't matter since
  // the buffer already uses the new storage and will deallocate it in case
  // of exception.
  if (old_ptr != data_)
    Allocator::deallocate(old_ptr, old_capacity);
}

// A fixed-size buffer.
template <typename Char>
class FixedBuffer : public fmt::Buffer<Char> {
 public:
  FixedBuffer(Char *array, std::size_t size) : fmt::Buffer<Char>(array, size) {}

 protected:
  FMT_API void grow(std::size_t size);
};

template <typename Char>
class BasicCharTraits {
 public:
#if FMT_SECURE_SCL
  typedef stdext::checked_array_iterator<Char*> CharPtr;
#else
  typedef Char *CharPtr;
#endif
  static Char cast(int value) { return static_cast<Char>(value); }
};

template <typename Char>
class CharTraits;

template <>
class CharTraits<char> : public BasicCharTraits<char> {
 private:
  // Conversion from wchar_t to char is not allowed.
  static char convert(wchar_t);

 public:
  static char convert(char value) { return value; }

  // Formats a floating-point number.
  template <typename T>
  FMT_API static int format_float(char *buffer, std::size_t size,
      const char *format, unsigned width, int precision, T value);
};

template <>
class CharTraits<wchar_t> : public BasicCharTraits<wchar_t> {
 public:
  static wchar_t convert(char value) { return value; }
  static wchar_t convert(wchar_t value) { return value; }

  template <typename T>
  FMT_API static int format_float(wchar_t *buffer, std::size_t size,
      const wchar_t *format, unsigned width, int precision, T value);
};

// Checks if a number is negative - used to avoid warnings.
template <bool IsSigned>
struct SignChecker {
  template <typename T>
  static bool is_negative(T value) { return value < 0; }
};

template <>
struct SignChecker<false> {
  template <typename T>
  static bool is_negative(T) { return false; }
};

// Returns true if value is negative, false otherwise.
// Same as (value < 0) but doesn't produce warnings if T is an unsigned type.
template <typename T>
inline bool is_negative(T value) {
  return SignChecker<std::numeric_limits<T>::is_signed>::is_negative(value);
}

// Selects uint32_t if FitsIn32Bits is true, uint64_t otherwise.
template <bool FitsIn32Bits>
struct TypeSelector { typedef uint32_t Type; };

template <>
struct TypeSelector<false> { typedef uint64_t Type; };

template <typename T>
struct IntTraits {
  // Smallest of uint32_t and uint64_t that is large enough to represent
  // all values of T.
  typedef typename
    TypeSelector<std::numeric_limits<T>::digits <= 32>::Type MainType;
};

FMT_API void report_unknown_type(char code, const char *type);

// Static data is placed in this class template to allow header-only
// configuration.
template <typename T = void>
struct FMT_API BasicData {
  static const uint32_t POWERS_OF_10_32[];
  static const uint64_t POWERS_OF_10_64[];
  static const char DIGITS[];
};

#ifndef FMT_USE_EXTERN_TEMPLATES
// Clang doesn't have a feature check for extern templates so we check
// for variadic templates which were introduced in the same version.
# define FMT_USE_EXTERN_TEMPLATES (__clang__)
#endif

#if FMT_USE_EXTERN_TEMPLATES && !defined(FMT_HEADER_ONLY)
extern template struct BasicData<void>;
#endif

typedef BasicData<> Data;

#ifdef FMT_BUILTIN_CLZLL
// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case count_digits returns 1.
inline unsigned count_digits(uint64_t n) {
  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  int t = (64 - FMT_BUILTIN_CLZLL(n | 1)) * 1233 >> 12;
  return to_unsigned(t) - (n < Data::POWERS_OF_10_64[t]) + 1;
}
#else
// Fallback version of count_digits used when __builtin_clz is not available.
inline unsigned count_digits(uint64_t n) {
  unsigned count = 1;
  for (;;) {
    // Integer division is slow so do it for a group of four digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    if (n < 10) return count;
    if (n < 100) return count + 1;
    if (n < 1000) return count + 2;
    if (n < 10000) return count + 3;
    n /= 10000u;
    count += 4;
  }
}
#endif

#ifdef FMT_BUILTIN_CLZ
// Optional version of count_digits for better performance on 32-bit platforms.
inline unsigned count_digits(uint32_t n) {
  int t = (32 - FMT_BUILTIN_CLZ(n | 1)) * 1233 >> 12;
  return to_unsigned(t) - (n < Data::POWERS_OF_10_32[t]) + 1;
}
#endif

// A functor that doesn't add a thousands separator.
struct NoThousandsSep {
  template <typename Char>
  void operator()(Char *) {}
};

// A functor that adds a thousands separator.
class ThousandsSep {
 private:
  fmt::StringRef sep_;

  // Index of a decimal digit with the least significant digit having index 0.
  unsigned digit_index_;

 public:
  explicit ThousandsSep(fmt::StringRef sep) : sep_(sep), digit_index_(0) {}

  template <typename Char>
  void operator()(Char *&buffer) {
    if (++digit_index_ % 3 != 0)
      return;
    buffer -= sep_.size();
    std::uninitialized_copy(sep_.data(), sep_.data() + sep_.size(),
                            internal::make_ptr(buffer, sep_.size()));
  }
};

// Formats a decimal unsigned integer value writing into buffer.
// thousands_sep is a functor that is called after writing each char to
// add a thousands separator if necessary.
template <typename UInt, typename Char, typename ThousandsSep>
inline void format_decimal(Char *buffer, UInt value, unsigned num_digits,
                           ThousandsSep thousands_sep) {
  buffer += num_digits;
  while (value >= 100) {
    // Integer division is slow so do it for a group of two digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    unsigned index = static_cast<unsigned>((value % 100) * 2);
    value /= 100;
    *--buffer = Data::DIGITS[index + 1];
    thousands_sep(buffer);
    *--buffer = Data::DIGITS[index];
    thousands_sep(buffer);
  }
  if (value < 10) {
    *--buffer = static_cast<char>('0' + value);
    return;
  }
  unsigned index = static_cast<unsigned>(value * 2);
  *--buffer = Data::DIGITS[index + 1];
  thousands_sep(buffer);
  *--buffer = Data::DIGITS[index];
}

template <typename UInt, typename Char>
inline void format_decimal(Char *buffer, UInt value, unsigned num_digits) {
  return format_decimal(buffer, value, num_digits, NoThousandsSep());
}

#ifndef _WIN32
# define FMT_USE_WINDOWS_H 0
#elif !defined(FMT_USE_WINDOWS_H)
# define FMT_USE_WINDOWS_H 1
#endif

// Define FMT_USE_WINDOWS_H to 0 to disable use of windows.h.
// All the functionality that relies on it will be disabled too.
#if FMT_USE_WINDOWS_H
// A converter from UTF-8 to UTF-16.
// It is only provided for Windows since other systems support UTF-8 natively.
class UTF8ToUTF16 {
 private:
  MemoryBuffer<wchar_t, INLINE_BUFFER_SIZE> buffer_;

 public:
  FMT_API explicit UTF8ToUTF16(StringRef s);
  operator WStringRef() const { return WStringRef(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
  const wchar_t *c_str() const { return &buffer_[0]; }
  std::wstring str() const { return std::wstring(&buffer_[0], size()); }
};

// A converter from UTF-16 to UTF-8.
// It is only provided for Windows since other systems support UTF-8 natively.
class UTF16ToUTF8 {
 private:
  MemoryBuffer<char, INLINE_BUFFER_SIZE> buffer_;

 public:
  UTF16ToUTF8() {}
  FMT_API explicit UTF16ToUTF8(WStringRef s);
  operator StringRef() const { return StringRef(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
  const char *c_str() const { return &buffer_[0]; }
  std::string str() const { return std::string(&buffer_[0], size()); }

  // Performs conversion returning a system error code instead of
  // throwing exception on conversion error. This method may still throw
  // in case of memory allocation error.
  FMT_API int convert(WStringRef s);
};

FMT_API void format_windows_error(fmt::writer &out, int error_code,
                                  fmt::StringRef message) FMT_NOEXCEPT;
#endif

template<bool B, class T = void>
struct EnableIf {};

template<class T>
struct EnableIf<true, T> { typedef T type; };

template<bool B, class T, class F>
struct Conditional { typedef T type; };

template<class T, class F>
struct Conditional<false, T, F> { typedef F type; };

template <typename T>
struct False { enum { value = 0 }; };

template <typename T = void>
struct Null {};

// A helper class template to enable or disable overloads taking wide
// characters and strings in value's constructor.
template <typename T, typename Char>
struct WCharHelper {
  typedef Null<T> Supported;
  typedef T Unsupported;
};

template <typename T>
struct WCharHelper<T, wchar_t> {
  typedef T Supported;
  typedef Null<T> Unsupported;
};

typedef char Yes[1];
typedef char No[2];

template <typename T>
T &get();

// These are non-members to workaround an overload resolution bug in bcc32.
Yes &convert(fmt::ULongLong);
No &convert(...);

template<typename T, bool ENABLE_CONVERSION>
struct ConvertToIntImpl {
  enum { value = ENABLE_CONVERSION };
};

template<typename T, bool ENABLE_CONVERSION>
struct ConvertToIntImpl2 {
  enum { value = false };
};

template<typename T>
struct ConvertToIntImpl2<T, true> {
  enum {
    // Don't convert numeric types.
    value = ConvertToIntImpl<T, !std::numeric_limits<T>::is_specialized>::value
  };
};

template<typename T>
struct ConvertToInt {
  enum { enable_conversion = sizeof(convert(get<T>())) == sizeof(Yes) };
  enum { value = ConvertToIntImpl2<T, enable_conversion>::value };
};

#define FMT_DISABLE_CONVERSION_TO_INT(Type) \
  template <> \
  struct ConvertToInt<Type> {  enum { value = 0 }; }

// Silence warnings about convering float to int.
FMT_DISABLE_CONVERSION_TO_INT(float);
FMT_DISABLE_CONVERSION_TO_INT(double);
FMT_DISABLE_CONVERSION_TO_INT(long double);

enum Type {
  NONE, NAMED_ARG,
  // Integer types should go first,
  INT, UINT, LONG_LONG, ULONG_LONG, BOOL, CHAR, LAST_INTEGER_TYPE = CHAR,
  // followed by floating-point types.
  DOUBLE, LONG_DOUBLE, LAST_NUMERIC_TYPE = LONG_DOUBLE,
  CSTRING, STRING, TSTRING, POINTER, CUSTOM
};

template <typename Char>
struct string_value {
  const Char *value;
  std::size_t size;
};

template <typename Char>
struct CustomValue {
  typedef void (*FormatFunc)(
      basic_writer<Char> &writer, const void *arg, void *ctx);

  const void *value;
  FormatFunc format;
};

template <typename Char>
struct NamedArg;

template <typename T>
struct IsNamedArg : std::false_type {};

template <typename Char>
struct IsNamedArg< NamedArg<Char> > : std::true_type {};

template <typename T>
constexpr Type gettype() {
  return IsNamedArg<T>::value ?
        NAMED_ARG : (ConvertToInt<T>::value ? INT : CUSTOM);
}

template <> constexpr Type gettype<bool>() { return BOOL; }
template <> constexpr Type gettype<short>() { return INT; }
template <> constexpr Type gettype<unsigned short>() { return UINT; }
template <> constexpr Type gettype<int>() { return INT; }
template <> constexpr Type gettype<unsigned>() { return UINT; }
template <> constexpr Type gettype<long>() {
  return sizeof(long) == sizeof(int) ? INT : LONG_LONG;
}
template <> constexpr Type gettype<unsigned long>() {
  return sizeof(unsigned long) == sizeof(unsigned) ?
        UINT : ULONG_LONG;
}
template <> constexpr Type gettype<LongLong>() { return LONG_LONG; }
template <> constexpr Type gettype<ULongLong>() { return ULONG_LONG; }
template <> constexpr Type gettype<float>() { return DOUBLE; }
template <> constexpr Type gettype<double>() { return DOUBLE; }
template <> constexpr Type gettype<long double>() { return LONG_DOUBLE; }
template <> constexpr Type gettype<signed char>() { return INT; }
template <> constexpr Type gettype<unsigned char>() { return UINT; }
template <> constexpr Type gettype<char>() { return CHAR; }

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
template <> constexpr Type gettype<wchar_t>() { return CHAR; }
#endif

template <> constexpr Type gettype<char *>() { return CSTRING; }
template <> constexpr Type gettype<const char *>() { return CSTRING; }
template <> constexpr Type gettype<signed char *>() { return CSTRING; }
template <> constexpr Type gettype<const signed char *>() { return CSTRING; }
template <> constexpr Type gettype<unsigned char *>() { return CSTRING; }
template <> constexpr Type gettype<const unsigned char *>() { return CSTRING; }
template <> constexpr Type gettype<std::string>() { return STRING; }
template <> constexpr Type gettype<StringRef>() { return STRING; }
template <> constexpr Type gettype<CStringRef>() { return CSTRING; }
template <> constexpr Type gettype<wchar_t *>() { return TSTRING; }
template <> constexpr Type gettype<const wchar_t *>() { return TSTRING; }
template <> constexpr Type gettype<std::wstring>() { return TSTRING; }
template <> constexpr Type gettype<WStringRef>() { return TSTRING; }
template <> constexpr Type gettype<void *>() { return POINTER; }
template <> constexpr Type gettype<const void *>() { return POINTER; }

template <typename T>
constexpr Type type() { return gettype<typename std::decay<T>::type>(); }

// A formatting argument value.
template <typename Context>
class value {
 public:
  union {
    int int_value;
    unsigned uint_value;
    LongLong long_long_value;
    ULongLong ulong_long_value;
    double double_value;
    long double long_double_value;
    const void *pointer;
    string_value<char> string;
    string_value<signed char> sstring;
    string_value<unsigned char> ustring;
    string_value<typename Context::char_type> tstring;
    CustomValue<typename Context::char_type> custom;
  };

  typedef typename Context::char_type Char;

 private:
  // The following two methods are private to disallow formatting of
  // arbitrary pointers. If you want to output a pointer cast it to
  // "void *" or "const void *". In particular, this forbids formatting
  // of "[const] volatile char *" which is printed as bool by iostreams.
  // Do not implement!
  template <typename T>
  value(const T *value);
  template <typename T>
  value(T *value);

  // The following methods are private to disallow formatting of wide
  // characters and strings into narrow strings as in
  //   fmt::format("{}", L"test");
  // To fix this, use a wide format string: fmt::format(L"{}", L"test").
#if !FMT_MSC_VER || defined(_NATIVE_WCHAR_T_DEFINED)
  value(typename WCharHelper<wchar_t, Char>::Unsupported);
#endif
  value(typename WCharHelper<wchar_t *, Char>::Unsupported);
  value(typename WCharHelper<const wchar_t *, Char>::Unsupported);
  value(typename WCharHelper<const std::wstring &, Char>::Unsupported);
  value(typename WCharHelper<WStringRef, Char>::Unsupported);

  void set_string(StringRef str) {
    this->string.value = str.data();
    this->string.size = str.size();
  }

  void set_string(WStringRef str) {
    this->tstring.value = str.data();
    this->tstring.size = str.size();
  }

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  static void format_custom_arg(
      basic_writer<Char> &writer, const void *arg, void *context) {
    format_value(writer, *static_cast<const T*>(arg),
                 *static_cast<Context*>(context));
  }

 public:
  value() {}

#define FMT_MAKE_VALUE_(Type, field, TYPE, rhs) \
  value(Type value) { \
    static_assert(internal::type<Type>() == internal::TYPE, "invalid type"); \
    this->field = rhs; \
  }

#define FMT_MAKE_VALUE(Type, field, TYPE) \
  FMT_MAKE_VALUE_(Type, field, TYPE, value)

  FMT_MAKE_VALUE(bool, int_value, BOOL)
  FMT_MAKE_VALUE(short, int_value, INT)
  FMT_MAKE_VALUE(unsigned short, uint_value, UINT)
  FMT_MAKE_VALUE(int, int_value, INT)
  FMT_MAKE_VALUE(unsigned, uint_value, UINT)

  value(long value) {
    // To minimize the number of types we need to deal with, long is
    // translated either to int or to long long depending on its size.
    if (const_check(sizeof(long) == sizeof(int)))
      this->int_value = static_cast<int>(value);
    else
      this->long_long_value = value;
  }

  value(unsigned long value) {
    if (const_check(sizeof(unsigned long) == sizeof(unsigned)))
      this->uint_value = static_cast<unsigned>(value);
    else
      this->ulong_long_value = value;
  }

  FMT_MAKE_VALUE(LongLong, long_long_value, LONG_LONG)
  FMT_MAKE_VALUE(ULongLong, ulong_long_value, ULONG_LONG)
  FMT_MAKE_VALUE(float, double_value, DOUBLE)
  FMT_MAKE_VALUE(double, double_value, DOUBLE)
  FMT_MAKE_VALUE(long double, long_double_value, LONG_DOUBLE)
  FMT_MAKE_VALUE(signed char, int_value, INT)
  FMT_MAKE_VALUE(unsigned char, uint_value, UINT)
  FMT_MAKE_VALUE(char, int_value, CHAR)

#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
  typedef typename WCharHelper<wchar_t, Char>::Supported WChar;
  value(WChar value) {
    static_assert(internal::type<WChar>() == internal::CHAR, "invalid type");
    this->int_value = value;
  }
#endif

#define FMT_MAKE_STR_VALUE(Type, TYPE) \
  value(Type value) { \
    static_assert(internal::type<Type>() == internal::TYPE, "invalid type"); \
    set_string(value); \
  }

  FMT_MAKE_VALUE(char *, string.value, CSTRING)
  FMT_MAKE_VALUE(const char *, string.value, CSTRING)
  FMT_MAKE_VALUE(signed char *, sstring.value, CSTRING)
  FMT_MAKE_VALUE(const signed char *, sstring.value, CSTRING)
  FMT_MAKE_VALUE(unsigned char *, ustring.value, CSTRING)
  FMT_MAKE_VALUE(const unsigned char *, ustring.value, CSTRING)
  FMT_MAKE_STR_VALUE(const std::string &, STRING)
  FMT_MAKE_STR_VALUE(StringRef, STRING)
  FMT_MAKE_VALUE_(CStringRef, string.value, CSTRING, value.c_str())

#define FMT_MAKE_WSTR_VALUE(Type, TYPE) \
  value(typename WCharHelper<Type, Char>::Supported value) { \
    static_assert(internal::type<Type>() == internal::TYPE, "invalid type"); \
    set_string(value); \
  }

  FMT_MAKE_WSTR_VALUE(wchar_t *, TSTRING)
  FMT_MAKE_WSTR_VALUE(const wchar_t *, TSTRING)
  FMT_MAKE_WSTR_VALUE(const std::wstring &, TSTRING)
  FMT_MAKE_WSTR_VALUE(WStringRef, TSTRING)

  FMT_MAKE_VALUE(void *, pointer, POINTER)
  FMT_MAKE_VALUE(const void *, pointer, POINTER)

  template <typename T>
  value(const T &value,
        typename EnableIf<!ConvertToInt<T>::value, int>::type = 0) {
    static_assert(internal::type<T>() == internal::CUSTOM, "invalid type");
    this->custom.value = &value;
    this->custom.format = &format_custom_arg<T>;
  }

  template <typename T>
  value(const T &value,
        typename EnableIf<ConvertToInt<T>::value, int>::type = 0) {
    static_assert(internal::type<T>() == internal::INT, "invalid type");
    this->int_value = value;
  }

  // Additional template param `Char_` is needed here because make_type always
  // uses char.
  template <typename Char_>
  value(const NamedArg<Char_> &value) {
    static_assert(
      internal::type<const NamedArg<Char_> &>() == internal::NAMED_ARG,
      "invalid type");
    this->pointer = &value;
  }
};

template <typename Context>
class ArgMap;

template <typename Context, typename T>
basic_format_arg<Context> make_arg(const T &value);
}  // namespace internal

struct monostate {};

template <typename Context>
class basic_format_args;

// A formatting argument. It is a trivially copyable/constructible type to
// allow storage in internal::MemoryBuffer.
template <typename Context>
class basic_format_arg {
 private:
  internal::value<Context> value_;
  internal::Type type_;

  template <typename ContextType, typename T>
  friend basic_format_arg<ContextType> internal::make_arg(const T &value);

  template <typename Visitor, typename Ctx>
  friend typename std::result_of<Visitor(int)>::type
    visit(Visitor &&vis, basic_format_arg<Ctx> arg);

  friend class basic_format_args<Context>;
  friend class internal::ArgMap<Context>;

 public:
  basic_format_arg() : type_(internal::NONE) {}

  explicit operator bool() const noexcept { return type_ != internal::NONE; }

  bool is_integral() const {
    FMT_ASSERT(type_ != internal::NAMED_ARG, "invalid argument type");
    return type_ > internal::NONE && type_ <= internal::LAST_INTEGER_TYPE;
  }

  bool is_numeric() const {
    FMT_ASSERT(type_ != internal::NAMED_ARG, "invalid argument type");
    return type_ > internal::NONE && type_ <= internal::LAST_NUMERIC_TYPE;
  }

  bool is_pointer() const {
    return type_ == internal::POINTER;
  }
};

typedef basic_format_arg<format_context> format_arg;
typedef basic_format_arg<wformat_context> wformat_arg;

/**
  \rst
  Visits an argument dispatching to the appropriate visit method based on
  the argument type. For example, if the argument type is ``double`` then
  ``vis(value)`` will be called with the value of type ``double``.
  \endrst
 */
template <typename Visitor, typename Context>
typename std::result_of<Visitor(int)>::type
    visit(Visitor &&vis, basic_format_arg<Context> arg) {
  typedef typename Context::char_type Char;
  switch (arg.type_) {
  case internal::NONE:
    return vis(monostate());
  case internal::NAMED_ARG:
    FMT_ASSERT(false, "invalid argument type");
    break;
  case internal::INT:
    return vis(arg.value_.int_value);
  case internal::UINT:
    return vis(arg.value_.uint_value);
  case internal::LONG_LONG:
    return vis(arg.value_.long_long_value);
  case internal::ULONG_LONG:
    return vis(arg.value_.ulong_long_value);
  case internal::BOOL:
    return vis(arg.value_.int_value != 0);
  case internal::CHAR:
    return vis(static_cast<Char>(arg.value_.int_value));
  case internal::DOUBLE:
    return vis(arg.value_.double_value);
  case internal::LONG_DOUBLE:
    return vis(arg.value_.long_double_value);
  case internal::CSTRING:
    return vis(arg.value_.string.value);
  case internal::STRING:
    return vis(StringRef(arg.value_.string.value, arg.value_.string.size));
  case internal::TSTRING:
    return vis(BasicStringRef<Char>(
                 arg.value_.tstring.value, arg.value_.tstring.size));
  case internal::POINTER:
    return vis(arg.value_.pointer);
  case internal::CUSTOM:
    return vis(arg.value_.custom);
  }
  return typename std::result_of<Visitor(int)>::type();
}

namespace internal {

template <typename Context, typename T>
basic_format_arg<Context> make_arg(const T &value) {
  basic_format_arg<Context> arg;
  arg.type_ = internal::type<T>();
  arg.value_ = value;
  return arg;
}

template <typename T, T> struct LConvCheck {
  LConvCheck(int) {}
};

// Returns the thousands separator for the current locale.
// We check if ``lconv`` contains ``thousands_sep`` because on Android
// ``lconv`` is stubbed as an empty struct.
template <typename LConv>
inline StringRef thousands_sep(
    LConv *lc, LConvCheck<char *LConv::*, &LConv::thousands_sep> = 0) {
  return lc->thousands_sep;
}

inline fmt::StringRef thousands_sep(...) { return ""; }

#define FMT_CONCAT(a, b) a##b

#if FMT_GCC_VERSION >= 407
# define FMT_UNUSED __attribute__((unused))
#else
# define FMT_UNUSED
#endif

#ifndef FMT_USE_STATIC_ASSERT
# define FMT_USE_STATIC_ASSERT 0
#endif

#if FMT_USE_STATIC_ASSERT || FMT_HAS_FEATURE(cxx_static_assert) || \
  (FMT_GCC_VERSION >= 403 && FMT_HAS_GXX_CXX11) || _MSC_VER >= 1600
# define FMT_STATIC_ASSERT(cond, message) static_assert(cond, message)
#else
# define FMT_CONCAT_(a, b) FMT_CONCAT(a, b)
# define FMT_STATIC_ASSERT(cond, message) \
  typedef int FMT_CONCAT_(Assert, __LINE__)[(cond) ? 1 : -1] FMT_UNUSED
#endif

template <typename Formatter, typename T, typename Char>
void format_value(basic_writer<Char> &, const T &, Formatter &, const Char *) {
  FMT_STATIC_ASSERT(False<T>::value,
                    "Cannot format argument. To enable the use of ostream "
                    "operator<< include fmt/ostream.h. Otherwise provide "
                    "an overload of format_arg.");
}

template <typename Context>
struct NamedArg : basic_format_arg<Context> {
  typedef typename Context::char_type Char;

  BasicStringRef<Char> name;

  template <typename T>
  NamedArg(BasicStringRef<Char> argname, const T &value)
  : basic_format_arg<Context>(make_arg<Context>(value)), name(argname) {}
};

class RuntimeError : public std::runtime_error {
 protected:
  RuntimeError() : std::runtime_error("") {}
  ~RuntimeError() throw();
};

template <typename Arg, typename... Args>
constexpr uint64_t make_type() {
  return type<Arg>() | (make_type<Args...>() << 4);
}

template <>
constexpr uint64_t make_type<void>() { return 0; }

// Maximum number of arguments with packed types.
enum { MAX_PACKED_ARGS = 16 };

template <bool IS_PACKED, typename Context, typename T>
inline typename std::enable_if<IS_PACKED, value<Context>>::type
    make_arg(const T& value) {
  return value;
}

template <bool IS_PACKED, typename Context, typename T>
inline typename std::enable_if<!IS_PACKED, basic_format_arg<Context>>::type
    make_arg(const T& value) {
  return make_arg<Context>(value);
}
}  // namespace internal

template <typename Context, typename ...Args>
class format_arg_store {
 private:
  static const size_t NUM_ARGS = sizeof...(Args);

  // Packed is a macro on MinGW so use IS_PACKED instead.
  static const bool IS_PACKED = NUM_ARGS < internal::MAX_PACKED_ARGS;

  typedef typename Context::char_type char_type;

  typedef typename std::conditional<IS_PACKED,
    internal::value<Context>, basic_format_arg<Context>>::type value_type;

  // If the arguments are not packed, add one more element to mark the end.
  typedef std::array<value_type, NUM_ARGS + (IS_PACKED ? 0 : 1)> Array;
  Array data_;

 public:
  static const uint64_t TYPES = internal::make_type<Args..., void>();

  format_arg_store(const Args &... args)
    : data_(Array{internal::make_arg<IS_PACKED, Context>(args)...}) {}

  const value_type *data() const { return data_.data(); }
};

template <typename Context, typename ...Args>
inline format_arg_store<Context, Args...>
    make_xformat_args(const Args & ... args) {
  return format_arg_store<Context, Args...>(args...);
}

template <typename ...Args>
inline format_arg_store<format_context, Args...>
    make_format_args(const Args & ... args) {
  return format_arg_store<format_context, Args...>(args...);
}

/** Formatting arguments. */
template <typename Context>
class basic_format_args {
 public:
  typedef unsigned size_type;
  typedef basic_format_arg<Context> format_arg;

 private:
  // To reduce compiled code size per formatting function call, types of first
  // MAX_PACKED_ARGS arguments are passed in the types_ field.
  uint64_t types_;
  union {
    // If the number of arguments is less than MAX_PACKED_ARGS, the argument
    // values are stored in values_, otherwise they are stored in args_.
    // This is done to reduce compiled code size as storing larger objects
    // may require more code (at least on x86-64) even if the same amount of
    // data is actually copied to stack. It saves ~10% on the bloat test.
    const internal::value<Context> *values_;
    const format_arg *args_;
  };

  typename internal::Type type(unsigned index) const {
    unsigned shift = index * 4;
    uint64_t mask = 0xf;
    return static_cast<typename internal::Type>(
      (types_ & (mask << shift)) >> shift);
  }

  friend class internal::ArgMap<Context>;

  void set_data(const internal::value<Context> *values) { values_ = values; }
  void set_data(const format_arg *args) { args_ = args; }

  format_arg get(size_type index) const {
    format_arg arg;
    bool use_values = type(internal::MAX_PACKED_ARGS - 1) == internal::NONE;
    if (index < internal::MAX_PACKED_ARGS) {
      typename internal::Type arg_type = type(index);
      internal::value<Context> &val = arg.value_;
      if (arg_type != internal::NONE)
        val = use_values ? values_[index] : args_[index].value_;
      arg.type_ = arg_type;
      return arg;
    }
    if (use_values) {
      // The index is greater than the number of arguments that can be stored
      // in values, so return a "none" argument.
      arg.type_ = internal::NONE;
      return arg;
    }
    for (unsigned i = internal::MAX_PACKED_ARGS; i <= index; ++i) {
      if (args_[i].type_ == internal::NONE)
        return args_[i];
    }
    return args_[index];
  }

 public:
  basic_format_args() : types_(0) {}

  template <typename... Args>
  basic_format_args(const format_arg_store<Context, Args...> &store)
  : types_(store.TYPES) {
    set_data(store.data());
  }

  /** Returns the argument at specified index. */
  format_arg operator[](size_type index) const {
    format_arg arg = get(index);
    return arg.type_ == internal::NAMED_ARG ?
      *static_cast<const format_arg*>(arg.value_.pointer) : arg;
  }
};

typedef basic_format_args<format_context> format_args;
typedef basic_format_args<wformat_context> wformat_args;

enum Alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

// Flags.
enum {
  SIGN_FLAG = 1, PLUS_FLAG = 2, MINUS_FLAG = 4, HASH_FLAG = 8,
  CHAR_FLAG = 0x10  // Argument has char type - used in error reporting.
};

// An empty format specifier.
struct EmptySpec {};

// A type specifier.
template <char TYPE>
struct TypeSpec : EmptySpec {
  Alignment align() const { return ALIGN_DEFAULT; }
  unsigned width() const { return 0; }
  int precision() const { return -1; }
  bool flag(unsigned) const { return false; }
  char type() const { return TYPE; }
  char fill() const { return ' '; }
};

// A width specifier.
struct WidthSpec {
  unsigned width_;
  // Fill is always wchar_t and cast to char if necessary to avoid having
  // two specialization of WidthSpec and its subclasses.
  wchar_t fill_;

  WidthSpec(unsigned width, wchar_t fill) : width_(width), fill_(fill) {}

  unsigned width() const { return width_; }
  wchar_t fill() const { return fill_; }
};

// An alignment specifier.
struct AlignSpec : WidthSpec {
  Alignment align_;

  AlignSpec(unsigned width, wchar_t fill, Alignment align = ALIGN_DEFAULT)
  : WidthSpec(width, fill), align_(align) {}

  Alignment align() const { return align_; }

  int precision() const { return -1; }
};

// An alignment and type specifier.
template <char TYPE>
struct AlignTypeSpec : AlignSpec {
  AlignTypeSpec(unsigned width, wchar_t fill) : AlignSpec(width, fill) {}

  bool flag(unsigned) const { return false; }
  char type() const { return TYPE; }
};

// A full format specifier.
struct FormatSpec : AlignSpec {
  unsigned flags_;
  int precision_;
  char type_;

  FormatSpec(
    unsigned width = 0, char type = 0, wchar_t fill = ' ')
  : AlignSpec(width, fill), flags_(0), precision_(-1), type_(type) {}

  bool flag(unsigned f) const { return (flags_ & f) != 0; }
  int precision() const { return precision_; }
  char type() const { return type_; }
};

// An integer format specifier.
template <typename T, typename SpecT = TypeSpec<0>, typename Char = char>
class IntFormatSpec : public SpecT {
 private:
  T value_;

 public:
  IntFormatSpec(T val, const SpecT &spec = SpecT())
  : SpecT(spec), value_(val) {}

  T value() const { return value_; }
};

// A string format specifier.
template <typename Char>
class StrFormatSpec : public AlignSpec {
 private:
  const Char *str_;

 public:
  template <typename FillChar>
  StrFormatSpec(const Char *str, unsigned width, FillChar fill)
  : AlignSpec(width, fill), str_(str) {
    internal::CharTraits<Char>::convert(FillChar());
  }

  const Char *str() const { return str_; }
};

/**
  Returns an integer format specifier to format the value in base 2.
 */
IntFormatSpec<int, TypeSpec<'b'> > bin(int value);

/**
  Returns an integer format specifier to format the value in base 8.
 */
IntFormatSpec<int, TypeSpec<'o'> > oct(int value);

/**
  Returns an integer format specifier to format the value in base 16 using
  lower-case letters for the digits above 9.
 */
IntFormatSpec<int, TypeSpec<'x'> > hex(int value);

/**
  Returns an integer formatter format specifier to format in base 16 using
  upper-case letters for the digits above 9.
 */
IntFormatSpec<int, TypeSpec<'X'> > hexu(int value);

/**
  \rst
  Returns an integer format specifier to pad the formatted argument with the
  fill character to the specified width using the default (right) numeric
  alignment.

  **Example**::

    MemoryWriter out;
    out << pad(hex(0xcafe), 8, '0');
    // out.str() == "0000cafe"

  \endrst
 */
template <char TYPE_CODE, typename Char>
IntFormatSpec<int, AlignTypeSpec<TYPE_CODE>, Char> pad(
    int value, unsigned width, Char fill = ' ');

#define FMT_DEFINE_INT_FORMATTERS(TYPE) \
inline IntFormatSpec<TYPE, TypeSpec<'b'> > bin(TYPE value) { \
  return IntFormatSpec<TYPE, TypeSpec<'b'> >(value, TypeSpec<'b'>()); \
} \
 \
inline IntFormatSpec<TYPE, TypeSpec<'o'> > oct(TYPE value) { \
  return IntFormatSpec<TYPE, TypeSpec<'o'> >(value, TypeSpec<'o'>()); \
} \
 \
inline IntFormatSpec<TYPE, TypeSpec<'x'> > hex(TYPE value) { \
  return IntFormatSpec<TYPE, TypeSpec<'x'> >(value, TypeSpec<'x'>()); \
} \
 \
inline IntFormatSpec<TYPE, TypeSpec<'X'> > hexu(TYPE value) { \
  return IntFormatSpec<TYPE, TypeSpec<'X'> >(value, TypeSpec<'X'>()); \
} \
 \
template <char TYPE_CODE> \
inline IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE> > pad( \
    IntFormatSpec<TYPE, TypeSpec<TYPE_CODE> > f, unsigned width) { \
  return IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE> >( \
      f.value(), AlignTypeSpec<TYPE_CODE>(width, ' ')); \
} \
 \
/* For compatibility with older compilers we provide two overloads for pad, */ \
/* one that takes a fill character and one that doesn't. In the future this */ \
/* can be replaced with one overload making the template argument Char      */ \
/* default to char (C++11). */ \
template <char TYPE_CODE, typename Char> \
inline IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE>, Char> pad( \
    IntFormatSpec<TYPE, TypeSpec<TYPE_CODE>, Char> f, \
    unsigned width, Char fill) { \
  return IntFormatSpec<TYPE, AlignTypeSpec<TYPE_CODE>, Char>( \
      f.value(), AlignTypeSpec<TYPE_CODE>(width, fill)); \
} \
 \
inline IntFormatSpec<TYPE, AlignTypeSpec<0> > pad( \
    TYPE value, unsigned width) { \
  return IntFormatSpec<TYPE, AlignTypeSpec<0> >( \
      value, AlignTypeSpec<0>(width, ' ')); \
} \
 \
template <typename Char> \
inline IntFormatSpec<TYPE, AlignTypeSpec<0>, Char> pad( \
   TYPE value, unsigned width, Char fill) { \
 return IntFormatSpec<TYPE, AlignTypeSpec<0>, Char>( \
     value, AlignTypeSpec<0>(width, fill)); \
}

FMT_DEFINE_INT_FORMATTERS(int)
FMT_DEFINE_INT_FORMATTERS(long)
FMT_DEFINE_INT_FORMATTERS(unsigned)
FMT_DEFINE_INT_FORMATTERS(unsigned long)
FMT_DEFINE_INT_FORMATTERS(LongLong)
FMT_DEFINE_INT_FORMATTERS(ULongLong)

/**
  \rst
  Returns a string formatter that pads the formatted argument with the fill
  character to the specified width using the default (left) string alignment.

  **Example**::

    std::string s = str(MemoryWriter() << pad("abc", 8));
    // s == "abc     "

  \endrst
 */
template <typename Char>
inline StrFormatSpec<Char> pad(
    const Char *str, unsigned width, Char fill = ' ') {
  return StrFormatSpec<Char>(str, width, fill);
}

inline StrFormatSpec<wchar_t> pad(
    const wchar_t *str, unsigned width, char fill = ' ') {
  return StrFormatSpec<wchar_t>(str, width, fill);
}

namespace internal {

template <typename Context>
class ArgMap {
 private:
  typedef typename Context::char_type Char;
  typedef std::vector<
    std::pair<fmt::BasicStringRef<Char>, basic_format_arg<Context> > > MapType;
  typedef typename MapType::value_type Pair;

  MapType map_;

 public:
  void init(const basic_format_args<Context> &args);

  const basic_format_arg<Context>
      *find(const fmt::BasicStringRef<Char> &name) const {
    // The list is unsorted, so just return the first matching name.
    for (typename MapType::const_iterator it = map_.begin(), end = map_.end();
         it != end; ++it) {
      if (it->first == name)
        return &it->second;
    }
    return 0;
  }
};

template <typename Context>
void ArgMap<Context>::init(const basic_format_args<Context> &args) {
  if (!map_.empty())
    return;
  typedef internal::NamedArg<Context> NamedArg;
  const NamedArg *named_arg = 0;
  bool use_values =
  args.type(MAX_PACKED_ARGS - 1) == internal::NONE;
  if (use_values) {
    for (unsigned i = 0;/*nothing*/; ++i) {
      internal::Type arg_type = args.type(i);
      switch (arg_type) {
        case internal::NONE:
          return;
        case internal::NAMED_ARG:
          named_arg = static_cast<const NamedArg*>(args.values_[i].pointer);
          map_.push_back(Pair(named_arg->name, *named_arg));
          break;
        default:
          /*nothing*/;
      }
    }
    return;
  }
  for (unsigned i = 0; i != MAX_PACKED_ARGS; ++i) {
    internal::Type arg_type = args.type(i);
    if (arg_type == internal::NAMED_ARG) {
      named_arg = static_cast<const NamedArg*>(args.args_[i].value_.pointer);
      map_.push_back(Pair(named_arg->name, *named_arg));
    }
  }
  for (unsigned i = MAX_PACKED_ARGS; ; ++i) {
    switch (args.args_[i].type_) {
      case internal::NONE:
        return;
      case internal::NAMED_ARG:
        named_arg = static_cast<const NamedArg*>(args.args_[i].value_.pointer);
        map_.push_back(Pair(named_arg->name, *named_arg));
        break;
      default:
        /*nothing*/;
    }
  }
}

template <typename Char>
class ArgFormatterBase {
 private:
  basic_writer<Char> &writer_;
  FormatSpec &spec_;

  FMT_DISALLOW_COPY_AND_ASSIGN(ArgFormatterBase);

  void write_pointer(const void *p) {
    spec_.flags_ = HASH_FLAG;
    spec_.type_ = 'x';
    writer_.write_int(reinterpret_cast<uintptr_t>(p), spec_);
  }

  template <typename StrChar>
  void write_str(BasicStringRef<StrChar> value,
                 typename EnableIf<
                   std::is_same<Char, wchar_t>::value &&
                   std::is_same<StrChar, wchar_t>::value, int>::type = 0) {
    writer_.write_str(value, spec_);
  }

  template <typename StrChar>
  void write_str(BasicStringRef<StrChar> value,
                 typename EnableIf<
                   !std::is_same<Char, wchar_t>::value ||
                   !std::is_same<StrChar, wchar_t>::value, int>::type = 0) {
    // Do nothing.
  }

 protected:
  basic_writer<Char> &writer() { return writer_; }
  FormatSpec &spec() { return spec_; }

  void write(bool value) {
    writer_.write_str(StringRef(value ? "true" : "false"), spec_);
  }

  void write(const char *value) {
    writer_.write_str(
          StringRef(value, value != 0 ? std::strlen(value) : 0), spec_);
  }

 public:
  typedef Char char_type;

  ArgFormatterBase(basic_writer<Char> &w, FormatSpec &s)
  : writer_(w), spec_(s) {}

  void operator()(monostate) {
    FMT_ASSERT(false, "invalid argument type");
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value>::type
      operator()(T value) { writer_.write_int(value, spec_); }

  template <typename T>
  typename std::enable_if<std::is_floating_point<T>::value>::type
      operator()(T value) { writer_.write_double(value, spec_); }

  void operator()(bool value) {
    if (spec_.type_)
      return (*this)(value ? 1 : 0);
    write(value);
  }

  void operator()(Char value) {
    if (spec_.type_ && spec_.type_ != 'c') {
      spec_.flags_ |= CHAR_FLAG;
      writer_.write_int(value, spec_);
      return;
    }
    if (spec_.align_ == ALIGN_NUMERIC || spec_.flags_ != 0)
      FMT_THROW(format_error("invalid format specifier for char"));
    typedef typename basic_writer<Char>::CharPtr CharPtr;
    Char fill = internal::CharTraits<Char>::cast(spec_.fill());
    CharPtr out = CharPtr();
    const unsigned CHAR_WIDTH = 1;
    if (spec_.width_ > CHAR_WIDTH) {
      out = writer_.grow_buffer(spec_.width_);
      if (spec_.align_ == ALIGN_RIGHT) {
        std::uninitialized_fill_n(out, spec_.width_ - CHAR_WIDTH, fill);
        out += spec_.width_ - CHAR_WIDTH;
      } else if (spec_.align_ == ALIGN_CENTER) {
        out = writer_.fill_padding(out, spec_.width_,
                                   internal::const_check(CHAR_WIDTH), fill);
      } else {
        std::uninitialized_fill_n(out + CHAR_WIDTH,
                                  spec_.width_ - CHAR_WIDTH, fill);
      }
    } else {
      out = writer_.grow_buffer(CHAR_WIDTH);
    }
    *out = internal::CharTraits<Char>::cast(value);
  }

  void operator()(const char *value) {
    if (spec_.type_ == 'p')
      return write_pointer(value);
    write(value);
  }

  void operator()(StringRef value) {
    writer_.write_str(value, spec_);
  }

  void operator()(BasicStringRef<wchar_t> value) {
    write_str(value);
  }

  void operator()(const void *value) {
    if (spec_.type_ && spec_.type_ != 'p')
      report_unknown_type(spec_.type_, "pointer");
    write_pointer(value);
  }
};

template <typename Char>
inline void write(basic_writer<Char> &w, const Char *start, const Char *end) {
  if (start != end)
    w << BasicStringRef<Char>(start, internal::to_unsigned(end - start));
}

template <typename Char, typename Context>
class format_context_base {
 private:
  const Char *ptr_;
  basic_format_args<Context> args_;
  int next_arg_index_;

 protected:
  typedef basic_format_arg<Context> format_arg;

  format_context_base(const Char *format_str, basic_format_args<Context> args)
  : ptr_(format_str), args_(args), next_arg_index_(0) {}
  ~format_context_base() {}

  basic_format_args<Context> args() const { return args_; }

  // Returns the argument with specified index.
  format_arg do_get_arg(unsigned arg_index, const char *&error) {
    format_arg arg = args_[arg_index];
    if (!arg)
      error = "argument index out of range";
    return arg;
  }

  // Checks if manual indexing is used and returns the argument with
  // specified index.
  format_arg get_arg(unsigned arg_index, const char *&error) {
    return this->check_no_auto_index(error) ?
      this->do_get_arg(arg_index, error) : format_arg();
  }

  // Returns the next argument.
  format_arg next_arg(const char *&error) {
    if (next_arg_index_ >= 0)
      return this->do_get_arg(internal::to_unsigned(next_arg_index_++), error);
    error = "cannot switch from manual to automatic argument indexing";
    return format_arg();
  }

  bool check_no_auto_index(const char *&error) {
    if (next_arg_index_ > 0) {
      error = "cannot switch from automatic to manual argument indexing";
      return false;
    }
    next_arg_index_ = -1;
    return true;
  }

 public:
  // Returns a pointer to the current position in the format string.
  const Char *&ptr() { return ptr_; }
};
}  // namespace internal

/** The default argument formatter. */
template <typename Char>
class ArgFormatter : public internal::ArgFormatterBase<Char> {
 private:
  basic_format_context<Char> &ctx_;

 public:
  /**
    \rst
    Constructs an argument formatter object.
    *writer* is a reference to the writer to be used for output,
    *ctx* is a reference to the formatting context, *spec* contains
    format specifier information for standard argument types.
    \endrst
   */
  ArgFormatter(basic_writer<Char> &writer, basic_format_context<Char> &ctx,
               FormatSpec &spec)
  : internal::ArgFormatterBase<Char>(writer, spec), ctx_(ctx) {}

  using internal::ArgFormatterBase<Char>::operator();

  /** Formats an argument of a custom (user-defined) type. */
  void operator()(internal::CustomValue<Char> c) {
    c.format(this->writer(), c.value, &ctx_);
  }
};

template <typename Char>
class basic_format_context :
  public internal::format_context_base<Char, basic_format_context<Char>> {
 public:
  /** The character type for the output. */
  typedef Char char_type;

 private:
  internal::ArgMap<basic_format_context<Char>> map_;

  FMT_DISALLOW_COPY_AND_ASSIGN(basic_format_context);

  typedef internal::format_context_base<Char, basic_format_context<Char>> Base;

  typedef typename Base::format_arg format_arg;
  using Base::get_arg;

  // Checks if manual indexing is used and returns the argument with
  // specified name.
  format_arg get_arg(BasicStringRef<Char> name, const char *&error);

 public:
  /**
   \rst
   Constructs a ``basic_format_context`` object. References to the arguments are
   stored in the object so make sure they have appropriate lifetimes.
   \endrst
   */
  basic_format_context(const Char *format_str,
                       basic_format_args<basic_format_context> args)
  : Base(format_str, args) {}

  // Parses argument id and returns corresponding argument.
  format_arg parse_arg_id();

  using Base::ptr;
};

/**
 An error returned by an operating system or a language runtime,
 for example a file opening error.
*/
class SystemError : public internal::RuntimeError {
 private:
  void init(int err_code, CStringRef format_str, format_args args);

 protected:
  int error_code_;

  SystemError() {}

 public:
  /**
   \rst
   Constructs a :class:`fmt::SystemError` object with a description
   formatted with `fmt::format_system_error`. *message* and additional
   arguments passed into the constructor are formatted similarly to
   `fmt::format`.

   **Example**::

     // This throws a SystemError with the description
     //   cannot open file 'madeup': No such file or directory
     // or similar (system message may vary).
     const char *filename = "madeup";
     std::FILE *file = std::fopen(filename, "r");
     if (!file)
       throw fmt::SystemError(errno, "cannot open file '{}'", filename);
   \endrst
  */
  template <typename... Args>
  SystemError(int error_code, CStringRef message, const Args & ... args) {
    init(error_code, message, make_format_args(args...));
  }

  ~SystemError() throw();

  int error_code() const { return error_code_; }
};

/**
  \rst
  Formats an error returned by an operating system or a language runtime,
  for example a file opening error, and writes it to *out* in the following
  form:

  .. parsed-literal::
     *<message>*: *<system-message>*

  where *<message>* is the passed message and *<system-message>* is
  the system message corresponding to the error code.
  *error_code* is a system error code as given by ``errno``.
  If *error_code* is not a valid error code such as -1, the system message
  may look like "Unknown error -1" and is platform-dependent.
  \endrst
 */
FMT_API void format_system_error(fmt::writer &out, int error_code,
                                 fmt::StringRef message) FMT_NOEXCEPT;

/**
  \rst
  This template provides operations for formatting and writing data into
  a character stream. The output is stored in a buffer provided by a subclass
  such as :class:`fmt::BasicMemoryWriter`.

  You can use one of the following typedefs for common character types:

  +---------+-----------------------+
  | Type    | Definition            |
  +=========+=======================+
  | writer  | basic_writer<char>    |
  +---------+-----------------------+
  | wwriter | basic_writer<wchar_t> |
  +---------+-----------------------+

  \endrst
 */
template <typename Char>
class basic_writer {
 private:
  // Output buffer.
  Buffer<Char> &buffer_;

  FMT_DISALLOW_COPY_AND_ASSIGN(basic_writer);

  typedef typename internal::CharTraits<Char>::CharPtr CharPtr;

#if FMT_SECURE_SCL
  // Returns pointer value.
  static Char *get(CharPtr p) { return p.base(); }
#else
  static Char *get(Char *p) { return p; }
#endif

  // Fills the padding around the content and returns the pointer to the
  // content area.
  static CharPtr fill_padding(CharPtr buffer,
      unsigned total_size, std::size_t content_size, wchar_t fill);

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  CharPtr grow_buffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return internal::make_ptr(&buffer_[size], n);
  }

  // Writes an unsigned decimal integer.
  template <typename UInt>
  Char *write_unsigned_decimal(UInt value, unsigned prefix_size = 0) {
    unsigned num_digits = internal::count_digits(value);
    Char *ptr = get(grow_buffer(prefix_size + num_digits));
    internal::format_decimal(ptr + prefix_size, value, num_digits);
    return ptr;
  }

  // Writes a decimal integer.
  template <typename Int>
  void write_decimal(Int value) {
    typedef typename internal::IntTraits<Int>::MainType MainType;
    MainType abs_value = static_cast<MainType>(value);
    if (internal::is_negative(value)) {
      abs_value = 0 - abs_value;
      *write_unsigned_decimal(abs_value, 1) = '-';
    } else {
      write_unsigned_decimal(abs_value, 0);
    }
  }

  // Prepare a buffer for integer formatting.
  CharPtr prepare_int_buffer(unsigned num_digits,
      const EmptySpec &, const char *prefix, unsigned prefix_size) {
    unsigned size = prefix_size + num_digits;
    CharPtr p = grow_buffer(size);
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }

  template <typename Spec>
  CharPtr prepare_int_buffer(unsigned num_digits,
    const Spec &spec, const char *prefix, unsigned prefix_size);

  // Formats an integer.
  template <typename T, typename Spec>
  void write_int(T value, Spec spec);

  // Formats a floating-point number (double or long double).
  template <typename T>
  void write_double(T value, const FormatSpec &spec);

  // Writes a formatted string.
  template <typename StrChar>
  CharPtr write_str(const StrChar *s, std::size_t size, const AlignSpec &spec);

  template <typename StrChar>
  void write_str(BasicStringRef<StrChar> str, const FormatSpec &spec);

  // This following methods are private to disallow writing wide characters
  // and strings to a char stream. If you want to print a wide string as a
  // pointer as std::ostream does, cast it to const void*.
  // Do not implement!
  void operator<<(typename internal::WCharHelper<wchar_t, Char>::Unsupported);
  void operator<<(
      typename internal::WCharHelper<const wchar_t *, Char>::Unsupported);

  // Appends floating-point length specifier to the format string.
  // The second argument is only used for overload resolution.
  void append_float_length(Char *&format_ptr, long double) {
    *format_ptr++ = 'L';
  }

  template<typename T>
  void append_float_length(Char *&, T) {}

  template <typename Char_>
  friend class internal::ArgFormatterBase;

  template <typename Char_>
  friend class PrintfArgFormatter;

 protected:
  /**
    Constructs a ``basic_writer`` object.
   */
  explicit basic_writer(Buffer<Char> &b) : buffer_(b) {}

 public:
  /**
    \rst
    Destroys a ``basic_writer`` object.
    \endrst
   */
  virtual ~basic_writer() {}

  /**
    Returns the total number of characters written.
   */
  std::size_t size() const { return buffer_.size(); }

  /**
    Returns a pointer to the output buffer content. No terminating null
    character is appended.
   */
  const Char *data() const FMT_NOEXCEPT { return &buffer_[0]; }

  /**
    Returns a pointer to the output buffer content with terminating null
    character appended.
   */
  const Char *c_str() const {
    std::size_t size = buffer_.size();
    buffer_.reserve(size + 1);
    buffer_[size] = '\0';
    return &buffer_[0];
  }

  /**
    \rst
    Returns the content of the output buffer as an `std::string`.
    \endrst
   */
  std::basic_string<Char> str() const {
    return std::basic_string<Char>(&buffer_[0], buffer_.size());
  }

  void vwrite(BasicCStringRef<Char> format,
              basic_format_args<basic_format_context<Char>> args);
  /**
    \rst
    Writes formatted data.

    *args* is an argument list representing arbitrary arguments.

    **Example**::

       MemoryWriter out;
       out.write("Current point:\n");
       out.write("({:+f}, {:+f})", -3.14, 3.14);

    This will write the following output to the ``out`` object:

    .. code-block:: none

       Current point:
       (-3.140000, +3.140000)

    The output can be accessed using :func:`data()`, :func:`c_str` or
    :func:`str` methods.

    See also :ref:`syntax`.
    \endrst
   */
  template <typename... Args>
  void write(BasicCStringRef<Char> format, const Args & ... args) {
    vwrite(format, make_xformat_args<basic_format_context<Char>>(args...));
  }

  basic_writer &operator<<(int value) {
    write_decimal(value);
    return *this;
  }
  basic_writer &operator<<(unsigned value) {
    return *this << IntFormatSpec<unsigned>(value);
  }
  basic_writer &operator<<(long value) {
    write_decimal(value);
    return *this;
  }
  basic_writer &operator<<(unsigned long value) {
    return *this << IntFormatSpec<unsigned long>(value);
  }
  basic_writer &operator<<(LongLong value) {
    write_decimal(value);
    return *this;
  }

  /**
    \rst
    Formats *value* and writes it to the stream.
    \endrst
   */
  basic_writer &operator<<(ULongLong value) {
    return *this << IntFormatSpec<ULongLong>(value);
  }

  basic_writer &operator<<(double value) {
    write_double(value, FormatSpec());
    return *this;
  }

  /**
    \rst
    Formats *value* using the general format for floating-point numbers
    (``'g'``) and writes it to the stream.
    \endrst
   */
  basic_writer &operator<<(long double value) {
    write_double(value, FormatSpec());
    return *this;
  }

  /**
    Writes a character to the stream.
   */
  basic_writer &operator<<(char value) {
    buffer_.push_back(value);
    return *this;
  }

  basic_writer &operator<<(
      typename internal::WCharHelper<wchar_t, Char>::Supported value) {
    buffer_.push_back(value);
    return *this;
  }

  /**
    \rst
    Writes *value* to the stream.
    \endrst
   */
  basic_writer &operator<<(fmt::BasicStringRef<Char> value) {
    const Char *str = value.data();
    buffer_.append(str, str + value.size());
    return *this;
  }

  basic_writer &operator<<(
      typename internal::WCharHelper<StringRef, Char>::Supported value) {
    const char *str = value.data();
    buffer_.append(str, str + value.size());
    return *this;
  }

  template <typename T, typename Spec, typename FillChar>
  basic_writer &operator<<(IntFormatSpec<T, Spec, FillChar> spec) {
    internal::CharTraits<Char>::convert(FillChar());
    write_int(spec.value(), spec);
    return *this;
  }

  template <typename StrChar>
  basic_writer &operator<<(const StrFormatSpec<StrChar> &spec) {
    const StrChar *s = spec.str();
    write_str(s, std::char_traits<Char>::length(s), spec);
    return *this;
  }

  void clear() FMT_NOEXCEPT { buffer_.clear(); }

  Buffer<Char> &buffer() FMT_NOEXCEPT { return buffer_; }
};

template <typename Char>
template <typename StrChar>
typename basic_writer<Char>::CharPtr basic_writer<Char>::write_str(
      const StrChar *s, std::size_t size, const AlignSpec &spec) {
  CharPtr out = CharPtr();
  if (spec.width() > size) {
    out = grow_buffer(spec.width());
    Char fill = internal::CharTraits<Char>::cast(spec.fill());
    if (spec.align() == ALIGN_RIGHT) {
      std::uninitialized_fill_n(out, spec.width() - size, fill);
      out += spec.width() - size;
    } else if (spec.align() == ALIGN_CENTER) {
      out = fill_padding(out, spec.width(), size, fill);
    } else {
      std::uninitialized_fill_n(out + size, spec.width() - size, fill);
    }
  } else {
    out = grow_buffer(size);
  }
  std::uninitialized_copy(s, s + size, out);
  return out;
}

template <typename Char>
template <typename StrChar>
void basic_writer<Char>::write_str(
    BasicStringRef<StrChar> s, const FormatSpec &spec) {
  // Check if StrChar is convertible to Char.
  internal::CharTraits<Char>::convert(StrChar());
  if (spec.type_ && spec.type_ != 's')
    internal::report_unknown_type(spec.type_, "string");
  const StrChar *str_value = s.data();
  std::size_t str_size = s.size();
  if (str_size == 0) {
    if (!str_value) {
      FMT_THROW(format_error("string pointer is null"));
    }
  }
  std::size_t precision = static_cast<std::size_t>(spec.precision_);
  if (spec.precision_ >= 0 && precision < str_size)
    str_size = precision;
  write_str(str_value, str_size, spec);
}

template <typename Char>
typename basic_writer<Char>::CharPtr basic_writer<Char>::fill_padding(
    CharPtr buffer, unsigned total_size,
    std::size_t content_size, wchar_t fill) {
  std::size_t padding = total_size - content_size;
  std::size_t left_padding = padding / 2;
  Char fill_char = internal::CharTraits<Char>::cast(fill);
  std::uninitialized_fill_n(buffer, left_padding, fill_char);
  buffer += left_padding;
  CharPtr content = buffer;
  std::uninitialized_fill_n(buffer + content_size,
                            padding - left_padding, fill_char);
  return content;
}

template <typename Char>
template <typename Spec>
typename basic_writer<Char>::CharPtr basic_writer<Char>::prepare_int_buffer(
    unsigned num_digits, const Spec &spec,
    const char *prefix, unsigned prefix_size) {
  unsigned width = spec.width();
  Alignment align = spec.align();
  Char fill = internal::CharTraits<Char>::cast(spec.fill());
  if (spec.precision() > static_cast<int>(num_digits)) {
    // Octal prefix '0' is counted as a digit, so ignore it if precision
    // is specified.
    if (prefix_size > 0 && prefix[prefix_size - 1] == '0')
      --prefix_size;
    unsigned number_size =
        prefix_size + internal::to_unsigned(spec.precision());
    AlignSpec subspec(number_size, '0', ALIGN_NUMERIC);
    if (number_size >= width)
      return prepare_int_buffer(num_digits, subspec, prefix, prefix_size);
    buffer_.reserve(width);
    unsigned fill_size = width - number_size;
    if (align != ALIGN_LEFT) {
      CharPtr p = grow_buffer(fill_size);
      std::uninitialized_fill(p, p + fill_size, fill);
    }
    CharPtr result = prepare_int_buffer(
        num_digits, subspec, prefix, prefix_size);
    if (align == ALIGN_LEFT) {
      CharPtr p = grow_buffer(fill_size);
      std::uninitialized_fill(p, p + fill_size, fill);
    }
    return result;
  }
  unsigned size = prefix_size + num_digits;
  if (width <= size) {
    CharPtr p = grow_buffer(size);
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }
  CharPtr p = grow_buffer(width);
  CharPtr end = p + width;
  if (align == ALIGN_LEFT) {
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    p += size;
    std::uninitialized_fill(p, end, fill);
  } else if (align == ALIGN_CENTER) {
    p = fill_padding(p, width, size, fill);
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    p += size;
  } else {
    if (align == ALIGN_NUMERIC) {
      if (prefix_size != 0) {
        p = std::uninitialized_copy(prefix, prefix + prefix_size, p);
        size -= prefix_size;
      }
    } else {
      std::uninitialized_copy(prefix, prefix + prefix_size, end - size);
    }
    std::uninitialized_fill(p, end - size, fill);
    p = end;
  }
  return p - 1;
}

template <typename Char>
template <typename T, typename Spec>
void basic_writer<Char>::write_int(T value, Spec spec) {
  unsigned prefix_size = 0;
  typedef typename internal::IntTraits<T>::MainType UnsignedType;
  UnsignedType abs_value = static_cast<UnsignedType>(value);
  char prefix[4] = "";
  if (internal::is_negative(value)) {
    prefix[0] = '-';
    ++prefix_size;
    abs_value = 0 - abs_value;
  } else if (spec.flag(SIGN_FLAG)) {
    prefix[0] = spec.flag(PLUS_FLAG) ? '+' : ' ';
    ++prefix_size;
  }
  switch (spec.type()) {
  case 0: case 'd': {
    unsigned num_digits = internal::count_digits(abs_value);
    CharPtr p = prepare_int_buffer(num_digits, spec, prefix, prefix_size) + 1;
    internal::format_decimal(get(p), abs_value, 0);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    if (spec.flag(HASH_FLAG)) {
      prefix[prefix_size++] = '0';
      prefix[prefix_size++] = spec.type();
    }
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 4) != 0);
    Char *p = get(prepare_int_buffer(
      num_digits, spec, prefix, prefix_size));
    n = abs_value;
    const char *digits = spec.type() == 'x' ?
        "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    break;
  }
  case 'b': case 'B': {
    UnsignedType n = abs_value;
    if (spec.flag(HASH_FLAG)) {
      prefix[prefix_size++] = '0';
      prefix[prefix_size++] = spec.type();
    }
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 1) != 0);
    Char *p = get(prepare_int_buffer(num_digits, spec, prefix, prefix_size));
    n = abs_value;
    do {
      *p-- = static_cast<Char>('0' + (n & 1));
    } while ((n >>= 1) != 0);
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    if (spec.flag(HASH_FLAG))
      prefix[prefix_size++] = '0';
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 3) != 0);
    Char *p = get(prepare_int_buffer(num_digits, spec, prefix, prefix_size));
    n = abs_value;
    do {
      *p-- = static_cast<Char>('0' + (n & 7));
    } while ((n >>= 3) != 0);
    break;
  }
  case 'n': {
    unsigned num_digits = internal::count_digits(abs_value);
    fmt::StringRef sep = internal::thousands_sep(std::localeconv());
    unsigned size = static_cast<unsigned>(
          num_digits + sep.size() * ((num_digits - 1) / 3));
    CharPtr p = prepare_int_buffer(size, spec, prefix, prefix_size) + 1;
    internal::format_decimal(get(p), abs_value, 0, internal::ThousandsSep(sep));
    break;
  }
  default:
    internal::report_unknown_type(
      spec.type(), spec.flag(CHAR_FLAG) ? "char" : "integer");
    break;
  }
}

template <typename Char>
template <typename T>
void basic_writer<Char>::write_double(T value, const FormatSpec &spec) {
  // Check type.
  char type = spec.type();
  bool upper = false;
  switch (type) {
  case 0:
    type = 'g';
    break;
  case 'e': case 'f': case 'g': case 'a':
    break;
  case 'F':
#if FMT_MSC_VER
    // MSVC's printf doesn't support 'F'.
    type = 'f';
#endif
    // Fall through.
  case 'E': case 'G': case 'A':
    upper = true;
    break;
  default:
    internal::report_unknown_type(type, "double");
    break;
  }

  char sign = 0;
  // Use isnegative instead of value < 0 because the latter is always
  // false for NaN.
  if (internal::fputil::isnegative(static_cast<double>(value))) {
    sign = '-';
    value = -value;
  } else if (spec.flag(SIGN_FLAG)) {
    sign = spec.flag(PLUS_FLAG) ? '+' : ' ';
  }

  if (internal::fputil::isnotanumber(value)) {
    // Format NaN ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t nan_size = 4;
    const char *nan = upper ? " NAN" : " nan";
    if (!sign) {
      --nan_size;
      ++nan;
    }
    CharPtr out = write_str(nan, nan_size, spec);
    if (sign)
      *out = sign;
    return;
  }

  if (internal::fputil::isinfinity(value)) {
    // Format infinity ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t inf_size = 4;
    const char *inf = upper ? " INF" : " inf";
    if (!sign) {
      --inf_size;
      ++inf;
    }
    CharPtr out = write_str(inf, inf_size, spec);
    if (sign)
      *out = sign;
    return;
  }

  std::size_t offset = buffer_.size();
  unsigned width = spec.width();
  if (sign) {
    buffer_.reserve(buffer_.size() + (width > 1u ? width : 1u));
    if (width > 0)
      --width;
    ++offset;
  }

  // Build format string.
  enum { MAX_FORMAT_SIZE = 10}; // longest format: %#-*.*Lg
  Char format[MAX_FORMAT_SIZE];
  Char *format_ptr = format;
  *format_ptr++ = '%';
  unsigned width_for_sprintf = width;
  if (spec.flag(HASH_FLAG))
    *format_ptr++ = '#';
  if (spec.align() == ALIGN_CENTER) {
    width_for_sprintf = 0;
  } else {
    if (spec.align() == ALIGN_LEFT)
      *format_ptr++ = '-';
    if (width != 0)
      *format_ptr++ = '*';
  }
  if (spec.precision() >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }

  append_float_length(format_ptr, value);
  *format_ptr++ = type;
  *format_ptr = '\0';

  // Format using snprintf.
  Char fill = internal::CharTraits<Char>::cast(spec.fill());
  unsigned n = 0;
  Char *start = 0;
  for (;;) {
    std::size_t buffer_size = buffer_.capacity() - offset;
#if FMT_MSC_VER
    // MSVC's vsnprintf_s doesn't work with zero size, so reserve
    // space for at least one extra character to make the size non-zero.
    // Note that the buffer's capacity will increase by more than 1.
    if (buffer_size == 0) {
      buffer_.reserve(offset + 1);
      buffer_size = buffer_.capacity() - offset;
    }
#endif
    start = &buffer_[offset];
    int result = internal::CharTraits<Char>::format_float(
        start, buffer_size, format, width_for_sprintf, spec.precision(), value);
    if (result >= 0) {
      n = internal::to_unsigned(result);
      if (offset + n < buffer_.capacity())
        break;  // The buffer is large enough - continue with formatting.
      buffer_.reserve(offset + n + 1);
    } else {
      // If result is negative we ask to increase the capacity by at least 1,
      // but as std::vector, the buffer grows exponentially.
      buffer_.reserve(buffer_.capacity() + 1);
    }
  }
  if (sign) {
    if ((spec.align() != ALIGN_RIGHT && spec.align() != ALIGN_DEFAULT) ||
        *start != ' ') {
      *(start - 1) = sign;
      sign = 0;
    } else {
      *(start - 1) = fill;
    }
    ++n;
  }
  if (spec.align() == ALIGN_CENTER && spec.width() > n) {
    width = spec.width();
    CharPtr p = grow_buffer(width);
    std::memmove(get(p) + (width - n) / 2, get(p), n * sizeof(Char));
    fill_padding(p, spec.width(), n, fill);
    return;
  }
  if (spec.fill() != ' ' || sign) {
    while (*start == ' ')
      *start++ = fill;
    if (sign)
      *(start - 1) = sign;
  }
  grow_buffer(n);
}

/**
  \rst
  This class template provides operations for formatting and writing data
  into a character stream. The output is stored in a memory buffer that grows
  dynamically.

  You can use one of the following typedefs for common character types
  and the standard allocator:

  +---------------+-----------------------------------------------------+
  | Type          | Definition                                          |
  +===============+=====================================================+
  | MemoryWriter  | BasicMemoryWriter<char, std::allocator<char>>       |
  +---------------+-----------------------------------------------------+
  | WMemoryWriter | BasicMemoryWriter<wchar_t, std::allocator<wchar_t>> |
  +---------------+-----------------------------------------------------+

  **Example**::

     MemoryWriter out;
     out << "The answer is " << 42 << "\n";
     out.write("({:+f}, {:+f})", -3.14, 3.14);

  This will write the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42
     (-3.140000, +3.140000)

  The output can be converted to an ``std::string`` with ``out.str()`` or
  accessed as a C string with ``out.c_str()``.
  \endrst
 */
template <typename Char, typename Allocator = std::allocator<Char> >
class BasicMemoryWriter : public basic_writer<Char> {
 private:
  internal::MemoryBuffer<Char, internal::INLINE_BUFFER_SIZE, Allocator> buffer_;

 public:
  explicit BasicMemoryWriter(const Allocator& alloc = Allocator())
    : basic_writer<Char>(buffer_), buffer_(alloc) {}

#if FMT_USE_RVALUE_REFERENCES
  /**
    \rst
    Constructs a :class:`fmt::BasicMemoryWriter` object moving the content
    of the other object to it.
    \endrst
   */
  BasicMemoryWriter(BasicMemoryWriter &&other)
    : basic_writer<Char>(buffer_), buffer_(std::move(other.buffer_)) {
  }

  /**
    \rst
    Moves the content of the other ``BasicMemoryWriter`` object to this one.
    \endrst
   */
  BasicMemoryWriter &operator=(BasicMemoryWriter &&other) {
    buffer_ = std::move(other.buffer_);
    return *this;
  }
#endif
};

typedef BasicMemoryWriter<char> MemoryWriter;
typedef BasicMemoryWriter<wchar_t> WMemoryWriter;

/**
  \rst
  This class template provides operations for formatting and writing data
  into a fixed-size array. For writing into a dynamically growing buffer
  use :class:`fmt::BasicMemoryWriter`.

  Any write method will throw ``std::runtime_error`` if the output doesn't fit
  into the array.

  You can use one of the following typedefs for common character types:

  +--------------+---------------------------+
  | Type         | Definition                |
  +==============+===========================+
  | ArrayWriter  | BasicArrayWriter<char>    |
  +--------------+---------------------------+
  | WArrayWriter | BasicArrayWriter<wchar_t> |
  +--------------+---------------------------+
  \endrst
 */
template <typename Char>
class BasicArrayWriter : public basic_writer<Char> {
 private:
  internal::FixedBuffer<Char> buffer_;

 public:
  /**
   \rst
   Constructs a :class:`fmt::BasicArrayWriter` object for *array* of the
   given size.
   \endrst
   */
  BasicArrayWriter(Char *array, std::size_t size)
    : basic_writer<Char>(buffer_), buffer_(array, size) {}

  /**
   \rst
   Constructs a :class:`fmt::BasicArrayWriter` object for *array* of the
   size known at compile time.
   \endrst
   */
  template <std::size_t SIZE>
  explicit BasicArrayWriter(Char (&array)[SIZE])
    : basic_writer<Char>(buffer_), buffer_(array, SIZE) {}
};

typedef BasicArrayWriter<char> ArrayWriter;
typedef BasicArrayWriter<wchar_t> WArrayWriter;

// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_system_error(int error_code,
                                 StringRef message) FMT_NOEXCEPT;

#if FMT_USE_WINDOWS_H

/** A Windows error. */
class WindowsError : public SystemError {
 private:
  FMT_API void init(int error_code, CStringRef format_str, format_args args);

 public:
  /**
   \rst
   Constructs a :class:`fmt::WindowsError` object with the description
   of the form

   .. parsed-literal::
     *<message>*: *<system-message>*

   where *<message>* is the formatted message and *<system-message>* is the
   system message corresponding to the error code.
   *error_code* is a Windows error code as given by ``GetLastError``.
   If *error_code* is not a valid error code such as -1, the system message
   will look like "error -1".

   **Example**::

     // This throws a WindowsError with the description
     //   cannot open file 'madeup': The system cannot find the file specified.
     // or similar (system message may vary).
     const char *filename = "madeup";
     LPOFSTRUCT of = LPOFSTRUCT();
     HFILE file = OpenFile(filename, &of, OF_READ);
     if (file == HFILE_ERROR) {
       throw fmt::WindowsError(GetLastError(),
                               "cannot open file '{}'", filename);
     }
   \endrst
  */
  template <typename... Args>
  WindowsError(int error_code, CStringRef message, const Args & ... args) {
    init(error_code, message, make_format_args(args...));
  }
};

// Reports a Windows error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_windows_error(int error_code,
                                  StringRef message) FMT_NOEXCEPT;

#endif

enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

FMT_API void vprint_colored(Color c, CStringRef format, format_args args);

/**
  Formats a string and prints it to stdout using ANSI escape sequences
  to specify color (experimental).
  Example:
    print_colored(fmt::RED, "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... Args>
inline void print_colored(Color c, CStringRef format_str,
                          const Args & ... args) {
  vprint_colored(c, format_str, make_format_args(args...));
}

inline std::string vformat(CStringRef format_str, format_args args) {
  MemoryWriter w;
  w.vwrite(format_str, args);
  return w.str();
}

/**
  \rst
  Formats arguments and returns the result as a string.

  **Example**::

    std::string message = format("The answer is {}", 42);
  \endrst
*/
template <typename... Args>
inline std::string format(CStringRef format_str, const Args & ... args) {
  return vformat(format_str, make_format_args(args...));
}

inline std::wstring vformat(WCStringRef format_str, wformat_args args) {
  WMemoryWriter w;
  w.vwrite(format_str, args);
  return w.str();
}

template <typename... Args>
inline std::wstring format(WCStringRef format_str, const Args & ... args) {
  auto vargs = make_xformat_args<wformat_context>(args...);
  return vformat(format_str, vargs);
}

FMT_API void vprint(std::FILE *f, CStringRef format_str, format_args args);

/**
  \rst
  Prints formatted data to the file *f*.

  **Example**::

    print(stderr, "Don't {}!", "panic");
  \endrst
 */
template <typename... Args>
inline void print(std::FILE *f, CStringRef format_str, const Args & ... args) {
  vprint(f, format_str, make_format_args(args...));
}

FMT_API void vprint(CStringRef format_str, format_args args);

/**
  \rst
  Prints formatted data to ``stdout``.

  **Example**::

    print("Elapsed time: {0:.2f} seconds", 1.23);
  \endrst
 */
template <typename... Args>
inline void print(CStringRef format_str, const Args & ... args) {
  vprint(format_str, make_format_args(args...));
}

/**
  Fast integer formatter.
 */
class FormatInt {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum {BUFFER_SIZE = std::numeric_limits<ULongLong>::digits10 + 3};
  mutable char buffer_[BUFFER_SIZE];
  char *str_;

  // Formats value in reverse and returns the number of digits.
  char *format_decimal(ULongLong value) {
    char *buffer_end = buffer_ + BUFFER_SIZE - 1;
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      unsigned index = static_cast<unsigned>((value % 100) * 2);
      value /= 100;
      *--buffer_end = internal::Data::DIGITS[index + 1];
      *--buffer_end = internal::Data::DIGITS[index];
    }
    if (value < 10) {
      *--buffer_end = static_cast<char>('0' + value);
      return buffer_end;
    }
    unsigned index = static_cast<unsigned>(value * 2);
    *--buffer_end = internal::Data::DIGITS[index + 1];
    *--buffer_end = internal::Data::DIGITS[index];
    return buffer_end;
  }

  void FormatSigned(LongLong value) {
    ULongLong abs_value = static_cast<ULongLong>(value);
    bool negative = value < 0;
    if (negative)
      abs_value = 0 - abs_value;
    str_ = format_decimal(abs_value);
    if (negative)
      *--str_ = '-';
  }

 public:
  explicit FormatInt(int value) { FormatSigned(value); }
  explicit FormatInt(long value) { FormatSigned(value); }
  explicit FormatInt(LongLong value) { FormatSigned(value); }
  explicit FormatInt(unsigned value) : str_(format_decimal(value)) {}
  explicit FormatInt(unsigned long value) : str_(format_decimal(value)) {}
  explicit FormatInt(ULongLong value) : str_(format_decimal(value)) {}

  /** Returns the number of characters written to the output buffer. */
  std::size_t size() const {
    return internal::to_unsigned(buffer_ - str_ + BUFFER_SIZE - 1);
  }

  /**
    Returns a pointer to the output buffer content. No terminating null
    character is appended.
   */
  const char *data() const { return str_; }

  /**
    Returns a pointer to the output buffer content with terminating null
    character appended.
   */
  const char *c_str() const {
    buffer_[BUFFER_SIZE - 1] = '\0';
    return str_;
  }

  /**
    \rst
    Returns the content of the output buffer as an ``std::string``.
    \endrst
   */
  std::string str() const { return std::string(str_, size()); }
};

// Formats a decimal integer value writing into buffer and returns
// a pointer to the end of the formatted string. This function doesn't
// write a terminating null character.
template <typename T>
inline void format_decimal(char *&buffer, T value) {
  typedef typename internal::IntTraits<T>::MainType MainType;
  MainType abs_value = static_cast<MainType>(value);
  if (internal::is_negative(value)) {
    *buffer++ = '-';
    abs_value = 0 - abs_value;
  }
  if (abs_value < 100) {
    if (abs_value < 10) {
      *buffer++ = static_cast<char>('0' + abs_value);
      return;
    }
    unsigned index = static_cast<unsigned>(abs_value * 2);
    *buffer++ = internal::Data::DIGITS[index];
    *buffer++ = internal::Data::DIGITS[index + 1];
    return;
  }
  unsigned num_digits = internal::count_digits(abs_value);
  internal::format_decimal(buffer, abs_value, num_digits);
  buffer += num_digits;
}

/**
  \rst
  Returns a named argument for formatting functions.

  **Example**::

    print("Elapsed time: {s:.2f} seconds", arg("s", 1.23));

  \endrst
 */
template <typename T>
inline internal::NamedArg<format_context> arg(StringRef name, const T &arg) {
  return internal::NamedArg<format_context>(name, arg);
}

template <typename T>
inline internal::NamedArg<wformat_context> arg(WStringRef name, const T &arg) {
  return internal::NamedArg<wformat_context>(name, arg);
}

// The following two functions are deleted intentionally to disable
// nested named arguments as in ``format("{}", arg("a", arg("b", 42)))``.
template <typename Context>
void arg(StringRef, const internal::NamedArg<Context>&)
  FMT_DELETED_OR_UNDEFINED;
template <typename Context>
void arg(WStringRef, const internal::NamedArg<Context>&)
  FMT_DELETED_OR_UNDEFINED;
}

#if FMT_GCC_VERSION
// Use the system_header pragma to suppress warnings about variadic macros
// because suppressing -Wvariadic-macros with the diagnostic pragma doesn't
// work. It is used at the end because we want to suppress as little warnings
// as possible.
# pragma GCC system_header
#endif

namespace fmt {
namespace internal {
template <typename Char>
inline bool is_name_start(Char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || '_' == c;
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
template <typename Char>
unsigned parse_nonnegative_int(const Char *&s) {
  assert('0' <= *s && *s <= '9');
  unsigned value = 0;
  do {
    unsigned new_value = value * 10 + (*s++ - '0');
    // Check if value wrapped around.
    if (new_value < value) {
      value = (std::numeric_limits<unsigned>::max)();
      break;
    }
    value = new_value;
  } while ('0' <= *s && *s <= '9');
  // Convert to unsigned to prevent a warning.
  unsigned max_int = (std::numeric_limits<int>::max)();
  if (value > max_int)
    FMT_THROW(format_error("number is too big"));
  return value;
}

template <typename Char>
inline void require_numeric_argument(
    const basic_format_arg<Char> &arg, char spec) {
  if (!arg.is_numeric()) {
    FMT_THROW(fmt::format_error(
        fmt::format("format specifier '{}' requires numeric argument", spec)));
  }
}

struct IsUnsigned {
  template <typename T>
  typename std::enable_if<std::is_unsigned<T>::value, bool>::type
      operator()(T value) {
    return true;
  }

  template <typename T>
  typename std::enable_if<!std::is_unsigned<T>::value, bool>::type
      operator()(T value) {
    return false;
  }
};

template <typename Char, typename Context>
void check_sign(const Char *&s, const basic_format_arg<Context> &arg) {
  char sign = static_cast<char>(*s);
  require_numeric_argument(arg, sign);
  if (visit(IsUnsigned(), arg)) {
    FMT_THROW(format_error(fmt::format(
      "format specifier '{}' requires signed argument", sign)));
  }
  ++s;
}

template <typename Char, typename Context>
class CustomFormatter {
 private:
  basic_writer<Char> &writer_;
  Context &ctx_;

 public:
  CustomFormatter(basic_writer<Char> &writer, Context &ctx)
  : writer_(writer), ctx_(ctx) {}

  bool operator()(internal::CustomValue<Char> custom) {
    custom.format(writer_, custom.value, &ctx_);
    return true;
  }

  template <typename T>
  bool operator()(T) { return false; }
};

template <typename T>
struct IsInteger {
  enum {
    value = std::is_integral<T>::value && !std::is_same<T, bool>::value &&
            !std::is_same<T, char>::value && !std::is_same<T, wchar_t>::value
  };
};

struct WidthHandler {
  template <typename T>
  typename std::enable_if<IsInteger<T>::value, ULongLong>::type
      operator()(T value) {
    if (is_negative(value))
      FMT_THROW(format_error("negative width"));
    return value;
  }

  template <typename T>
  typename std::enable_if<!IsInteger<T>::value, ULongLong>::type
      operator()(T value) {
    FMT_THROW(format_error("width is not integer"));
    return 0;
  }
};

struct PrecisionHandler {
  template <typename T>
  typename std::enable_if<IsInteger<T>::value, ULongLong>::type
      operator()(T value) {
    if (is_negative(value))
      FMT_THROW(format_error("negative precision"));
    return value;
  }

  template <typename T>
  typename std::enable_if<!IsInteger<T>::value, ULongLong>::type
      operator()(T value) {
    FMT_THROW(format_error("precision is not integer"));
    return 0;
  }
};
}  // namespace internal

template <typename Char>
inline typename basic_format_context<Char>::format_arg
  basic_format_context<Char>::get_arg(
    BasicStringRef<Char> name, const char *&error) {
  if (this->check_no_auto_index(error)) {
    map_.init(this->args());
    if (const format_arg *arg = map_.find(name))
      return *arg;
    error = "argument not found";
  }
  return format_arg();
}

template <typename Char>
inline typename basic_format_context<Char>::format_arg
    basic_format_context<Char>::parse_arg_id() {
  const Char *&s = this->ptr();
  if (!internal::is_name_start(*s)) {
    const char *error = 0;
    format_arg arg = *s < '0' || *s > '9' ?
      this->next_arg(error) :
      get_arg(internal::parse_nonnegative_int(s), error);
    if (error) {
      FMT_THROW(format_error(
                  *s != '}' && *s != ':' ? "invalid format string" : error));
    }
    return arg;
  }
  const Char *start = s;
  Char c;
  do {
    c = *++s;
  } while (internal::is_name_start(c) || ('0' <= c && c <= '9'));
  const char *error = 0;
  format_arg arg = get_arg(BasicStringRef<Char>(start, s - start), error);
  if (error)
    FMT_THROW(format_error(error));
  return arg;
}

// Formats a single argument.
template <typename ArgFormatter, typename Char, typename Context>
void do_format_arg(basic_writer<Char> &writer, basic_format_arg<Context> arg,
                   Context &ctx) {
  const Char *&s = ctx.ptr();
  FormatSpec spec;
  if (*s == ':') {
    if (visit(internal::CustomFormatter<Char, Context>(writer, ctx), arg))
      return;
    ++s;
    // Parse fill and alignment.
    if (Char c = *s) {
      const Char *p = s + 1;
      spec.align_ = ALIGN_DEFAULT;
      do {
        switch (*p) {
          case '<':
            spec.align_ = ALIGN_LEFT;
            break;
          case '>':
            spec.align_ = ALIGN_RIGHT;
            break;
          case '=':
            spec.align_ = ALIGN_NUMERIC;
            break;
          case '^':
            spec.align_ = ALIGN_CENTER;
            break;
        }
        if (spec.align_ != ALIGN_DEFAULT) {
          if (p != s) {
            if (c == '}') break;
            if (c == '{')
              FMT_THROW(format_error("invalid fill character '{'"));
            s += 2;
            spec.fill_ = c;
          } else ++s;
          if (spec.align_ == ALIGN_NUMERIC)
            internal::require_numeric_argument(arg, '=');
          break;
        }
      } while (--p >= s);
    }

    // Parse sign.
    switch (*s) {
      case '+':
        internal::check_sign(s, arg);
        spec.flags_ |= SIGN_FLAG | PLUS_FLAG;
        break;
      case '-':
        internal::check_sign(s, arg);
        spec.flags_ |= MINUS_FLAG;
        break;
      case ' ':
        internal::check_sign(s, arg);
        spec.flags_ |= SIGN_FLAG;
        break;
    }

    if (*s == '#') {
      internal::require_numeric_argument(arg, '#');
      spec.flags_ |= HASH_FLAG;
      ++s;
    }

    // Parse zero flag.
    if (*s == '0') {
      internal::require_numeric_argument(arg, '0');
      spec.align_ = ALIGN_NUMERIC;
      spec.fill_ = '0';
      ++s;
    }

    // Parse width.
    if ('0' <= *s && *s <= '9') {
      spec.width_ = internal::parse_nonnegative_int(s);
    } else if (*s == '{') {
      ++s;
      auto width_arg = ctx.parse_arg_id();
      if (*s++ != '}')
        FMT_THROW(format_error("invalid format string"));
      ULongLong width = visit(internal::WidthHandler(), width_arg);
      if (width > (std::numeric_limits<int>::max)())
        FMT_THROW(format_error("number is too big"));
      spec.width_ = static_cast<int>(width);
    }

    // Parse precision.
    if (*s == '.') {
      ++s;
      spec.precision_ = 0;
      if ('0' <= *s && *s <= '9') {
        spec.precision_ = internal::parse_nonnegative_int(s);
      } else if (*s == '{') {
        ++s;
        auto precision_arg = ctx.parse_arg_id();
        if (*s++ != '}')
          FMT_THROW(format_error("invalid format string"));
        ULongLong precision =
          visit(internal::PrecisionHandler(), precision_arg);
        if (precision > (std::numeric_limits<int>::max)())
          FMT_THROW(format_error("number is too big"));
        spec.precision_ = static_cast<int>(precision);
      } else {
        FMT_THROW(format_error("missing precision specifier"));
      }
      if (arg.is_integral() || arg.is_pointer()) {
        FMT_THROW(format_error(
            fmt::format("precision not allowed in {} format specifier",
            arg.is_pointer() ? "pointer" : "integer")));
      }
    }

    // Parse type.
    if (*s != '}' && *s)
      spec.type_ = static_cast<char>(*s++);
  }

  if (*s != '}')
    FMT_THROW(format_error("missing '}' in format string"));

  // Format argument.
  visit(ArgFormatter(writer, ctx, spec), arg);
}

/** Formats arguments and writes the output to the writer. */
template <typename ArgFormatter, typename Char, typename Context>
void vformat(basic_writer<Char> &writer, BasicCStringRef<Char> format_str,
             basic_format_args<Context> args) {
  basic_format_context<Char> ctx(format_str.c_str(), args);
  const Char *&s = ctx.ptr();
  const Char *start = s;
  while (*s) {
    Char c = *s++;
    if (c != '{' && c != '}') continue;
    if (*s == c) {
      internal::write(writer, start, s);
      start = ++s;
      continue;
    }
    if (c == '}')
      FMT_THROW(format_error("unmatched '}' in format string"));
    internal::write(writer, start, s - 1);
    do_format_arg<ArgFormatter>(writer, ctx.parse_arg_id(), ctx);
    if (*s != '}')
      FMT_THROW(format_error(fmt::format("unknown format specifier")));
    start = ++s;
  }
  internal::write(writer, start, s);
}

template <typename Char>
inline void basic_writer<Char>::vwrite(
    BasicCStringRef<Char> format,
    basic_format_args<basic_format_context<Char>> args) {
  vformat<ArgFormatter<Char>>(*this, format, args);
}
}  // namespace fmt

#if FMT_USE_USER_DEFINED_LITERALS
namespace fmt {
namespace internal {

template <typename Char>
struct UdlFormat {
  const Char *str;

  template <typename... Args>
  auto operator()(Args && ... args) const
                  -> decltype(format(str, std::forward<Args>(args)...)) {
    return format(str, std::forward<Args>(args)...);
  }
};

template <typename Char>
struct UdlArg {
  const Char *str;

  template <typename T>
  NamedArg<basic_format_context<Char>> operator=(T &&value) const {
    return {str, std::forward<T>(value)};
  }
};

} // namespace internal

inline namespace literals {

/**
  \rst
  C++11 literal equivalent of :func:`fmt::format`.

  **Example**::

    using namespace fmt::literals;
    std::string message = "The answer is {}"_format(42);
  \endrst
 */
inline internal::UdlFormat<char>
operator"" _format(const char *s, std::size_t) { return {s}; }
inline internal::UdlFormat<wchar_t>
operator"" _format(const wchar_t *s, std::size_t) { return {s}; }

/**
  \rst
  C++11 literal equivalent of :func:`fmt::arg`.

  **Example**::

    using namespace fmt::literals;
    print("Elapsed time: {s:.2f} seconds", "s"_a=1.23);
  \endrst
 */
inline internal::UdlArg<char>
operator"" _a(const char *s, std::size_t) { return {s}; }
inline internal::UdlArg<wchar_t>
operator"" _a(const wchar_t *s, std::size_t) { return {s}; }

} // inline namespace literals
} // namespace fmt
#endif // FMT_USE_USER_DEFINED_LITERALS

// Restore warnings.
#if FMT_GCC_VERSION >= 406
# pragma GCC diagnostic pop
#endif

#if defined(__clang__) && !defined(FMT_ICC_VERSION)
# pragma clang diagnostic pop
#endif

#ifdef FMT_HEADER_ONLY
# define FMT_FUNC inline
# include "format.cc"
#else
# define FMT_FUNC
#endif

#endif  // FMT_FORMAT_H_
