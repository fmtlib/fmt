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

#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <stdint.h>

#include "fmt/core.h"

#ifdef _SECURE_SCL
# define FMT_SECURE_SCL _SECURE_SCL
#else
# define FMT_SECURE_SCL 0
#endif

#if FMT_SECURE_SCL
# include <iterator>
#endif

#ifdef __has_builtin
# define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
# define FMT_HAS_BUILTIN(x) 0
#endif

#ifdef __GNUC__
# if FMT_GCC_VERSION >= 406
#  pragma GCC diagnostic push

// Disable the warning about declaration shadowing because it affects too
// many valid cases.
#  pragma GCC diagnostic ignored "-Wshadow"

// Disable the warning about implicit conversions that may change the sign of
// an integer; silencing it otherwise would require many explicit casts.
#  pragma GCC diagnostic ignored "-Wsign-conversion"
# endif
#endif

#ifdef __clang__
# define FMT_CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#endif

#if defined(__INTEL_COMPILER)
# define FMT_ICC_VERSION __INTEL_COMPILER
#elif defined(__ICL)
# define FMT_ICC_VERSION __ICL
#endif

#if defined(__clang__) && !defined(FMT_ICC_VERSION)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
# pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
# pragma clang diagnostic ignored "-Wpadded"
#endif

#ifdef __GNUC_LIBSTD__
# define FMT_GNUC_LIBSTD_VERSION (__GNUC_LIBSTD__ * 100 + __GNUC_LIBSTD_MINOR__)
#endif

#ifdef __has_cpp_attribute
# define FMT_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
# define FMT_HAS_CPP_ATTRIBUTE(x) 0
#endif

#ifndef FMT_THROW
# if FMT_EXCEPTIONS
#  define FMT_THROW(x) throw x
# else
#  define FMT_THROW(x) assert(false)
# endif
#endif

// Use the compiler's attribute noreturn.
#if defined(__MINGW32__) || defined(__MINGW64__)
# define FMT_NORETURN __attribute__((noreturn))
#elif FMT_HAS_CPP_ATTRIBUTE(noreturn)
# define FMT_NORETURN [[noreturn]]
#else
# define FMT_NORETURN
#endif

#ifndef FMT_USE_USER_DEFINED_LITERALS
// All compilers which support UDLs also support variadic templates. This
// makes the fmt::literals implementation easier. However, an explicit check
// for variadic templates is added here just in case.
// For Intel's compiler both it and the system gcc/msc must support UDLs.
# if (FMT_HAS_FEATURE(cxx_user_literals) || \
      FMT_GCC_VERSION >= 407 || FMT_MSC_VER >= 1900) && \
      (!defined(FMT_ICC_VERSION) || FMT_ICC_VERSION >= 1500)
#  define FMT_USE_USER_DEFINED_LITERALS 1
# else
#  define FMT_USE_USER_DEFINED_LITERALS 0
# endif
#endif

#if FMT_USE_USER_DEFINED_LITERALS && \
    (FMT_GCC_VERSION >= 600 || FMT_CLANG_VERSION >= 304)
# define FMT_UDL_TEMPLATE 1
#else
# define FMT_UDL_TEMPLATE 0
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

template <typename Char>
class basic_writer;

/** A formatting error such as invalid format string. */
class format_error : public std::runtime_error {
 public:
  explicit format_error(const char *message)
  : std::runtime_error(message) {}

  explicit format_error(const std::string &message)
  : std::runtime_error(message) {}

  ~format_error() throw();
};

namespace internal {

// Casts nonnegative integer to unsigned.
template <typename Int>
constexpr typename std::make_unsigned<Int>::type to_unsigned(Int value) {
  FMT_ASSERT(value >= 0, "negative value");
  return static_cast<typename std::make_unsigned<Int>::type>(value);
}

// The number of characters to store in the basic_memory_buffer object itself
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

// A wrapper around std::locale used to reduce compile times since <locale>
// is very heavy.
class locale;

/**
  \rst
  A contiguous memory buffer with an optional growing ability.
  \endrst
 */
template <typename T>
class basic_buffer {
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(basic_buffer);

  T *ptr_;
  std::size_t size_;
  std::size_t capacity_;

 protected:
  basic_buffer() FMT_NOEXCEPT : ptr_(0), size_(0), capacity_(0) {}

  /** Sets the buffer data and capacity. */
  void set(T* data, std::size_t capacity) FMT_NOEXCEPT {
    ptr_ = data;
    capacity_ = capacity;
  }

  /**
    \rst
    Increases the buffer capacity to hold at least *capacity* elements.
    \endrst
   */
  virtual void grow(std::size_t capacity) = 0;

 public:
  using value_type = T;

  virtual ~basic_buffer() {}

  /** Returns the size of this buffer. */
  std::size_t size() const FMT_NOEXCEPT { return size_; }

  /** Returns the capacity of this buffer. */
  std::size_t capacity() const FMT_NOEXCEPT { return capacity_; }

  /** Returns a pointer to the buffer data. */
  T *data() FMT_NOEXCEPT { return ptr_; }

  /** Returns a pointer to the buffer data. */
  const T *data() const FMT_NOEXCEPT { return ptr_; }

  /**
    Resizes the buffer. If T is a POD type new elements may not be initialized.
   */
  void resize(std::size_t new_size) {
    reserve(new_size);
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

  void push_back(const T &value) {
    reserve(size_ + 1);
    ptr_[size_++] = value;
  }

  /** Appends data to the end of the buffer. */
  template <typename U>
  void append(const U *begin, const U *end);

  T &operator[](std::size_t index) { return ptr_[index]; }
  const T &operator[](std::size_t index) const { return ptr_[index]; }

  virtual fmt::locale locale() const;
};

template <typename T>
template <typename U>
void basic_buffer<T>::append(const U *begin, const U *end) {
  std::size_t new_size = size_ + internal::to_unsigned(end - begin);
  reserve(new_size);
  std::uninitialized_copy(begin, end,
                          internal::make_ptr(ptr_, capacity_) + size_);
  size_ = new_size;
}

template <typename Char>
inline std::basic_string<Char> to_string(const basic_buffer<Char>& buffer) {
  return std::basic_string<Char>(buffer.data(), buffer.size());
}

/**
  \rst
  A dynamically growing memory buffer for trivially copyable/constructible types
  with the first SIZE elements stored in the object itself.

  You can use one of the following typedefs for common character types:

  +----------------+------------------------------+
  | Type           | Definition                   |
  +================+==============================+
  | memory_buffer  | basic_memory_buffer<char>    |
  +----------------+------------------------------+
  | wmemory_buffer | basic_memory_buffer<wchar_t> |
  +----------------+------------------------------+

  **Example**::

     memory_buffer out;
     format_to(out, "The answer is {}.", 42);

  This will write the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42.

  The output can be converted to an ``std::string`` with ``to_string(out)``.
  \endrst
 */
template <typename T, std::size_t SIZE = internal::INLINE_BUFFER_SIZE,
          typename Allocator = std::allocator<T> >
class basic_memory_buffer : private Allocator, public basic_buffer<T> {
 private:
  T store_[SIZE];

  // Deallocate memory allocated by the buffer.
  void deallocate() {
    T* data = this->data();
    if (data != store_) Allocator::deallocate(data, this->capacity());
  }

 protected:
  void grow(std::size_t size);

 public:
  explicit basic_memory_buffer(const Allocator &alloc = Allocator())
      : Allocator(alloc) {
    this->set(store_, SIZE);
  }
  ~basic_memory_buffer() { deallocate(); }

 private:
  // Move data from other to this buffer.
  void move(basic_memory_buffer &other) {
    Allocator &this_alloc = *this, &other_alloc = other;
    this_alloc = std::move(other_alloc);
    T* data = other.data();
    std::size_t size = other.size(), capacity = other.capacity();
    if (data == other.store_) {
      this->set(store_, capacity);
      std::uninitialized_copy(other.store_, other.store_ + size,
                              internal::make_ptr(store_, capacity));
    } else {
      this->set(data, capacity);
      // Set pointer to the inline array so that delete is not called
      // when deallocating.
      other.set(other.store_, 0);
    }
    this->resize(size);
  }

 public:
  /**
    \rst
    Constructs a :class:`fmt::basic_memory_buffer` object moving the content
    of the other object to it.
    \endrst
   */
  basic_memory_buffer(basic_memory_buffer &&other) {
    move(other);
  }

  /**
    \rst
    Moves the content of the other ``basic_memory_buffer`` object to this one.
    \endrst
   */
  basic_memory_buffer &operator=(basic_memory_buffer &&other) {
    assert(this != &other);
    deallocate();
    move(other);
    return *this;
  }

  // Returns a copy of the allocator associated with this buffer.
  Allocator get_allocator() const { return *this; }
};

template <typename T, std::size_t SIZE, typename Allocator>
void basic_memory_buffer<T, SIZE, Allocator>::grow(std::size_t size) {
  std::size_t old_capacity = this->capacity();
  std::size_t new_capacity = old_capacity + old_capacity / 2;
  if (size > new_capacity)
      new_capacity = size;
  T *old_data = this->data();
  T *new_data = this->allocate(new_capacity);
  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(old_data, old_data + this->size(),
                          internal::make_ptr(new_data, new_capacity));
  this->set(new_data, new_capacity);
  // deallocate must not throw according to the standard, but even if it does,
  // the buffer already uses the new storage and will deallocate it in
  // destructor.
  if (old_data != store_)
    Allocator::deallocate(old_data, old_capacity);
}

typedef basic_memory_buffer<char> memory_buffer;
typedef basic_memory_buffer<wchar_t> wmemory_buffer;

/**
  \rst
  A fixed-size memory buffer. For a dynamically growing buffer use
  :class:`fmt::basic_memory_buffer`.

  Trying to increase the buffer size past the initial capacity will throw
  ``std::runtime_error``.
  \endrst
 */
template <typename Char>
class basic_fixed_buffer : public basic_buffer<Char> {
 public:
  /**
   \rst
   Constructs a :class:`fmt::basic_fixed_buffer` object for *array* of the
   given size.
   \endrst
   */
  basic_fixed_buffer(Char *array, std::size_t size) {
    this->set(array, size);
  }

  /**
   \rst
   Constructs a :class:`fmt::basic_fixed_buffer` object for *array* of the
   size known at compile time.
   \endrst
   */
  template <std::size_t SIZE>
  explicit basic_fixed_buffer(Char (&array)[SIZE]) {
    this->set(array, SIZE);
  }

 protected:
  FMT_API void grow(std::size_t size);
};

namespace internal {

template <typename Char>
class basic_char_traits {
 public:
  static Char cast(int value) { return static_cast<Char>(value); }
};

template <typename Char>
class char_traits;

template <>
class char_traits<char> : public basic_char_traits<char> {
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
class char_traits<wchar_t> : public basic_char_traits<wchar_t> {
 public:
  static wchar_t convert(char value) { return value; }
  static wchar_t convert(wchar_t value) { return value; }

  template <typename T>
  FMT_API static int format_float(wchar_t *buffer, std::size_t size,
      const wchar_t *format, unsigned width, int precision, T value);
};

template <typename Char>
class null_terminating_iterator;

template <typename Char>
constexpr const Char *pointer_from(null_terminating_iterator<Char> it);

// An iterator that produces a null terminator on *end. This simplifies parsing
// and allows comparing the performance of processing a null-terminated string
// vs string_view.
template <typename Char>
class null_terminating_iterator {
 public:
  using difference_type = std::ptrdiff_t;
  using value_type = Char;
  using pointer = const Char*;
  using reference = const Char&;
  using iterator_category = std::random_access_iterator_tag;

  null_terminating_iterator() : ptr_(0), end_(0) {}

  constexpr null_terminating_iterator(const Char *ptr, const Char *end)
    : ptr_(ptr), end_(end) {}

  template <typename Range>
  constexpr explicit null_terminating_iterator(const Range &r)
    : ptr_(r.begin()), end_(r.end()) {}

  null_terminating_iterator &operator=(const Char *ptr) {
    assert(ptr <= end_);
    ptr_ = ptr;
    return *this;
  }

  constexpr Char operator*() const {
    return ptr_ != end_ ? *ptr_ : 0;
  }

  constexpr null_terminating_iterator operator++() {
    ++ptr_;
    return *this;
  }

  constexpr null_terminating_iterator operator++(int) {
    null_terminating_iterator result(*this);
    ++ptr_;
    return result;
  }

  constexpr null_terminating_iterator operator--() {
    --ptr_;
    return *this;
  }

  constexpr null_terminating_iterator operator+(difference_type n) {
    return null_terminating_iterator(ptr_ + n, end_);
  }

  constexpr null_terminating_iterator operator-(difference_type n) {
    return null_terminating_iterator(ptr_ - n, end_);
  }

  constexpr null_terminating_iterator operator+=(difference_type n) {
    ptr_ += n;
    return *this;
  }

  constexpr difference_type operator-(null_terminating_iterator other) const {
    return ptr_ - other.ptr_;
  }

  constexpr bool operator!=(null_terminating_iterator other) const {
    return ptr_ != other.ptr_;
  }

  bool operator>=(null_terminating_iterator other) const {
    return ptr_ >= other.ptr_;
  }

  friend constexpr const Char *pointer_from<Char>(null_terminating_iterator it);

 private:
  const Char *ptr_;
  const Char *end_;
};

template <typename T>
constexpr const T *pointer_from(const T *p) { return p; }

template <typename Char>
constexpr const Char *pointer_from(null_terminating_iterator<Char> it) {
  return it.ptr_;
}

// Returns true if value is negative, false otherwise.
// Same as (value < 0) but doesn't produce warnings if T is an unsigned type.
template <typename T>
constexpr typename std::enable_if<
    std::numeric_limits<T>::is_signed, bool>::type is_negative(T value) {
  return value < 0;
}
template <typename T>
constexpr typename std::enable_if<
    !std::numeric_limits<T>::is_signed, bool>::type is_negative(T) {
  return false;
}

template <typename T>
struct int_traits {
  // Smallest of uint32_t and uint64_t that is large enough to represent
  // all values of T.
  typedef typename std::conditional<
    std::numeric_limits<T>::digits <= 32, uint32_t, uint64_t>::type main_type;
};

// Static data is placed in this class template to allow header-only
// configuration.
template <typename T = void>
struct FMT_API basic_data {
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
extern template struct basic_data<void>;
#endif

typedef basic_data<> data;

#ifdef FMT_BUILTIN_CLZLL
// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case count_digits returns 1.
inline unsigned count_digits(uint64_t n) {
  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  int t = (64 - FMT_BUILTIN_CLZLL(n | 1)) * 1233 >> 12;
  return to_unsigned(t) - (n < data::POWERS_OF_10_64[t]) + 1;
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

#if FMT_HAS_CPP_ATTRIBUTE(always_inline)
# define FMT_ALWAYS_INLINE __attribute__((always_inline))
#else
# define FMT_ALWAYS_INLINE
#endif

template <typename Handler>
inline char *lg(uint32_t n, Handler h) FMT_ALWAYS_INLINE;

// Computes g = floor(log10(n)) and calls h.on<g>(n);
template <typename Handler>
inline char *lg(uint32_t n, Handler h) {
  return n < 100 ? n < 10 ? h.template on<0>(n) : h.template on<1>(n)
                 : n < 1000000
                       ? n < 10000 ? n < 1000 ? h.template on<2>(n)
                                              : h.template on<3>(n)
                                   : n < 100000 ? h.template on<4>(n)
                                                : h.template on<5>(n)
                       : n < 100000000 ? n < 10000000 ? h.template on<6>(n)
                                                      : h.template on<7>(n)
                                       : n < 1000000000 ? h.template on<8>(n)
                                                        : h.template on<9>(n);
}

// An lg handler that formats a decimal number.
// Usage: lg(n, decimal_formatter(buffer));
class decimal_formatter {
 private:
  char *buffer_;

  void write_pair(unsigned N, uint32_t index) {
    std::memcpy(buffer_ + N, data::DIGITS + index * 2, 2);
  }

 public:
  explicit decimal_formatter(char *buf) : buffer_(buf) {}

  template <unsigned N> char *on(uint32_t u) {
    if (N == 0) {
      *buffer_ = static_cast<char>(u) + '0';
    } else if (N == 1) {
      write_pair(0, u);
    } else {
      // The idea of using 4.32 fixed-point numbers is based on
      // https://github.com/jeaiii/itoa
      unsigned n = N - 1;
      unsigned a = n / 5 * n * 53 / 16;
      uint64_t t = ((1ULL << (32 + a)) / data::POWERS_OF_10_32[n] + 1 - n / 9);
      t = ((t * u) >> a) + n / 5 * 4;
      write_pair(0, t >> 32);
      for (int i = 2; i < N; i += 2) {
        t = 100ULL * static_cast<uint32_t>(t);
        write_pair(i, t >> 32);
      }
      if (N % 2 == 0) {
        buffer_[N] = static_cast<char>(
          (10ULL * static_cast<uint32_t>(t)) >> 32) + '0';
      }
    }
    return buffer_ += N + 1;
  }
};

// An lg handler that formats a decimal number with a terminating null.
class decimal_formatter_null : public decimal_formatter {
 public:
  explicit decimal_formatter_null(char *buf) : decimal_formatter(buf) {}

  template <unsigned N> char *on(uint32_t u) {
    char *buf = decimal_formatter::on<N>(u);
    *buf = '\0';
    return buf;
  }
};

#ifdef FMT_BUILTIN_CLZ
// Optional version of count_digits for better performance on 32-bit platforms.
inline unsigned count_digits(uint32_t n) {
  int t = (32 - FMT_BUILTIN_CLZ(n | 1)) * 1233 >> 12;
  return to_unsigned(t) - (n < data::POWERS_OF_10_32[t]) + 1;
}
#endif

// A functor that doesn't add a thousands separator.
struct no_thousands_sep {
  template <typename Char>
  void operator()(Char *) {}
};

// A functor that adds a thousands separator.
template <typename Char>
class add_thousands_sep {
 private:
  basic_string_view<Char> sep_;

  // Index of a decimal digit with the least significant digit having index 0.
  unsigned digit_index_;

 public:
  explicit add_thousands_sep(basic_string_view<Char> sep)
    : sep_(sep), digit_index_(0) {}

  void operator()(Char *&buffer) {
    if (++digit_index_ % 3 != 0)
      return;
    buffer -= sep_.size();
    std::uninitialized_copy(sep_.data(), sep_.data() + sep_.size(),
                            internal::make_ptr(buffer, sep_.size()));
  }
};

template <typename Char>
Char thousands_sep(const basic_buffer<Char>& buf);

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
    *--buffer = data::DIGITS[index + 1];
    thousands_sep(buffer);
    *--buffer = data::DIGITS[index];
    thousands_sep(buffer);
  }
  if (value < 10) {
    *--buffer = static_cast<char>('0' + value);
    return;
  }
  unsigned index = static_cast<unsigned>(value * 2);
  *--buffer = data::DIGITS[index + 1];
  thousands_sep(buffer);
  *--buffer = data::DIGITS[index];
}

template <typename UInt, typename Char>
inline void format_decimal(Char *buffer, UInt value, unsigned num_digits) {
  return format_decimal(buffer, value, num_digits, no_thousands_sep());
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
class utf8_to_utf16 {
 private:
  wmemory_buffer buffer_;

 public:
  FMT_API explicit utf8_to_utf16(string_view s);
  operator wstring_view() const { return wstring_view(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
  const wchar_t *c_str() const { return &buffer_[0]; }
  std::wstring str() const { return std::wstring(&buffer_[0], size()); }
};

// A converter from UTF-16 to UTF-8.
// It is only provided for Windows since other systems support UTF-8 natively.
class utf16_to_utf8 {
 private:
  memory_buffer buffer_;

 public:
  utf16_to_utf8() {}
  FMT_API explicit utf16_to_utf8(wstring_view s);
  operator string_view() const { return string_view(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
  const char *c_str() const { return &buffer_[0]; }
  std::string str() const { return std::string(&buffer_[0], size()); }

  // Performs conversion returning a system error code instead of
  // throwing exception on conversion error. This method may still throw
  // in case of memory allocation error.
  FMT_API int convert(wstring_view s);
};

FMT_API void format_windows_error(fmt::buffer &out, int error_code,
                                  fmt::string_view message) FMT_NOEXCEPT;
#endif

template <typename T = void>
struct null {};
}  // namespace internal

struct monostate {};

/**
  \rst
  Visits an argument dispatching to the appropriate visit method based on
  the argument type. For example, if the argument type is ``double`` then
  ``vis(value)`` will be called with the value of type ``double``.
  \endrst
 */
template <typename Visitor, typename Context>
constexpr typename std::result_of<Visitor(int)>::type
    visit(Visitor &&vis, basic_arg<Context> arg) {
  using char_type = typename Context::char_type;
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
    return vis(static_cast<char_type>(arg.value_.int_value));
  case internal::DOUBLE:
    return vis(arg.value_.double_value);
  case internal::LONG_DOUBLE:
    return vis(arg.value_.long_double_value);
  case internal::CSTRING:
    return vis(arg.value_.string.value);
  case internal::STRING:
    return vis(basic_string_view<char_type>(
                 arg.value_.string.value, arg.value_.string.size));
  case internal::POINTER:
    return vis(arg.value_.pointer);
  case internal::CUSTOM:
    return vis(typename basic_arg<Context>::handle(arg.value_.custom));
  }
  return typename std::result_of<Visitor(int)>::type();
}

enum alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

// Flags.
enum {SIGN_FLAG = 1, PLUS_FLAG = 2, MINUS_FLAG = 4, HASH_FLAG = 8};

enum format_spec_tag {fill_tag, align_tag, width_tag, type_tag};

// Format specifier.
template <typename T, format_spec_tag>
class format_spec {
 private:
  T value_;

 public:
  typedef T value_type;

  explicit format_spec(T value) : value_(value) {}

  T value() const { return value_; }
};

// template <typename Char>
// using fill_spec = format_spec<Char, fill_tag>;
template <typename Char>
class fill_spec : public format_spec<Char, fill_tag> {
 public:
  explicit fill_spec(Char value) : format_spec<Char, fill_tag>(value) {}
};

typedef format_spec<unsigned, width_tag> width_spec;
typedef format_spec<char, type_tag> type_spec;

// An empty format specifier.
struct empty_spec {};

// An alignment specifier.
struct align_spec : empty_spec {
  unsigned width_;
  // Fill is always wchar_t and cast to char if necessary to avoid having
  // two specialization of AlignSpec and its subclasses.
  wchar_t fill_;
  alignment align_;

  constexpr align_spec(
      unsigned width, wchar_t fill, alignment align = ALIGN_DEFAULT)
  : width_(width), fill_(fill), align_(align) {}

  constexpr unsigned width() const { return width_; }
  constexpr wchar_t fill() const { return fill_; }
  constexpr alignment align() const { return align_; }

  int precision() const { return -1; }
};

// Format specifiers.
template <typename Char>
class basic_format_specs : public align_spec {
 private:
  template <typename FillChar>
  typename std::enable_if<std::is_same<FillChar, Char>::value ||
                          std::is_same<FillChar, char>::value, void>::type
      set(fill_spec<FillChar> fill) {
    fill_ = fill.value();
  }

  void set(width_spec width) {
    width_ = width.value();
  }

  void set(type_spec type) {
    type_ = type.value();
  }

  template <typename Spec, typename... Specs>
  void set(Spec spec, Specs... tail) {
    set(spec);
    set(tail...);
  }

 public:
  unsigned flags_;
  int precision_;
  Char type_;

  constexpr basic_format_specs(
      unsigned width = 0, char type = 0, wchar_t fill = ' ')
  : align_spec(width, fill), flags_(0), precision_(-1), type_(type) {}

  template <typename... FormatSpecs>
  explicit basic_format_specs(FormatSpecs... specs)
    : align_spec(0, ' '), flags_(0), precision_(-1), type_(0) {
    set(specs...);
  }

  constexpr bool flag(unsigned f) const { return (flags_ & f) != 0; }
  constexpr int precision() const { return precision_; }
  constexpr Char type() const { return type_; }
};

typedef basic_format_specs<char> format_specs;

template <typename Char, typename ErrorHandler>
constexpr unsigned basic_parse_context<Char, ErrorHandler>::next_arg_id() {
  if (next_arg_id_ >= 0)
    return internal::to_unsigned(next_arg_id_++);
  on_error("cannot switch from manual to automatic argument indexing");
  return 0;
}

namespace internal {

template <typename Handler>
constexpr void handle_int_type_spec(char spec, Handler &&handler) {
  switch (spec) {
  case 0: case 'd':
    handler.on_dec();
    break;
  case 'x': case 'X':
    handler.on_hex();
    break;
  case 'b': case 'B':
    handler.on_bin();
    break;
  case 'o':
    handler.on_oct();
    break;
  case 'n':
    handler.on_num();
    break;
  default:
    handler.on_error();
  }
}

template <typename Handler>
constexpr void handle_float_type_spec(char spec, Handler &&handler) {
  switch (spec) {
  case 0: case 'g': case 'G':
    handler.on_general();
    break;
  case 'e': case 'E':
    handler.on_exp();
    break;
  case 'f': case 'F':
    handler.on_fixed();
    break;
   case 'a': case 'A':
    handler.on_hex();
    break;
  default:
    handler.on_error();
    break;
  }
}

template <typename Char, typename Handler>
constexpr void handle_char_specs(
    const basic_format_specs<Char> &specs, Handler &&handler) {
  if (specs.type() && specs.type() != 'c') {
    handler.on_int();
    return;
  }
  if (specs.align() == ALIGN_NUMERIC || specs.flag(~0u) != 0)
    handler.on_error("invalid format specifier for char");
  handler.on_char();
}

template <typename Handler>
constexpr void handle_cstring_type_spec(char spec, Handler &&handler) {
  if (spec == 0 || spec == 's')
    handler.on_string();
  else if (spec == 'p')
    handler.on_pointer();
  else
    handler.on_error("invalid type specifier");
}

template <typename ErrorHandler>
constexpr void check_string_type_spec(char spec, ErrorHandler &&eh) {
  if (spec != 0 && spec != 's')
    eh.on_error("invalid type specifier");
}

template <typename ErrorHandler>
constexpr void check_pointer_type_spec(char spec, ErrorHandler &&eh) {
  if (spec != 0 && spec != 'p')
    eh.on_error("invalid type specifier");
}

template <typename ErrorHandler>
class int_type_checker : private ErrorHandler {
 public:
  constexpr explicit int_type_checker(ErrorHandler eh) : ErrorHandler(eh) {}

  constexpr void on_dec() {}
  constexpr void on_hex() {}
  constexpr void on_bin() {}
  constexpr void on_oct() {}
  constexpr void on_num() {}

  constexpr void on_error() {
    ErrorHandler::on_error("invalid type specifier");
  }
};

template <typename ErrorHandler>
class float_type_checker : private ErrorHandler {
 public:
  constexpr explicit float_type_checker(ErrorHandler eh) : ErrorHandler(eh) {}

  constexpr void on_general() {}
  constexpr void on_exp() {}
  constexpr void on_fixed() {}
  constexpr void on_hex() {}

  constexpr void on_error() {
    ErrorHandler::on_error("invalid type specifier");
  }
};

template <typename ErrorHandler>
class char_specs_checker : public ErrorHandler {
 private:
  char type_;

 public:
  constexpr char_specs_checker(char type, ErrorHandler eh)
    : ErrorHandler(eh), type_(type) {}

  constexpr void on_int() {
    handle_int_type_spec(type_, int_type_checker<ErrorHandler>(*this));
  }
  constexpr void on_char() {}
};

template <typename ErrorHandler>
class cstring_type_checker : public ErrorHandler {
 public:
  constexpr explicit cstring_type_checker(ErrorHandler eh) : ErrorHandler(eh) {}

  constexpr void on_string() {}
  constexpr void on_pointer() {}
};

template <typename Context>
void arg_map<Context>::init(const basic_format_args<Context> &args) {
  if (map_)
    return;
  map_ = new arg[args.max_size()];
  typedef internal::named_arg<Context> NamedArg;
  const NamedArg *named_arg = 0;
  bool use_values =
  args.type(MAX_PACKED_ARGS - 1) == internal::NONE;
  if (use_values) {
    for (unsigned i = 0;/*nothing*/; ++i) {
      internal::type arg_type = args.type(i);
      switch (arg_type) {
        case internal::NONE:
          return;
        case internal::NAMED_ARG:
          named_arg = static_cast<const NamedArg*>(args.values_[i].pointer);
          push_back(arg{named_arg->name, *named_arg});
          break;
        default:
          break; // Do nothing.
      }
    }
    return;
  }
  for (unsigned i = 0; i != MAX_PACKED_ARGS; ++i) {
    internal::type arg_type = args.type(i);
    if (arg_type == internal::NAMED_ARG) {
      named_arg = static_cast<const NamedArg*>(args.args_[i].value_.pointer);
      push_back(arg{named_arg->name, *named_arg});
    }
  }
  for (unsigned i = MAX_PACKED_ARGS; ; ++i) {
    switch (args.args_[i].type_) {
      case internal::NONE:
        return;
      case internal::NAMED_ARG:
        named_arg = static_cast<const NamedArg*>(args.args_[i].value_.pointer);
        push_back(arg{named_arg->name, *named_arg});
        break;
      default:
        break; // Do nothing.
    }
  }
}

template <typename Char>
class arg_formatter_base {
 public:
  typedef basic_format_specs<Char> format_specs;

 private:
  using writer_type = basic_writer<basic_buffer<Char>>;
  writer_type writer_;
  format_specs &specs_;

  FMT_DISALLOW_COPY_AND_ASSIGN(arg_formatter_base);

  void write_char(Char value) {
    using pointer_type = typename writer_type::pointer_type;
    Char fill = internal::char_traits<Char>::cast(specs_.fill());
    pointer_type out = pointer_type();
    const unsigned character_width = 1;
    if (specs_.width_ > character_width) {
      out = writer_.grow_buffer(specs_.width_);
      if (specs_.align_ == ALIGN_RIGHT) {
        std::uninitialized_fill_n(out, specs_.width_ - character_width, fill);
        out += specs_.width_ - character_width;
      } else if (specs_.align_ == ALIGN_CENTER) {
        out = writer_.fill_padding(out, specs_.width_,
                                   internal::const_check(character_width), fill);
      } else {
        std::uninitialized_fill_n(out + character_width,
                                  specs_.width_ - character_width, fill);
      }
    } else {
      out = writer_.grow_buffer(character_width);
    }
    *out = internal::char_traits<Char>::cast(value);
  }

  void write_pointer(const void *p) {
    specs_.flags_ = HASH_FLAG;
    specs_.type_ = 'x';
    writer_.write_int(reinterpret_cast<uintptr_t>(p), specs_);
  }

 protected:
  writer_type &writer() { return writer_; }
  format_specs &spec() { return specs_; }

  void write(bool value) {
    writer_.write_str(string_view(value ? "true" : "false"), specs_);
  }

  void write(const Char *value) {
    writer_.write_str(basic_string_view<Char>(
        value, value != 0 ? std::char_traits<Char>::length(value) : 0), specs_);
  }

 public:
  typedef Char char_type;

  arg_formatter_base(basic_buffer<Char> &b, format_specs &s)
  : writer_(b), specs_(s) {}

  void operator()(monostate) {
    FMT_ASSERT(false, "invalid argument type");
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value>::type
      operator()(T value) { writer_.write_int(value, specs_); }

  template <typename T>
  typename std::enable_if<std::is_floating_point<T>::value>::type
      operator()(T value) { writer_.write_double(value, specs_); }

  void operator()(bool value) {
    if (specs_.type_)
      return (*this)(value ? 1 : 0);
    write(value);
  }

  void operator()(Char value) {
    struct spec_handler : internal::error_handler {
      arg_formatter_base &formatter;
      Char value;

      spec_handler(arg_formatter_base& f, Char val): formatter(f), value(val) {}

      void on_int() { formatter.writer_.write_int(value, formatter.specs_); }
      void on_char() { formatter.write_char(value); }
    };
    internal::handle_char_specs(specs_, spec_handler(*this, value));
  }

  void operator()(const Char *value) {
    struct spec_handler : internal::error_handler {
      arg_formatter_base &formatter;
      const Char *value;

      spec_handler(arg_formatter_base &f, const Char *val)
        : formatter(f), value(val) {}

      void on_string() { formatter.write(value); }
      void on_pointer() { formatter.write_pointer(value); }
    };
    internal::handle_cstring_type_spec(
          specs_.type_, spec_handler(*this, value));
  }

  void operator()(basic_string_view<Char> value) {
    internal::check_string_type_spec(specs_.type_, internal::error_handler());
    writer_.write_str(value, specs_);
  }

  void operator()(const void *value) {
    check_pointer_type_spec(specs_.type_, internal::error_handler());
    write_pointer(value);
  }
};

struct format_string {};

template <typename Char>
constexpr bool is_name_start(Char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || '_' == c;
}

// Parses the input as an unsigned integer. This function assumes that the
// first character is a digit and presence of a non-digit character at the end.
// it: an iterator pointing to the beginning of the input range.
template <typename Iterator, typename ErrorHandler>
constexpr unsigned parse_nonnegative_int(Iterator &it, ErrorHandler &&eh) {
  assert('0' <= *it && *it <= '9');
  unsigned value = 0;
  // Convert to unsigned to prevent a warning.
  unsigned max_int = (std::numeric_limits<int>::max)();
  unsigned big = max_int / 10;
  do {
    // Check for overflow.
    if (value > big) {
      value = max_int + 1;
      break;
    }
    value = value * 10 + (*it - '0');
    // Workaround for MSVC "setup_exception stack overflow" error:
    auto next = it;
    ++next;
    it = next;
  } while ('0' <= *it && *it <= '9');
  if (value > max_int)
    eh.on_error("number is too big");
  return value;
}

template <typename Char, typename Context>
class custom_formatter {
 private:
  basic_buffer<Char> &buffer_;
  Context &ctx_;

 public:
  custom_formatter(basic_buffer<Char> &buffer, Context &ctx)
  : buffer_(buffer), ctx_(ctx) {}

  bool operator()(typename basic_arg<Context>::handle h) {
    h.format(buffer_, ctx_);
    return true;
  }

  template <typename T>
  bool operator()(T) { return false; }
};

template <typename T>
struct is_integer {
  enum {
    value = std::is_integral<T>::value && !std::is_same<T, bool>::value &&
            !std::is_same<T, char>::value && !std::is_same<T, wchar_t>::value
  };
};

template <typename ErrorHandler>
class width_checker {
 public:
  explicit constexpr width_checker(ErrorHandler &eh) : handler_(eh) {}

  template <typename T>
  constexpr typename std::enable_if<
      is_integer<T>::value, unsigned long long>::type operator()(T value) {
    if (is_negative(value))
      handler_.on_error("negative width");
    return value;
  }

  template <typename T>
  constexpr typename std::enable_if<
      !is_integer<T>::value, unsigned long long>::type operator()(T) {
    handler_.on_error("width is not integer");
    return 0;
  }

 private:
  ErrorHandler &handler_;
};

template <typename ErrorHandler>
class precision_checker {
 public:
  explicit constexpr precision_checker(ErrorHandler &eh) : handler_(eh) {}

  template <typename T>
  constexpr typename std::enable_if<
      is_integer<T>::value, unsigned long long>::type operator()(T value) {
    if (is_negative(value))
      handler_.on_error("negative precision");
    return value;
  }

  template <typename T>
  constexpr typename std::enable_if<
      !is_integer<T>::value, unsigned long long>::type operator()(T) {
    handler_.on_error("precision is not integer");
    return 0;
  }

 private:
  ErrorHandler &handler_;
};

// A format specifier handler that sets fields in basic_format_specs.
template <typename Char>
class specs_setter {
 public:
  explicit constexpr specs_setter(basic_format_specs<Char> &specs):
    specs_(specs) {}

  constexpr specs_setter(const specs_setter &other) : specs_(other.specs_) {}

  constexpr void on_align(alignment align) { specs_.align_ = align; }
  constexpr void on_fill(Char fill) { specs_.fill_ = fill; }
  constexpr void on_plus() { specs_.flags_ |= SIGN_FLAG | PLUS_FLAG; }
  constexpr void on_minus() { specs_.flags_ |= MINUS_FLAG; }
  constexpr void on_space() { specs_.flags_ |= SIGN_FLAG; }
  constexpr void on_hash() { specs_.flags_ |= HASH_FLAG; }

  constexpr void on_zero() {
    specs_.align_ = ALIGN_NUMERIC;
    specs_.fill_ = '0';
  }

  constexpr void on_width(unsigned width) { specs_.width_ = width; }
  constexpr void on_precision(unsigned precision) {
    specs_.precision_ = precision;
  }
  constexpr void end_precision() {}

  constexpr void on_type(Char type) { specs_.type_ = type; }

 protected:
  basic_format_specs<Char> &specs_;
};

// A format specifier handler that checks if specifiers are consistent with the
// argument type.
template <typename Handler>
class specs_checker : public Handler {
 public:
  constexpr specs_checker(const Handler& handler, internal::type arg_type)
    : Handler(handler), arg_type_(arg_type) {}

  constexpr specs_checker(const specs_checker &other)
    : Handler(other), arg_type_(other.arg_type_) {}

  constexpr void on_align(alignment align) {
    if (align == ALIGN_NUMERIC)
      require_numeric_argument();
    Handler::on_align(align);
  }

  constexpr void on_plus() {
    check_sign();
    Handler::on_plus();
  }

  constexpr void on_minus() {
    check_sign();
    Handler::on_minus();
  }

  constexpr void on_space() {
    check_sign();
    Handler::on_space();
  }

  constexpr void on_hash() {
    require_numeric_argument();
    Handler::on_hash();
  }

  constexpr void on_zero() {
    require_numeric_argument();
    Handler::on_zero();
  }

  constexpr void end_precision() {
    if (is_integral(arg_type_) || arg_type_ == POINTER)
      this->on_error("precision not allowed for this argument type");
  }

 private:
  constexpr void require_numeric_argument() {
    if (!is_arithmetic(arg_type_))
      this->on_error("format specifier requires numeric argument");
  }

  constexpr void check_sign() {
    require_numeric_argument();
    if (is_integral(arg_type_) && arg_type_ != INT && arg_type_ != LONG_LONG &&
        arg_type_ != CHAR) {
      this->on_error("format specifier requires signed argument");
    }
  }

  internal::type arg_type_;
};

template <template <typename> class Handler, typename T,
          typename Context, typename ErrorHandler>
constexpr void set_dynamic_spec(
    T &value, basic_arg<Context> arg, ErrorHandler eh) {
  unsigned long long big_value = visit(Handler<ErrorHandler>(eh), arg);
  if (big_value > (std::numeric_limits<int>::max)())
    eh.on_error("number is too big");
  value = static_cast<int>(big_value);
}

struct auto_id {};

// The standard format specifier handler with checking.
template <typename Context>
class specs_handler: public specs_setter<typename Context::char_type> {
 public:
  typedef typename Context::char_type char_type;

  constexpr specs_handler(basic_format_specs<char_type> &specs, Context &ctx)
    : specs_setter<char_type>(specs), context_(ctx) {}

  template <typename Id>
  constexpr void on_dynamic_width(Id arg_id) {
    set_dynamic_spec<width_checker>(
          this->specs_.width_, get_arg(arg_id), context_.error_handler());
  }

  template <typename Id>
  constexpr void on_dynamic_precision(Id arg_id) {
    set_dynamic_spec<precision_checker>(
          this->specs_.precision_, get_arg(arg_id), context_.error_handler());
  }

  void on_error(const char *message) {
    context_.on_error(message);
  }

 private:
  constexpr basic_arg<Context> get_arg(auto_id) {
    return context_.next_arg();
  }

  template <typename Id>
  constexpr basic_arg<Context> get_arg(Id arg_id) {
    context_.check_arg_id(arg_id);
    return context_.get_arg(arg_id);
  }

  Context &context_;
};

// An argument reference.
template <typename Char>
struct arg_ref {
  enum Kind { NONE, INDEX, NAME };

  constexpr arg_ref() : kind(NONE), index(0) {}
  constexpr explicit arg_ref(unsigned index) : kind(INDEX), index(index) {}
  explicit arg_ref(basic_string_view<Char> name) : kind(NAME), name(name) {}

  constexpr arg_ref &operator=(unsigned index) {
    kind = INDEX;
    this->index = index;
    return *this;
  }

  Kind kind;
  union {
    unsigned index;
    basic_string_view<Char> name;
  };
};

// Format specifiers with width and precision resolved at formatting rather
// than parsing time to allow re-using the same parsed specifiers with
// differents sets of arguments (precompilation of format strings).
template <typename Char>
struct dynamic_format_specs : basic_format_specs<Char> {
  arg_ref<Char> width_ref;
  arg_ref<Char> precision_ref;
};

// Format spec handler that saves references to arguments representing dynamic
// width and precision to be resolved at formatting time.
template <typename ParseContext>
class dynamic_specs_handler :
    public specs_setter<typename ParseContext::char_type> {
 public:
  using char_type = typename ParseContext::char_type;

  constexpr dynamic_specs_handler(
      dynamic_format_specs<char_type> &specs, ParseContext &ctx)
    : specs_setter<char_type>(specs), specs_(specs), context_(ctx) {}

  constexpr dynamic_specs_handler(const dynamic_specs_handler &other)
    : specs_setter<char_type>(other),
      specs_(other.specs_), context_(other.context_) {}

  template <typename Id>
  constexpr void on_dynamic_width(Id arg_id) {
    specs_.width_ref = make_arg_ref(arg_id);
  }

  template <typename Id>
  constexpr void on_dynamic_precision(Id arg_id) {
    specs_.precision_ref = make_arg_ref(arg_id);
  }

  constexpr void on_error(const char *message) {
    context_.on_error(message);
  }

 private:
  using arg_ref_type = arg_ref<char_type>;

  template <typename Id>
  constexpr arg_ref_type make_arg_ref(Id arg_id) {
    context_.check_arg_id(arg_id);
    return arg_ref_type(arg_id);
  }

  constexpr arg_ref_type make_arg_ref(auto_id) {
    return arg_ref_type(context_.next_arg_id());
  }

  dynamic_format_specs<char_type> &specs_;
  ParseContext &context_;
};

template <typename Iterator, typename IDHandler>
constexpr Iterator parse_arg_id(Iterator it, IDHandler &&handler) {
  using char_type = typename std::iterator_traits<Iterator>::value_type;
  char_type c = *it;
  if (c == '}' || c == ':') {
    handler();
    return it;
  }
  if (c >= '0' && c <= '9') {
    unsigned index = parse_nonnegative_int(it, handler);
    if (*it != '}' && *it != ':') {
      handler.on_error("invalid format string");
      return it;
    }
    handler(index);
    return it;
  }
  if (!is_name_start(c)) {
    handler.on_error("invalid format string");
    return it;
  }
  auto start = it;
  do {
    c = *++it;
  } while (is_name_start(c) || ('0' <= c && c <= '9'));
  handler(basic_string_view<char_type>(pointer_from(start), it - start));
  return it;
}

// Adapts SpecHandler to IDHandler API for dynamic width.
template <typename SpecHandler, typename Char>
struct width_adapter {
  explicit constexpr width_adapter(SpecHandler &h) : handler(h) {}

  constexpr void operator()() { handler.on_dynamic_width(auto_id()); }
  constexpr void operator()(unsigned id) { handler.on_dynamic_width(id); }
  constexpr void operator()(basic_string_view<Char> id) {
    handler.on_dynamic_width(id);
  }

  constexpr void on_error(const char *message) { handler.on_error(message); }

  SpecHandler &handler;
};

// Adapts SpecHandler to IDHandler API for dynamic precision.
template <typename SpecHandler, typename Char>
struct precision_adapter {
  explicit constexpr precision_adapter(SpecHandler &h) : handler(h) {}

  constexpr void operator()() { handler.on_dynamic_precision(auto_id()); }
  constexpr void operator()(unsigned id) { handler.on_dynamic_precision(id); }
  constexpr void operator()(basic_string_view<Char> id) {
    handler.on_dynamic_precision(id);
  }

  constexpr void on_error(const char *message) { handler.on_error(message); }

  SpecHandler &handler;
};

// Parses standard format specifiers and sends notifications about parsed
// components to handler.
// it: an iterator pointing to the beginning of a null-terminated range of
//     characters, possibly emulated via null_terminating_iterator, representing
//     format specifiers.
template <typename Iterator, typename SpecHandler>
constexpr Iterator parse_format_specs(Iterator it, SpecHandler &&handler) {
  using char_type = typename std::iterator_traits<Iterator>::value_type;
  // Parse fill and alignment.
  if (char_type c = *it) {
    alignment align = ALIGN_DEFAULT;
    int i = 1;
    do {
      auto p = it + i;
      switch (*p) {
        case '<':
          align = ALIGN_LEFT;
          break;
        case '>':
          align = ALIGN_RIGHT;
          break;
        case '=':
          align = ALIGN_NUMERIC;
          break;
        case '^':
          align = ALIGN_CENTER;
          break;
      }
      if (align != ALIGN_DEFAULT) {
        handler.on_align(align);
        if (p != it) {
          if (c == '}') break;
          if (c == '{') {
            handler.on_error("invalid fill character '{'");
            return it;
          }
          it += 2;
          handler.on_fill(c);
        } else ++it;
        break;
      }
    } while (--i >= 0);
  }

  // Parse sign.
  switch (*it) {
    case '+':
      handler.on_plus();
      ++it;
      break;
    case '-':
      handler.on_minus();
      ++it;
      break;
    case ' ':
      handler.on_space();
      ++it;
      break;
  }

  if (*it == '#') {
    handler.on_hash();
    ++it;
  }

  // Parse zero flag.
  if (*it == '0') {
    handler.on_zero();
    ++it;
  }

  // Parse width.
  if ('0' <= *it && *it <= '9') {
    handler.on_width(parse_nonnegative_int(it, handler));
  } else if (*it == '{') {
    it = parse_arg_id(it + 1, width_adapter<SpecHandler, char_type>(handler));
    if (*it++ != '}') {
      handler.on_error("invalid format string");
      return it;
    }
  }

  // Parse precision.
  if (*it == '.') {
    ++it;
    if ('0' <= *it && *it <= '9') {
      handler.on_precision(parse_nonnegative_int(it, handler));
    } else if (*it == '{') {
      it = parse_arg_id(
            it + 1, precision_adapter<SpecHandler, char_type>(handler));
      if (*it++ != '}') {
        handler.on_error("invalid format string");
        return it;
      }
    } else {
      handler.on_error("missing precision specifier");
      return it;
    }
    handler.end_precision();
  }

  // Parse type.
  if (*it != '}' && *it)
    handler.on_type(*it++);
  return it;
}

template <typename Handler, typename Char>
struct id_adapter {
  constexpr explicit id_adapter(Handler &h): handler(h) {}

  constexpr void operator()() { handler.on_arg_id(); }
  constexpr void operator()(unsigned id) { handler.on_arg_id(id); }
  constexpr void operator()(basic_string_view<Char> id) {
    handler.on_arg_id(id);
  }

  constexpr void on_error(const char *message) {
    handler.on_error(message);
  }

  Handler &handler;
};

template <typename Iterator, typename Handler>
constexpr void parse_format_string(Iterator it, Handler &&handler) {
  using char_type = typename std::iterator_traits<Iterator>::value_type;
  auto start = it;
  while (*it) {
    char_type ch = *it++;
    if (ch != '{' && ch != '}') continue;
    if (*it == ch) {
      handler.on_text(start, it);
      start = ++it;
      continue;
    }
    if (ch == '}') {
      handler.on_error("unmatched '}' in format string");
      return;
    }
    handler.on_text(start, it - 1);

    it = parse_arg_id(it, id_adapter<Handler, char_type>(handler));
    if (*it == '}') {
      handler.on_replacement_field(it);
    } else if (*it == ':') {
      ++it;
      it = handler.on_format_specs(it);
      if (*it != '}') {
        handler.on_error("unknown format specifier");
        return;
      }
    } else {
      handler.on_error("missing '}' in format string");
      return;
    }

    start = ++it;
  }
  handler.on_text(start, it);
}

template <typename T, typename ParseContext>
constexpr const typename ParseContext::char_type *
    parse_format_specs(ParseContext &ctx) {
  formatter<T, typename ParseContext::char_type> f;
  return f.parse(ctx);
}

template <typename Char, typename ErrorHandler, typename... Args>
class format_string_checker {
 public:
  explicit constexpr format_string_checker(
      basic_string_view<Char> format_str, ErrorHandler eh)
    : context_(format_str, eh) {}

  constexpr void on_text(const Char *, const Char *) {}

  constexpr void on_arg_id() {
    arg_id_ = context_.next_arg_id();
    check_arg_id();
  }
  constexpr void on_arg_id(unsigned id) {
    arg_id_ = id;
    context_.check_arg_id(id);
    check_arg_id();
  }
  constexpr void on_arg_id(basic_string_view<Char>) {}

  constexpr void on_replacement_field(const Char *) {}

  constexpr const Char *on_format_specs(const Char *s) {
    context_.advance_to(s);
    return to_unsigned(arg_id_) < NUM_ARGS ?
          parse_funcs_[arg_id_](context_) : s;
  }

  constexpr void on_error(const char *message) {
    context_.on_error(message);
  }

 private:
  using parse_context_type = basic_parse_context<Char, ErrorHandler>;
  constexpr static size_t NUM_ARGS = sizeof...(Args);

  constexpr void check_arg_id() {
    if (internal::to_unsigned(arg_id_) >= NUM_ARGS)
      context_.on_error("argument index out of range");
  }

  // Format specifier parsing function.
  using parse_func = const Char *(*)(parse_context_type &);

  int arg_id_ = -1;
  parse_context_type context_;
  parse_func parse_funcs_[NUM_ARGS > 0 ? NUM_ARGS : 1] = {
      &parse_format_specs<Args, parse_context_type>...
  };
};

template <typename Char, typename ErrorHandler, typename... Args>
constexpr bool check_format_string(
    basic_string_view<Char> s, ErrorHandler eh = ErrorHandler()) {
  format_string_checker<Char, ErrorHandler, Args...> checker(s, eh);
  parse_format_string(s.begin(), checker);
  return true;
}

// Specifies whether to format T using the standard formatter.
// It is not possible to use get_type in formatter specialization directly
// because of a bug in MSVC.
template <typename T>
struct format_type : std::integral_constant<bool, get_type<T>() != CUSTOM> {};

// Specifies whether to format enums.
template <typename T, typename Enable = void>
struct format_enum : std::integral_constant<bool, std::is_enum<T>::value> {};

template <template <typename> class Handler, typename Spec, typename Char>
void handle_dynamic_spec(
    Spec &value, arg_ref<Char> ref, basic_context<Char> &ctx) {
  switch (ref.kind) {
  case arg_ref<Char>::NONE:
    break;
  case arg_ref<Char>::INDEX:
    internal::set_dynamic_spec<Handler>(
          value, ctx.get_arg(ref.index), ctx.error_handler());
    break;
  case arg_ref<Char>::NAME:
    internal::set_dynamic_spec<Handler>(
          value, ctx.get_arg(ref.name), ctx.error_handler());
    break;
  }
}
}  // namespace internal

/** The default argument formatter. */
template <typename Char>
class arg_formatter : public internal::arg_formatter_base<Char> {
 private:
  basic_context<Char> &ctx_;

  typedef internal::arg_formatter_base<Char> Base;

 public:
  typedef typename Base::format_specs format_specs;

  /**
    \rst
    Constructs an argument formatter object.
    *buffer* is a reference to the buffer to be used for output,
    *ctx* is a reference to the formatting context, *spec* contains
    format specifier information for standard argument types.
    \endrst
   */
  arg_formatter(basic_buffer<Char> &buffer, basic_context<Char> &ctx,
                format_specs &spec)
  : internal::arg_formatter_base<Char>(buffer, spec), ctx_(ctx) {}

  using internal::arg_formatter_base<Char>::operator();

  /** Formats an argument of a custom (user-defined) type. */
  void operator()(typename basic_arg<basic_context<Char>>::handle handle) {
    handle.format(this->writer().buffer(), ctx_);
  }
};

/**
 An error returned by an operating system or a language runtime,
 for example a file opening error.
*/
class system_error : public std::runtime_error {
 private:
  void init(int err_code, string_view format_str, format_args args);

 protected:
  int error_code_;

  system_error() : std::runtime_error("") {}

 public:
  /**
   \rst
   Constructs a :class:`fmt::system_error` object with a description
   formatted with `fmt::format_system_error`. *message* and additional
   arguments passed into the constructor are formatted similarly to
   `fmt::format`.

   **Example**::

     // This throws a system_error with the description
     //   cannot open file 'madeup': No such file or directory
     // or similar (system message may vary).
     const char *filename = "madeup";
     std::FILE *file = std::fopen(filename, "r");
     if (!file)
       throw fmt::system_error(errno, "cannot open file '{}'", filename);
   \endrst
  */
  template <typename... Args>
  system_error(int error_code, string_view message, const Args & ... args)
    : std::runtime_error("") {
    init(error_code, message, make_args(args...));
  }

  ~system_error() throw();

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
FMT_API void format_system_error(fmt::buffer &out, int error_code,
                                 fmt::string_view message) FMT_NOEXCEPT;

/**
  \rst
  This template provides operations for formatting and writing data into a
  character buffer.

  You can use one of the following typedefs for common character types:

  +---------+-----------------------+
  | Type    | Definition            |
  +=========+=======================+
  | writer  | basic_writer<buffer>  |
  +---------+-----------------------+
  | wwriter | basic_writer<wbuffer> |
  +---------+-----------------------+

  \endrst
 */
template <typename Buffer>
class basic_writer {
 public:
  using char_type = typename Buffer::value_type;
  typedef basic_format_specs<char_type> format_specs;

 private:
  // Output buffer.
  Buffer &buffer_;

  FMT_DISALLOW_COPY_AND_ASSIGN(basic_writer);

#if FMT_SECURE_SCL
  typedef stdext::checked_array_iterator<Char*> pointer_type;
  // Returns pointer value.
  static char_type *get(pointer_type p) { return p.base(); }
#else
  typedef char_type *pointer_type;
  static char_type *get(char_type *p) { return p; }
#endif

  // Fills the padding around the content and returns the pointer to the
  // content area.
  static pointer_type fill_padding(pointer_type buffer,
      unsigned total_size, std::size_t content_size, wchar_t fill);

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  pointer_type grow_buffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return internal::make_ptr(&buffer_[size], n);
  }

  // Writes an unsigned decimal integer.
  template <typename UInt>
  char_type *write_unsigned_decimal(UInt value, unsigned prefix_size = 0) {
    unsigned num_digits = internal::count_digits(value);
    char_type *ptr = get(grow_buffer(prefix_size + num_digits));
    internal::format_decimal(ptr + prefix_size, value, num_digits);
    return ptr;
  }

  // Writes a decimal integer.
  template <typename Int>
  void write_decimal(Int value) {
    typedef typename internal::int_traits<Int>::main_type main_type;
    main_type abs_value = static_cast<main_type>(value);
    if (internal::is_negative(value)) {
      abs_value = 0 - abs_value;
      *write_unsigned_decimal(abs_value, 1) = '-';
    } else {
      write_unsigned_decimal(abs_value, 0);
    }
  }

  // Prepare a buffer for integer formatting.
  pointer_type prepare_int_buffer(unsigned num_digits,
      const empty_spec &, const char *prefix, unsigned prefix_size) {
    unsigned size = prefix_size + num_digits;
    pointer_type p = grow_buffer(size);
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }

  template <typename Spec>
  pointer_type prepare_int_buffer(unsigned num_digits,
    const Spec &spec, const char *prefix, unsigned prefix_size);

  // Writes a formatted integer.
  template <typename T, typename Spec>
  void write_int(T value, const Spec& spec);

  // Formats a floating-point number (double or long double).
  template <typename T>
  void write_double(T value, const format_specs &spec);

  // Writes a formatted string.
  template <typename Char>
  pointer_type write_str(
      const Char *s, std::size_t size, const align_spec &spec);

  template <typename Char>
  void write_str(basic_string_view<Char> str, const format_specs &spec);

  // Appends floating-point length specifier to the format string.
  // The second argument is only used for overload resolution.
  void append_float_length(char_type *&format_ptr, long double) {
    *format_ptr++ = 'L';
  }

  template<typename T>
  void append_float_length(char_type *&, T) {}

  template <typename Char>
  friend class internal::arg_formatter_base;

 public:
  /**
    Constructs a ``basic_writer`` object.
   */
  explicit basic_writer(Buffer &b) : buffer_(b) {}

  /**
    \rst
    Destroys the ``basic_writer`` object.
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
  const char_type *data() const FMT_NOEXCEPT { return &buffer_[0]; }

  /**
    Returns a pointer to the output buffer content with terminating null
    character appended.
   */
  const char_type *c_str() const {
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
  std::basic_string<char_type> str() const {
    return std::basic_string<char_type>(&buffer_[0], buffer_.size());
  }

  void write(int value) {
    write_decimal(value);
  }
  void write(long value) {
    write_decimal(value);
  }
  void write(long long value) {
    write_decimal(value);
  }

  /**
    \rst
    Formats *value* and writes it to the buffer.
    \endrst
   */
  template <typename T, typename... FormatSpecs>
  typename std::enable_if<std::is_integral<T>::value, void>::type
      write(T value, FormatSpecs... specs) {
    write_int(value, format_specs(specs...));
  }

  void write(double value) {
    write_double(value, format_specs());
  }

  /**
    \rst
    Formats *value* using the general format for floating-point numbers
    (``'g'``) and writes it to the buffer.
    \endrst
   */
  void write(long double value) {
    write_double(value, format_specs());
  }

  /**
    Writes a character to the buffer.
   */
  void write(char value) {
    buffer_.push_back(value);
  }

  void write(wchar_t value) {
    internal::require_wchar<char_type>();
    buffer_.push_back(value);
  }

  /**
    \rst
    Writes *value* to the buffer.
    \endrst
   */
  void write(string_view value) {
    const char *str = value.data();
    buffer_.append(str, str + value.size());
  }

  void write(basic_string_view<wchar_t> value) {
    internal::require_wchar<char_type>();
    const wchar_t *str = value.data();
    buffer_.append(str, str + value.size());
  }

  template <typename... FormatSpecs>
  void write(basic_string_view<char_type> str, FormatSpecs... specs) {
    write_str(str, format_specs(specs...));
  }

  void clear() FMT_NOEXCEPT { buffer_.resize(0); }

  Buffer &buffer() FMT_NOEXCEPT { return buffer_; }
};

template <typename Buffer>
template <typename Char>
typename basic_writer<Buffer>::pointer_type basic_writer<Buffer>::write_str(
      const Char *s, std::size_t size, const align_spec &spec) {
  pointer_type out = pointer_type();
  if (spec.width() > size) {
    out = grow_buffer(spec.width());
    char_type fill = internal::char_traits<char_type>::cast(spec.fill());
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

template <typename Buffer>
template <typename Char>
void basic_writer<Buffer>::write_str(
    basic_string_view<Char> s, const format_specs &spec) {
  // Check if Char is convertible to char_type.
  internal::char_traits<char_type>::convert(Char());
  const Char *str_value = s.data();
  std::size_t str_size = s.size();
  if (str_size == 0 && !str_value)
    FMT_THROW(format_error("string pointer is null"));
  std::size_t precision = static_cast<std::size_t>(spec.precision_);
  if (spec.precision_ >= 0 && precision < str_size)
    str_size = precision;
  write_str(str_value, str_size, spec);
}

template <typename Buffer>
typename basic_writer<Buffer>::pointer_type basic_writer<Buffer>::fill_padding(
    pointer_type buffer, unsigned total_size,
    std::size_t content_size, wchar_t fill) {
  std::size_t padding = total_size - content_size;
  std::size_t left_padding = padding / 2;
  char_type fill_char = internal::char_traits<char_type>::cast(fill);
  std::uninitialized_fill_n(buffer, left_padding, fill_char);
  buffer += left_padding;
  pointer_type content = buffer;
  std::uninitialized_fill_n(buffer + content_size,
                            padding - left_padding, fill_char);
  return content;
}

template <typename Buffer>
template <typename Spec>
typename basic_writer<Buffer>::pointer_type
  basic_writer<Buffer>::prepare_int_buffer(
    unsigned num_digits, const Spec &spec,
    const char *prefix, unsigned prefix_size) {
  unsigned width = spec.width();
  alignment align = spec.align();
  char_type fill = internal::char_traits<char_type>::cast(spec.fill());
  if (spec.precision() > static_cast<int>(num_digits)) {
    // Octal prefix '0' is counted as a digit, so ignore it if precision
    // is specified.
    if (prefix_size > 0 && prefix[prefix_size - 1] == '0')
      --prefix_size;
    unsigned number_size =
        prefix_size + internal::to_unsigned(spec.precision());
    align_spec subspec(number_size, '0', ALIGN_NUMERIC);
    if (number_size >= width)
      return prepare_int_buffer(num_digits, subspec, prefix, prefix_size);
    buffer_.reserve(width);
    unsigned fill_size = width - number_size;
    if (align != ALIGN_LEFT) {
      pointer_type p = grow_buffer(fill_size);
      std::uninitialized_fill(p, p + fill_size, fill);
    }
    pointer_type result = prepare_int_buffer(
        num_digits, subspec, prefix, prefix_size);
    if (align == ALIGN_LEFT) {
      pointer_type p = grow_buffer(fill_size);
      std::uninitialized_fill(p, p + fill_size, fill);
    }
    return result;
  }
  unsigned size = prefix_size + num_digits;
  if (width <= size) {
    pointer_type p = grow_buffer(size);
    std::uninitialized_copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }
  pointer_type p = grow_buffer(width);
  pointer_type end = p + width;
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

template <typename Buffer>
template <typename T, typename Spec>
void basic_writer<Buffer>::write_int(T value, const Spec& spec) {
  using unsigned_type = typename internal::int_traits<T>::main_type;
  struct spec_handler {
    basic_writer<Buffer> &writer;
    const Spec& spec;
    unsigned prefix_size = 0;
    unsigned_type abs_value;
    char prefix[4] = "";

    spec_handler(basic_writer<Buffer> &w, T val, const Spec& s)
      : writer(w), spec(s), abs_value(static_cast<unsigned_type>(val)) {
      if (internal::is_negative(val)) {
        prefix[0] = '-';
        ++prefix_size;
        abs_value = 0 - abs_value;
      } else if (spec.flag(SIGN_FLAG)) {
        prefix[0] = spec.flag(PLUS_FLAG) ? '+' : ' ';
        ++prefix_size;
      }
    }

    void on_dec() {
      unsigned num_digits = internal::count_digits(abs_value);
      pointer_type p =
          writer.prepare_int_buffer(num_digits, spec, prefix, prefix_size) + 1;
      internal::format_decimal(get(p), abs_value, 0);
    }

    void on_hex() {
      unsigned_type n = abs_value;
      if (spec.flag(HASH_FLAG)) {
        prefix[prefix_size++] = '0';
        prefix[prefix_size++] = spec.type();
      }
      unsigned num_digits = 0;
      do {
        ++num_digits;
      } while ((n >>= 4) != 0);
      char_type *p =
          get(writer.prepare_int_buffer(num_digits, spec, prefix, prefix_size));
      n = abs_value;
      const char *digits = spec.type() == 'x' ?
          "0123456789abcdef" : "0123456789ABCDEF";
      do {
        *p-- = digits[n & 0xf];
      } while ((n >>= 4) != 0);
    }

    void on_bin() {
      unsigned_type n = abs_value;
      if (spec.flag(HASH_FLAG)) {
        prefix[prefix_size++] = '0';
        prefix[prefix_size++] = spec.type();
      }
      unsigned num_digits = 0;
      do {
        ++num_digits;
      } while ((n >>= 1) != 0);
      char_type *p =
          get(writer.prepare_int_buffer(num_digits, spec, prefix, prefix_size));
      n = abs_value;
      do {
        *p-- = static_cast<char_type>('0' + (n & 1));
      } while ((n >>= 1) != 0);
    }

    void on_oct() {
      unsigned_type n = abs_value;
      if (spec.flag(HASH_FLAG))
        prefix[prefix_size++] = '0';
      unsigned num_digits = 0;
      do {
        ++num_digits;
      } while ((n >>= 3) != 0);
      char_type *p =
          get(writer.prepare_int_buffer(num_digits, spec, prefix, prefix_size));
      n = abs_value;
      do {
        *p-- = static_cast<char_type>('0' + (n & 7));
      } while ((n >>= 3) != 0);
    }

    void on_num() {
      unsigned num_digits = internal::count_digits(abs_value);
      char_type thousands_sep = internal::thousands_sep(writer.buffer_);
      fmt::basic_string_view<char_type> sep(&thousands_sep, 1);
      unsigned size = static_cast<unsigned>(
            num_digits + sep.size() * ((num_digits - 1) / 3));
      pointer_type p =
          writer.prepare_int_buffer(size, spec, prefix, prefix_size) + 1;
      internal::format_decimal(get(p), abs_value, 0,
                               internal::add_thousands_sep<char_type>(sep));
    }

    void on_error() {
      FMT_THROW(format_error("invalid type specifier"));
    }
  };
  internal::handle_int_type_spec(spec.type(), spec_handler(*this, value, spec));
}

template <typename Buffer>
template <typename T>
void basic_writer<Buffer>::write_double(T value, const format_specs &spec) {
  // Check type.
  struct spec_handler {
    char type;
    bool upper = false;

    explicit spec_handler(char t) : type(t) {}

    void on_general() {
      if (type == 'G')
        upper = true;
      else
        type = 'g';
    }

    void on_exp() {
      if (type == 'E')
        upper = true;
    }

    void on_fixed() {
      if (type == 'F') {
        upper = true;
#if FMT_MSC_VER
        // MSVC's printf doesn't support 'F'.
        type = 'f';
#endif
      }
    }

    void on_hex() {
      if (type == 'A')
        upper = true;
    }

    void on_error() {
      FMT_THROW(format_error("invalid type specifier"));
    }
  };
  spec_handler handler(spec.type());
  internal::handle_float_type_spec(spec.type(), handler);

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
    const char *nan = handler.upper ? " NAN" : " nan";
    if (!sign) {
      --nan_size;
      ++nan;
    }
    pointer_type out = write_str(nan, nan_size, spec);
    if (sign)
      *out = sign;
    return;
  }

  if (internal::fputil::isinfinity(value)) {
    // Format infinity ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t inf_size = 4;
    const char *inf = handler.upper ? " INF" : " inf";
    if (!sign) {
      --inf_size;
      ++inf;
    }
    pointer_type out = write_str(inf, inf_size, spec);
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
  char_type format[MAX_FORMAT_SIZE];
  char_type *format_ptr = format;
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
  *format_ptr++ = handler.type;
  *format_ptr = '\0';

  // Format using snprintf.
  char_type fill = internal::char_traits<char_type>::cast(spec.fill());
  unsigned n = 0;
  char_type *start = 0;
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
    int result = internal::char_traits<char_type>::format_float(
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
    pointer_type p = grow_buffer(width);
    std::memmove(get(p) + (width - n) / 2, get(p), n * sizeof(char_type));
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

// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_system_error(int error_code,
                                 string_view message) FMT_NOEXCEPT;

#if FMT_USE_WINDOWS_H

/** A Windows error. */
class windows_error : public system_error {
 private:
  FMT_API void init(int error_code, string_view format_str, format_args args);

 public:
  /**
   \rst
   Constructs a :class:`fmt::windows_error` object with the description
   of the form

   .. parsed-literal::
     *<message>*: *<system-message>*

   where *<message>* is the formatted message and *<system-message>* is the
   system message corresponding to the error code.
   *error_code* is a Windows error code as given by ``GetLastError``.
   If *error_code* is not a valid error code such as -1, the system message
   will look like "error -1".

   **Example**::

     // This throws a windows_error with the description
     //   cannot open file 'madeup': The system cannot find the file specified.
     // or similar (system message may vary).
     const char *filename = "madeup";
     LPOFSTRUCT of = LPOFSTRUCT();
     HFILE file = OpenFile(filename, &of, OF_READ);
     if (file == HFILE_ERROR) {
       throw fmt::windows_error(GetLastError(),
                                "cannot open file '{}'", filename);
     }
   \endrst
  */
  template <typename... Args>
  windows_error(int error_code, string_view message, const Args & ... args) {
    init(error_code, message, make_args(args...));
  }
};

// Reports a Windows error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_windows_error(int error_code,
                                  string_view message) FMT_NOEXCEPT;

#endif

/**
  Fast integer formatter.
 */
class FormatInt {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum {BUFFER_SIZE = std::numeric_limits<unsigned long long>::digits10 + 3};
  mutable char buffer_[BUFFER_SIZE];
  char *str_;

  // Formats value in reverse and returns a pointer to the beginning.
  char *format_decimal(unsigned long long value) {
    char *ptr = buffer_ + BUFFER_SIZE - 1;
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      unsigned index = static_cast<unsigned>((value % 100) * 2);
      value /= 100;
      *--ptr = internal::data::DIGITS[index + 1];
      *--ptr = internal::data::DIGITS[index];
    }
    if (value < 10) {
      *--ptr = static_cast<char>('0' + value);
      return ptr;
    }
    unsigned index = static_cast<unsigned>(value * 2);
    *--ptr = internal::data::DIGITS[index + 1];
    *--ptr = internal::data::DIGITS[index];
    return ptr;
  }

  void format_signed(long long value) {
    unsigned long long abs_value = static_cast<unsigned long long>(value);
    bool negative = value < 0;
    if (negative)
      abs_value = 0 - abs_value;
    str_ = format_decimal(abs_value);
    if (negative)
      *--str_ = '-';
  }

 public:
  explicit FormatInt(int value) { format_signed(value); }
  explicit FormatInt(long value) { format_signed(value); }
  explicit FormatInt(long long value) { format_signed(value); }
  explicit FormatInt(unsigned value) : str_(format_decimal(value)) {}
  explicit FormatInt(unsigned long value) : str_(format_decimal(value)) {}
  explicit FormatInt(unsigned long long value) : str_(format_decimal(value)) {}

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
  typedef typename internal::int_traits<T>::main_type main_type;
  main_type abs_value = static_cast<main_type>(value);
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
    *buffer++ = internal::data::DIGITS[index];
    *buffer++ = internal::data::DIGITS[index + 1];
    return;
  }
  unsigned num_digits = internal::count_digits(abs_value);
  internal::format_decimal(buffer, abs_value, num_digits);
  buffer += num_digits;
}

// Formatter of objects of type T.
template <typename T, typename Char>
struct formatter<
    T, Char, typename std::enable_if<internal::format_type<T>::value>::type> {

  // Parses format specifiers stopping either at the end of the range or at the
  // terminating '}'.
  template <typename ParseContext>
  constexpr typename ParseContext::iterator parse(ParseContext &ctx) {
    auto it = internal::null_terminating_iterator<Char>(ctx);
    using handler_type = internal::dynamic_specs_handler<ParseContext>;
    auto type = internal::get_type<T>();
    internal::specs_checker<handler_type>
        handler(handler_type(specs_, ctx), type);
    it = parse_format_specs(it, handler);
    auto type_spec = specs_.type();
    auto eh = ctx.error_handler();
    switch (type) {
    case internal::NONE:
    case internal::NAMED_ARG:
      FMT_ASSERT(false, "invalid argument type");
      break;
    case internal::INT:
    case internal::UINT:
    case internal::LONG_LONG:
    case internal::ULONG_LONG:
    case internal::BOOL:
      handle_int_type_spec(
            type_spec, internal::int_type_checker<decltype(eh)>(eh));
      break;
    case internal::CHAR:
      handle_char_specs(specs_, internal::char_specs_checker<decltype(eh)>(
                          type_spec, eh));
      break;
    case internal::DOUBLE:
    case internal::LONG_DOUBLE:
      handle_float_type_spec(
            type_spec, internal::float_type_checker<decltype(eh)>(eh));
      break;
    case internal::CSTRING:
      internal::handle_cstring_type_spec(
            type_spec, internal::cstring_type_checker<decltype(eh)>(eh));
      break;
    case internal::STRING:
      internal::check_string_type_spec(type_spec, eh);
      break;
    case internal::POINTER:
      internal::check_pointer_type_spec(type_spec, eh);
      break;
    case internal::CUSTOM:
      // Custom format specifiers should be checked in parse functions of
      // formatter specializations.
      break;
    }
    return pointer_from(it);
  }

  void format(basic_buffer<Char> &buf, const T &val, basic_context<Char> &ctx) {
    internal::handle_dynamic_spec<internal::width_checker>(
      specs_.width_, specs_.width_ref, ctx);
    internal::handle_dynamic_spec<internal::precision_checker>(
      specs_.precision_, specs_.precision_ref, ctx);
    visit(arg_formatter<Char>(buf, ctx, specs_),
          internal::make_arg<basic_context<Char>>(val));
  }

 private:
  internal::dynamic_format_specs<Char> specs_;
};

template <typename T, typename Char>
struct formatter<T, Char,
    typename std::enable_if<internal::format_enum<T>::value>::type>
    : public formatter<int, Char> {
  template <typename ParseContext>
  auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
};

// A formatter for types known only at run time such as variant alternatives.
//
// Usage:
//   using variant = std::variant<int, std::string>;
//   template <>
//   struct formatter<variant>: dynamic_formatter<> {
//     void format(buffer &buf, const variant &v, context &ctx) {
//       visit([&](const auto &val) { format(buf, val, ctx); }, v);
//     }
//   };
template <typename Char = char>
struct dynamic_formatter {
  template <typename ParseContext>
  auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    auto it = internal::null_terminating_iterator<Char>(ctx);
    // Checks are deferred to formatting time when the argument type is known.
    internal::dynamic_specs_handler<ParseContext> handler(specs_, ctx);
    it = parse_format_specs(it, handler);
    return pointer_from(it);
  }

  template <typename T>
  void format(basic_buffer<Char> &buf, const T &val, basic_context<Char> &ctx) {
    handle_specs(ctx);
    struct null_handler : internal::error_handler {
      void on_align(alignment) {}
      void on_plus() {}
      void on_minus() {}
      void on_space() {}
      void on_hash() {}
    };
    internal::specs_checker<null_handler>
        checker(null_handler(), internal::get_type<T>());
    checker.on_align(specs_.align());
    if (specs_.flags_ == 0) {
      // Do nothing.
    } else if (specs_.flag(SIGN_FLAG)) {
      if (specs_.flag(PLUS_FLAG))
        checker.on_plus();
      else
        checker.on_space();
    } else if (specs_.flag(MINUS_FLAG)) {
      checker.on_minus();
    } else if (specs_.flag(HASH_FLAG)) {
      checker.on_hash();
    }
    if (specs_.precision_ != -1)
      checker.end_precision();
    visit(arg_formatter<Char>(buf, ctx, specs_),
          internal::make_arg<basic_context<Char>>(val));
  }

 private:
  void handle_specs(basic_context<Char> &ctx) {
    internal::handle_dynamic_spec<internal::width_checker>(
      specs_.width_, specs_.width_ref, ctx);
    internal::handle_dynamic_spec<internal::precision_checker>(
      specs_.precision_, specs_.precision_ref, ctx);
  }

  internal::dynamic_format_specs<Char> specs_;
};

template <typename Char>
inline typename basic_context<Char>::format_arg
  basic_context<Char>::get_arg(basic_string_view<Char> name) {
  map_.init(this->args());
  if (const format_arg *arg = map_.find(name))
    return *arg;
  this->on_error("argument not found");
  return format_arg();
}

/** Formats arguments and writes the output to the buffer. */
template <typename ArgFormatter, typename Char, typename Context>
void vformat_to(basic_buffer<Char> &buffer, basic_string_view<Char> format_str,
                basic_format_args<Context> args) {
  using iterator = internal::null_terminating_iterator<Char>;

  struct handler : internal::error_handler {
    handler(basic_buffer<Char> &b, basic_string_view<Char> str,
            basic_format_args<Context> format_args)
      : buffer(b), context(str, format_args) {}

    void on_text(iterator begin, iterator end) {
      buffer.append(pointer_from(begin), pointer_from(end));
    }

    void on_arg_id() { arg = context.next_arg(); }
    void on_arg_id(unsigned id) {
      context.check_arg_id(id);
      arg = context.get_arg(id);
    }
    void on_arg_id(basic_string_view<Char> id) {
      arg = context.get_arg(id);
    }

    void on_replacement_field(iterator it) {
      context.advance_to(pointer_from(it));
      using internal::custom_formatter;
      if (visit(custom_formatter<Char, Context>(buffer, context), arg))
        return;
      basic_format_specs<Char> specs;
      visit(ArgFormatter(buffer, context, specs), arg);
    }

    iterator on_format_specs(iterator it) {
      context.advance_to(pointer_from(it));
      using internal::custom_formatter;
      if (visit(custom_formatter<Char, Context>(buffer, context), arg))
        return iterator(context);
      basic_format_specs<Char> specs;
      using internal::specs_handler;
      internal::specs_checker<specs_handler<Context>>
          handler(specs_handler<Context>(specs, context), arg.type());
      it = parse_format_specs(it, handler);
      if (*it != '}')
        on_error("missing '}' in format string");
      context.advance_to(pointer_from(it));
      visit(ArgFormatter(buffer, context, specs), arg);
      return it;
    }

    basic_buffer<Char> &buffer;
    basic_context<Char> context;
    basic_arg<Context> arg;
  };
  parse_format_string(iterator(format_str.begin(), format_str.end()),
                      handler(buffer, format_str, args));
}

// Casts ``p`` to ``const void*`` for pointer formatting.
// Example:
//   auto s = format("{}", ptr(p));
template <typename T>
inline const void *ptr(const T *p) { return p; }

class fill_spec_factory {
 public:
  constexpr fill_spec_factory() {}

  template <typename Char>
  fill_spec<Char> operator=(Char value) const {
    return fill_spec<Char>(value);
  }
};

template <typename FormatSpec>
class format_spec_factory {
 public:
  constexpr format_spec_factory() {}

  FormatSpec operator=(typename FormatSpec::value_type value) const {
    return FormatSpec(value);
  }
};

constexpr fill_spec_factory fill;
constexpr format_spec_factory<width_spec> width;
constexpr format_spec_factory<type_spec> type;

template <typename ArgFormatter, typename Char, typename Context>
void vformat_to(basic_buffer<Char> &buffer, basic_string_view<Char> format_str,
                basic_format_args<Context> args);

inline void vformat_to(buffer &buf, string_view format_str, format_args args) {
  vformat_to<arg_formatter<char>>(buf, format_str, args);
}

inline void vformat_to(wbuffer &buf, wstring_view format_str,
                       wformat_args args) {
  vformat_to<arg_formatter<wchar_t>>(buf, format_str, args);
}

inline std::string vformat(string_view format_str, format_args args) {
  memory_buffer buffer;
  vformat_to(buffer, format_str, args);
  return to_string(buffer);
}

inline std::wstring vformat(wstring_view format_str, wformat_args args) {
  wmemory_buffer buffer;
  vformat_to(buffer, format_str, args);
  return to_string(buffer);
}

template <typename String, typename... Args>
inline typename std::enable_if<
  std::is_base_of<internal::format_string, String>::value, std::string>::type
    format(String format_str, const Args & ... args) {
  constexpr bool invalid_format =
      internal::check_format_string<char, internal::error_handler, Args...>(
        string_view(format_str.value(), format_str.size()));
  (void)invalid_format;
  return vformat(format_str.value(), make_args(args...));
}
}  // namespace fmt

#if FMT_USE_USER_DEFINED_LITERALS
namespace fmt {
namespace internal {

# if FMT_UDL_TEMPLATE
template <typename Char, Char... CHARS>
class udl_formatter {
 public:
  template <typename... Args>
  std::basic_string<Char> operator()(const Args &... args) const {
    constexpr Char s[] = {CHARS..., '\0'};
    constexpr bool invalid_format =
        check_format_string<Char, error_handler, Args...>(
          basic_string_view<Char>(s, sizeof...(CHARS)));
    (void)invalid_format;
    return format(s, args...);
  }
};
# else
template <typename Char>
struct udl_formatter {
  const Char *str;

  template <typename... Args>
  auto operator()(Args && ... args) const
                  -> decltype(format(str, std::forward<Args>(args)...)) {
    return format(str, std::forward<Args>(args)...);
  }
};
# endif // FMT_UDL_TEMPLATE

template <typename Char>
struct udl_arg {
  const Char *str;

  template <typename T>
  named_arg<basic_context<Char>> operator=(T &&value) const {
    return {str, std::forward<T>(value)};
  }
};

} // namespace internal

inline namespace literals {

# if FMT_UDL_TEMPLATE
template <typename Char, Char... CHARS>
constexpr internal::udl_formatter<Char, CHARS...> operator""_format() {
  return {};
}
# else
/**
  \rst
  C++11 literal equivalent of :func:`fmt::format`.

  **Example**::

    using namespace fmt::literals;
    std::string message = "The answer is {}"_format(42);
  \endrst
 */
inline internal::udl_formatter<char>
operator"" _format(const char *s, std::size_t) { return {s}; }
inline internal::udl_formatter<wchar_t>
operator"" _format(const wchar_t *s, std::size_t) { return {s}; }
# endif // FMT_UDL_TEMPLATE

/**
  \rst
  C++11 literal equivalent of :func:`fmt::arg`.

  **Example**::

    using namespace fmt::literals;
    print("Elapsed time: {s:.2f} seconds", "s"_a=1.23);
  \endrst
 */
inline internal::udl_arg<char>
operator"" _a(const char *s, std::size_t) { return {s}; }
inline internal::udl_arg<wchar_t>
operator"" _a(const wchar_t *s, std::size_t) { return {s}; }
} // inline namespace literals
} // namespace fmt
#endif // FMT_USE_USER_DEFINED_LITERALS

#define FMT_STRING(s) [] { \
    struct S : fmt::internal::format_string { \
      static constexpr auto value() { return s; } \
      static constexpr size_t size() { return sizeof(s); } \
    }; \
    return S{}; \
  }()

#ifdef FMT_HEADER_ONLY
# define FMT_FUNC inline
# include "format.cc"
#else
# define FMT_FUNC
#endif

// Restore warnings.
#if FMT_GCC_VERSION >= 406
# pragma GCC diagnostic pop
#endif

#if defined(__clang__) && !defined(FMT_ICC_VERSION)
# pragma clang diagnostic pop
#endif

#endif  // FMT_FORMAT_H_
