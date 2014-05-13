/*
 Formatting library for C++

 Copyright (c) 2012, Victor Zverovich
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

#include <stdint.h>

#include <cassert>
#include <cerrno>
#include <cstddef>  // for std::ptrdiff_t
#include <cstdio>
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>

#ifdef __GNUC__
# define FMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
# define FMT_GCC_EXTENSION __extension__
// Disable warning about "long long" which is sometimes reported even
// when using __extension__.
# if FMT_GCC_VERSION >= 406
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wlong-long"
# endif
#else
# define FMT_GCC_EXTENSION
#endif

#if defined(__GNUC_LIBSTD__) && defined (__GNUC_LIBSTD_MINOR__)
# define FMT_GNUC_LIBSTD_VERSION (__GNUC_LIBSTD__ * 100 + __GNUC_LIBSTD_MINOR__)
#endif

// Compatibility with compilers other than clang.
#ifdef __has_feature
# define FMT_HAS_FEATURE(x) __has_feature(x)
# define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
# define FMT_HAS_FEATURE(x) 0
# define FMT_HAS_BUILTIN(x) 0
#endif

#ifndef FMT_USE_VARIADIC_TEMPLATES
// Variadic templates are available in GCC since version 4.4
// (http://gcc.gnu.org/projects/cxx0x.html) and in Visual C++
// since version 2013.
# define FMT_USE_VARIADIC_TEMPLATES \
   (FMT_HAS_FEATURE(cxx_variadic_templates) || \
       (FMT_GCC_VERSION >= 404 && __cplusplus >= 201103) || _MSC_VER >= 1800)
#endif

#ifndef FMT_USE_RVALUE_REFERENCES
// Don't use rvalue references when compiling with clang and an old libstdc++
// as the latter doesn't provide std::move.
# if defined(FMT_GNUC_LIBSTD_VERSION) && FMT_GNUC_LIBSTD_VERSION <= 402
#  define FMT_USE_RVALUE_REFERENCES 0
# else
#  define FMT_USE_RVALUE_REFERENCES \
    (FMT_HAS_FEATURE(cxx_rvalue_references) || \
        (FMT_GCC_VERSION >= 403 && __cplusplus >= 201103) || _MSC_VER >= 1600)
# endif
#endif

#if FMT_USE_RVALUE_REFERENCES
# include <utility>  // for std::move
#endif

// Define FMT_USE_NOEXCEPT to make format use noexcept (C++11 feature).
#if FMT_USE_NOEXCEPT || FMT_HAS_FEATURE(cxx_noexcept) || \
  (FMT_GCC_VERSION >= 408 && __cplusplus >= 201103)
# define FMT_NOEXCEPT(expr) noexcept(expr)
#else
# define FMT_NOEXCEPT(expr)
#endif

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define FMT_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4521) // 'class' : multiple copy constructors specified
#endif

namespace fmt {

// Fix the warning about long long on older versions of GCC
// that don't support the diagnostic pragma.
FMT_GCC_EXTENSION typedef long long LongLong;
FMT_GCC_EXTENSION typedef unsigned long long ULongLong;

template <typename Char>
class BasicWriter;

typedef BasicWriter<char> Writer;
typedef BasicWriter<wchar_t> WWriter;

template <typename Char>
class BasicFormatter;

struct FormatSpec;

/**
  \rst
  A string reference. It can be constructed from a C string, ``std::string``
  or as a result of a formatting operation. It is most useful as a parameter
  type to allow passing different types of strings in a function, for example::

    Formatter<> Format(StringRef format);

    Format("{}") << 42;
    Format(std::string("{}")) << 42;
    Format(Format("{{}}")) << 42;
  \endrst
 */
template <typename Char>
class BasicStringRef {
 private:
  const Char *data_;
  mutable std::size_t size_;

 public:
  /**
    Constructs a string reference object from a C string and a size.
    If *size* is zero, which is the default, the size is computed with
    `strlen`.
   */
  BasicStringRef(const Char *s, std::size_t size = 0) : data_(s), size_(size) {}

  /**
    Constructs a string reference from an `std::string` object.
   */
  BasicStringRef(const std::basic_string<Char> &s)
  : data_(s.c_str()), size_(s.size()) {}

  /**
    Converts a string reference to an `std::string` object.
   */
  operator std::basic_string<Char>() const {
    return std::basic_string<Char>(data_, size());
  }

  /**
    Returns the pointer to a C string.
   */
  const Char *c_str() const { return data_; }

  /**
    Returns the string size.
   */
  std::size_t size() const {
    if (size_ == 0) size_ = std::char_traits<Char>::length(data_);
    return size_;
  }
};

typedef BasicStringRef<char> StringRef;
typedef BasicStringRef<wchar_t> WStringRef;

namespace internal {

// The number of characters to store in the Array object, representing the
// output buffer, itself to avoid dynamic memory allocation.
enum { INLINE_BUFFER_SIZE = 500 };

#if _SECURE_SCL
// Use checked iterator to avoid warnings on MSVC.
template <typename T>
inline stdext::checked_array_iterator<T*> CheckPtr(T *ptr, std::size_t size) {
  return stdext::checked_array_iterator<T*>(ptr, size);
}
#else
template <typename T>
inline T *CheckPtr(T *ptr, std::size_t) { return ptr; }
#endif

// A simple array for POD types with the first SIZE elements stored in
// the object itself. It supports a subset of std::vector's operations.
template <typename T, std::size_t SIZE>
class Array {
 private:
  std::size_t size_;
  std::size_t capacity_;
  T *ptr_;
  T data_[SIZE];

  void Grow(std::size_t size);

  // Free memory allocated by the array.
  void Free() {
    if (ptr_ != data_) delete [] ptr_;
  }

  // Move data from other to this array.
  void Move(Array &other) {
    size_ = other.size_;
    capacity_ = other.capacity_;
    if (other.ptr_ == other.data_) {
      ptr_ = data_;
      std::copy(other.data_, other.data_ + size_, CheckPtr(data_, capacity_));
    } else {
      ptr_ = other.ptr_;
      // Set pointer to the inline array so that delete is not called
      // when freeing.
      other.ptr_ = other.data_;
    }
  }

  FMT_DISALLOW_COPY_AND_ASSIGN(Array);

 public:
  Array() : size_(0), capacity_(SIZE), ptr_(data_) {}
  ~Array() { Free(); }

#if FMT_USE_RVALUE_REFERENCES
  Array(Array &&other) {
    Move(other);
  }

  Array& operator=(Array &&other) {
    assert(this != &other);
    Free();
    Move(other);
    return *this;
  }
#endif

  // Returns the size of this array.
  std::size_t size() const { return size_; }

  // Returns the capacity of this array.
  std::size_t capacity() const { return capacity_; }

  // Resizes the array. If T is a POD type new elements are not initialized.
  void resize(std::size_t new_size) {
    if (new_size > capacity_)
      Grow(new_size);
    size_ = new_size;
  }

  // Reserves space to store at least capacity elements.
  void reserve(std::size_t capacity) {
    if (capacity > capacity_)
      Grow(capacity);
  }

  void clear() { size_ = 0; }

  void push_back(const T &value) {
    if (size_ == capacity_)
      Grow(size_ + 1);
    ptr_[size_++] = value;
  }

  // Appends data to the end of the array.
  void append(const T *begin, const T *end);

  T &operator[](std::size_t index) { return ptr_[index]; }
  const T &operator[](std::size_t index) const { return ptr_[index]; }
};

template <typename T, std::size_t SIZE>
void Array<T, SIZE>::Grow(std::size_t size) {
  capacity_ = (std::max)(size, capacity_ + capacity_ / 2);
  T *p = new T[capacity_];
  std::copy(ptr_, ptr_ + size_, CheckPtr(p, capacity_));
  if (ptr_ != data_)
    delete [] ptr_;
  ptr_ = p;
}

template <typename T, std::size_t SIZE>
void Array<T, SIZE>::append(const T *begin, const T *end) {
  std::ptrdiff_t num_elements = end - begin;
  if (size_ + num_elements > capacity_)
    Grow(num_elements);
  std::copy(begin, end, CheckPtr(ptr_, capacity_) + size_);
  size_ += num_elements;
}

template <typename Char>
class CharTraits;

template <typename Char>
class BasicCharTraits {
 public:
#if _SECURE_SCL
  typedef stdext::checked_array_iterator<Char*> CharPtr;
#else
  typedef Char *CharPtr;
#endif
};

template <>
class CharTraits<char> : public BasicCharTraits<char> {
 private:
  // Conversion from wchar_t to char is not supported.
  static char ConvertChar(wchar_t);

 public:
  typedef const wchar_t *UnsupportedStrType;

  static char ConvertChar(char value) { return value; }

  template <typename T>
  static int FormatFloat(char *buffer, std::size_t size,
      const char *format, unsigned width, int precision, T value);
};

template <>
class CharTraits<wchar_t> : public BasicCharTraits<wchar_t> {
 public:
  typedef const char *UnsupportedStrType;

  static wchar_t ConvertChar(char value) { return value; }
  static wchar_t ConvertChar(wchar_t value) { return value; }

  template <typename T>
  static int FormatFloat(wchar_t *buffer, std::size_t size,
      const wchar_t *format, unsigned width, int precision, T value);
};

// Selects uint32_t if FitsIn32Bits is true, uint64_t otherwise.
template <bool FitsIn32Bits>
struct TypeSelector { typedef uint32_t Type; };

template <>
struct TypeSelector<false> { typedef uint64_t Type; };

// Checks if a number is negative - used to avoid warnings.
template <bool IsSigned>
struct SignChecker {
  template <typename T>
  static bool IsNegative(T) { return false; }
};

template <>
struct SignChecker<true> {
  template <typename T>
  static bool IsNegative(T value) { return value < 0; }
};

// Returns true if value is negative, false otherwise.
// Same as (value < 0) but doesn't produce warnings if T is an unsigned type.
template <typename T>
inline bool IsNegative(T value) {
  return SignChecker<std::numeric_limits<T>::is_signed>::IsNegative(value);
}

template <typename T>
struct IntTraits {
  // Smallest of uint32_t and uint64_t that is large enough to represent
  // all values of T.
  typedef typename
    TypeSelector<std::numeric_limits<T>::digits <= 32>::Type MainType;
};

template <typename T>
struct IsLongDouble { enum {VALUE = 0}; };

template <>
struct IsLongDouble<long double> { enum {VALUE = 1}; };

void ReportUnknownType(char code, const char *type);

extern const uint32_t POWERS_OF_10_32[];
extern const uint64_t POWERS_OF_10_64[];

#if FMT_GCC_VERSION >= 400 || FMT_HAS_BUILTIN(__builtin_clzll)
// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case CountDigits returns 1.
inline unsigned CountDigits(uint64_t n) {
  // Based on http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
  // and the benchmark https://github.com/localvoid/cxx-benchmark-count-digits.
  uint64_t t = (64 - __builtin_clzll(n | 1)) * 1233 >> 12;
  return t - (n < POWERS_OF_10_64[t]) + 1;
}
# if FMT_GCC_VERSION >= 400 || FMT_HAS_BUILTIN(__builtin_clz)
// Optional version of CountDigits for better performance on 32-bit platforms.
inline unsigned CountDigits(uint32_t n) {
  uint32_t t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  return t - (n < POWERS_OF_10_32[t]) + 1;
}
# endif
#else
// Slower version of CountDigits used when __builtin_clz is not available.
inline unsigned CountDigits(uint64_t n) {
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

extern const char DIGITS[];

// Formats a decimal unsigned integer value writing into buffer.
template <typename UInt, typename Char>
void FormatDecimal(Char *buffer, UInt value, unsigned num_digits) {
  --num_digits;
  while (value >= 100) {
    // Integer division is slow so do it for a group of two digits instead
    // of for every digit. The idea comes from the talk by Alexandrescu
    // "Three Optimization Tips for C++". See speed-test for a comparison.
    unsigned index = (value % 100) * 2;
    value /= 100;
    buffer[num_digits] = internal::DIGITS[index + 1];
    buffer[num_digits - 1] = internal::DIGITS[index];
    num_digits -= 2;
  }
  if (value < 10) {
    *buffer = static_cast<char>('0' + value);
    return;
  }
  unsigned index = static_cast<unsigned>(value * 2);
  buffer[1] = internal::DIGITS[index + 1];
  buffer[0] = internal::DIGITS[index];
}

template <typename Char, typename T>
void FormatCustomArg(
  BasicWriter<Char> &w, const void *arg, const FormatSpec &spec);

#ifdef _WIN32
// A converter from UTF-8 to UTF-16.
// It is only provided for Windows since other systems use UTF-8.
class UTF8ToUTF16 {
 private:
  Array<wchar_t, INLINE_BUFFER_SIZE> buffer_;

 public:
  explicit UTF8ToUTF16(StringRef s);
  operator WStringRef() const { return WStringRef(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
};

// A converter from UTF-16 to UTF-8.
// It is only provided for Windows since other systems use UTF-8.
class UTF16ToUTF8 {
 private:
  Array<char, INLINE_BUFFER_SIZE> buffer_;

 public:
  UTF16ToUTF8() {}
  explicit UTF16ToUTF8(WStringRef s);
  operator StringRef() const { return StringRef(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }

  // Performs conversion returning a system error code instead of
  // throwing exception on error.
  int Convert(WStringRef s);
};
#endif

// Portable thread-safe version of strerror.
// Sets buffer to point to a string describing the error code.
// This can be either a pointer to a string stored in buffer,
// or a pointer to some static immutable string.
// Returns one of the following values:
//   0      - success
//   ERANGE - buffer is not large enough to store the error message
//   other  - failure
// Buffer should be at least of size 1.
int StrError(int error_code,
    char *&buffer, std::size_t buffer_size) FMT_NOEXCEPT(true);

void FormatSystemErrorMessage(
    fmt::Writer &out, int error_code, fmt::StringRef message);

#ifdef _WIN32
void FormatWinErrorMessage(
    fmt::Writer &out, int error_code, fmt::StringRef message);
#endif

}  // namespace internal

/**
  A formatting error such as invalid format string.
 */
class FormatError : public std::runtime_error {
 public:
  explicit FormatError(const std::string &message)
  : std::runtime_error(message) {}
};

/**
  An error returned by the operating system or the language runtime,
  for example a file opening error.
 */
class SystemError : public std::runtime_error {
 private:
  int error_code_;

 public:
  SystemError(StringRef message, int error_code)
  : std::runtime_error(message), error_code_(error_code) {}

  int error_code() const { return error_code_; }
};

enum Alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

// Flags.
enum { SIGN_FLAG = 1, PLUS_FLAG = 2, HASH_FLAG = 4 };

// An empty format specifier.
struct EmptySpec {};

// A type specifier.
template <char TYPE>
struct TypeSpec : EmptySpec {
  Alignment align() const { return ALIGN_DEFAULT; }
  unsigned width() const { return 0; }

  bool sign_flag() const { return false; }
  bool plus_flag() const { return false; }
  bool hash_flag() const { return false; }

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
};

// An alignment and type specifier.
template <char TYPE>
struct AlignTypeSpec : AlignSpec {
  AlignTypeSpec(unsigned width, wchar_t fill) : AlignSpec(width, fill) {}

  bool sign_flag() const { return false; }
  bool plus_flag() const { return false; }
  bool hash_flag() const { return false; }

  char type() const { return TYPE; }
};

// A full format specifier.
struct FormatSpec : AlignSpec {
  unsigned flags_;
  char type_;

  FormatSpec(unsigned width = 0, char type = 0, wchar_t fill = ' ')
  : AlignSpec(width, fill), flags_(0), type_(type) {}

  bool sign_flag() const { return (flags_ & SIGN_FLAG) != 0; }
  bool plus_flag() const { return (flags_ & PLUS_FLAG) != 0; }
  bool hash_flag() const { return (flags_ & HASH_FLAG) != 0; }

  char type() const { return type_; }
};

// An integer format specifier.
template <typename T, typename SpecT = TypeSpec<0>, typename Char = char>
class IntFormatSpec : public SpecT {
 private:
  T value_;

 public:
  IntFormatSpec(T value, const SpecT &spec = SpecT())
  : SpecT(spec), value_(value) {}

  T value() const { return value_; }
};

// A string format specifier.
template <typename T>
class StrFormatSpec : public AlignSpec {
 private:
  const T *str_;

 public:
  StrFormatSpec(const T *str, unsigned width, wchar_t fill)
  : AlignSpec(width, fill), str_(str) {}

  const T *str() const { return str_; }
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

    std::string s = str(Writer() << pad(hex(0xcafe), 8, '0'));
    // s == "0000cafe"

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

    std::string s = str(Writer() << pad("abc", 8));
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

/**
  \rst
  This template provides operations for formatting and writing data into
  a character stream. The output is stored in a memory buffer that grows
  dynamically.

  You can use one of the following typedefs for common character types:

  +---------+----------------------+
  | Type    | Definition           |
  +=========+======================+
  | Writer  | BasicWriter<char>    |
  +---------+----------------------+
  | WWriter | BasicWriter<wchar_t> |
  +---------+----------------------+

  **Example**::

     Writer out;
     out << "The answer is " << 42 << "\n";
     out.Format("({:+f}, {:+f})") << -3.14 << 3.14;

  This will write the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42
     (-3.140000, +3.140000)

  The output can be converted to an ``std::string`` with ``out.str()`` or
  accessed as a C string with ``out.c_str()``.
  \endrst
 */
template <typename Char>
class BasicWriter {
 private:
  // Output buffer.
  mutable internal::Array<Char, internal::INLINE_BUFFER_SIZE> buffer_;

  // Make BasicFormatter a friend so that it can access ArgInfo and Arg.
  friend class BasicFormatter<Char>;

  typedef typename internal::CharTraits<Char>::CharPtr CharPtr;

#if _SECURE_SCL
  static Char *GetBase(CharPtr p) { return p.base(); }
#else
  static Char *GetBase(Char *p) { return p; }
#endif

  static CharPtr FillPadding(CharPtr buffer,
      unsigned total_size, std::size_t content_size, wchar_t fill);

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  CharPtr GrowBuffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return internal::CheckPtr(&buffer_[size], n);
  }

  CharPtr PrepareFilledBuffer(unsigned size, const EmptySpec &, char sign) {
    CharPtr p = GrowBuffer(size);
    *p = sign;
    return p + size - 1;
  }

  CharPtr PrepareFilledBuffer(unsigned size, const AlignSpec &spec, char sign);

  // Formats an integer.
  template <typename T, typename Spec>
  void FormatInt(T value, const Spec &spec);

  // Formats a floating-point number (double or long double).
  template <typename T>
  void FormatDouble(T value, const FormatSpec &spec, int precision);

  // Formats a string.
  template <typename StringChar>
  CharPtr FormatString(
      const StringChar *s, std::size_t size, const AlignSpec &spec);

  // This method is private to disallow writing a wide string to a
  // char stream and vice versa. If you want to print a wide string
  // as a pointer as std::ostream does, cast it to const void*.
  // Do not implement!
  void operator<<(typename internal::CharTraits<Char>::UnsupportedStrType);

  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, LONG_LONG, ULONG_LONG, DOUBLE, LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };

  struct StringValue {
    const Char *value;
    std::size_t size;
  };

  typedef void (*FormatFunc)(
    BasicWriter<Char> &w, const void *arg, const FormatSpec &spec);

  struct CustomValue {
    const void *value;
    FormatFunc format;
  };

  // Information about a format argument. It is a POD type to allow
  // storage in internal::Array.
  struct ArgInfo {
    Type type;
    union {
      int int_value;
      unsigned uint_value;
      double double_value;
      long long_value;
      unsigned long ulong_value;
      LongLong long_long_value;
      ULongLong ulong_long_value;
      long double long_double_value;
      const void *pointer_value;
      StringValue string;
      CustomValue custom;
    };
  };

  // An argument action that does nothing.
  struct NullArgAction {
    void operator()() const {}
  };

  // A wrapper around a format argument.
  template <typename Action = NullArgAction>
  class BasicArg : public Action, public ArgInfo {
   private:
    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to const void*. Do not implement!
    template <typename T>
    BasicArg(const T *value);

    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to void*. Do not implement!
    template <typename T>
    BasicArg(T *value);

   public:
    using ArgInfo::type;

    BasicArg(short value) { type = INT; this->int_value = value; }
    BasicArg(unsigned short value) { type = UINT; this->int_value = value; }
    BasicArg(int value) { type = INT; this->int_value = value; }
    BasicArg(unsigned value) { type = UINT; this->uint_value = value; }
    BasicArg(long value) { type = LONG; this->long_value = value; }
    BasicArg(unsigned long value) { type = ULONG; this->ulong_value = value; }
    BasicArg(LongLong value) {
      type = LONG_LONG;
      this->long_long_value = value;
    }
    BasicArg(ULongLong value) {
      type = ULONG_LONG;
      this->ulong_long_value = value;
    }
    BasicArg(float value) { type = DOUBLE; this->double_value = value; }
    BasicArg(double value) { type = DOUBLE; this->double_value = value; }
    BasicArg(long double value) {
      type = LONG_DOUBLE;
      this->long_double_value = value;
    }
    BasicArg(char value) { type = CHAR; this->int_value = value; }
    BasicArg(wchar_t value) {
      type = CHAR;
      this->int_value = internal::CharTraits<Char>::ConvertChar(value);
    }

    BasicArg(const Char *value) {
      type = STRING;
      this->string.value = value;
      this->string.size = 0;
    }

    BasicArg(Char *value) {
      type = STRING;
      this->string.value = value;
      this->string.size = 0;
    }

    BasicArg(const void *value) { type = POINTER; this->pointer_value = value; }
    BasicArg(void *value) { type = POINTER; this->pointer_value = value; }

    BasicArg(const std::basic_string<Char> &value) {
      type = STRING;
      this->string.value = value.c_str();
      this->string.size = value.size();
    }

    BasicArg(BasicStringRef<Char> value) {
      type = STRING;
      this->string.value = value.c_str();
      this->string.size = value.size();
    }

    template <typename T>
    BasicArg(const T &value) {
      type = CUSTOM;
      this->custom.value = &value;
      this->custom.format = &internal::FormatCustomArg<Char, T>;
    }

    // The destructor is declared noexcept(false) because the action may throw
    // an exception.
    ~BasicArg() FMT_NOEXCEPT(false) {
      // Invoke the action.
      (*this)();
    }
  };

  typedef BasicArg<> Arg;

  // Format string parser.
  class FormatParser {
   private:
    std::size_t num_args_;
    const ArgInfo *args_;
    int num_open_braces_;
    int next_arg_index_;

    void ReportError(const Char *s, StringRef message) const;

    unsigned ParseUInt(const Char *&s) const;

    // Parses argument index and returns an argument with this index.
    const ArgInfo &ParseArgIndex(const Char *&s);

    void CheckSign(const Char *&s, const ArgInfo &arg);

   public:
    void Format(BasicWriter<Char> &writer,
      BasicStringRef<Char> format, std::size_t num_args, const ArgInfo *args);
  };

 public:
  BasicWriter() {}

#if FMT_USE_RVALUE_REFERENCES
  BasicWriter(BasicWriter &&other) : buffer_(std::move(other.buffer_)) {}

  BasicWriter& operator=(BasicWriter &&other) {
    assert(this != &other);
    buffer_ = std::move(other.buffer_);
    return *this;
  }
#endif

  /**
    Returns the number of characters written to the output buffer.
   */
  std::size_t size() const { return buffer_.size(); }

  /**
    Returns a pointer to the output buffer content. No terminating null
    character is appended.
   */
  const Char *data() const { return &buffer_[0]; }

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
    Returns the content of the output buffer as an `std::string`.
   */
  std::basic_string<Char> str() const {
    return std::basic_string<Char>(&buffer_[0], buffer_.size());
  }

  /**
    \rst
    Formats a string sending the output to the writer. Arguments are
    accepted through the returned ``BasicFormatter`` object using inserter
    operator ``<<``.

    **Example**::

       Writer out;
       out.Format("Current point:\n");
       out.Format("({:+f}, {:+f})") << -3.14 << 3.14;

    This will write the following output to the ``out`` object:

    .. code-block:: none

       Current point:
       (-3.140000, +3.140000)

    The output can be accessed using :meth:`data` or :meth:`c_str`.

    See also `Format String Syntax`_.
    \endrst
   */
  BasicFormatter<Char> Format(StringRef format);

  inline void VFormat(BasicStringRef<Char> format,
      std::size_t num_args, const ArgInfo *args) {
    FormatParser().Format(*this, format, num_args, args);
  }

#if FMT_USE_VARIADIC_TEMPLATES
  /**
    \rst
    Formats a string sending the output to the writer.

    **Example**::

       Writer out;
       out.Format("Current point:\n");
       out.Format("({:+f}, {:+f})", -3.14, 3.14);

    This will write the following output to the ``out`` object:

    .. code-block:: none

       Current point:
       (-3.140000, +3.140000)

    The output can be accessed using :meth:`data` or :meth:`c_str`.

    See also `Format String Syntax`_.
    \endrst
   */
  template<typename... Args>
  void Format(BasicStringRef<Char> format, const Args & ... args) {
    Arg arg_array[] = {args...};
    VFormat(format, sizeof...(Args), arg_array);
  }
#endif

  BasicWriter &operator<<(int value) {
    return *this << IntFormatSpec<int>(value);
  }
  BasicWriter &operator<<(unsigned value) {
    return *this << IntFormatSpec<unsigned>(value);
  }
  BasicWriter &operator<<(long value) {
    return *this << IntFormatSpec<long>(value);
  }
  BasicWriter &operator<<(unsigned long value) {
    return *this << IntFormatSpec<unsigned long>(value);
  }
  BasicWriter &operator<<(LongLong value) {
    return *this << IntFormatSpec<LongLong>(value);
  }

  /**
    Formats *value* and writes it to the stream.
   */
  BasicWriter &operator<<(ULongLong value) {
    return *this << IntFormatSpec<ULongLong>(value);
  }

  BasicWriter &operator<<(double value) {
    FormatDouble(value, FormatSpec(), -1);
    return *this;
  }

  /**
    Formats *value* using the general format for floating-point numbers
    (``'g'``) and writes it to the stream.
   */
  BasicWriter &operator<<(long double value) {
    FormatDouble(value, FormatSpec(), -1);
    return *this;
  }

  /**
    Writes a character to the stream.
   */
  BasicWriter &operator<<(char value) {
    *GrowBuffer(1) = value;
    return *this;
  }

  BasicWriter &operator<<(wchar_t value) {
    *GrowBuffer(1) = internal::CharTraits<Char>::ConvertChar(value);
    return *this;
  }

  /**
    Writes *value* to the stream.
   */
  BasicWriter &operator<<(const fmt::BasicStringRef<Char> value) {
    const Char *str = value.c_str();
    std::size_t size = value.size();
    std::copy(str, str + size, GrowBuffer(size));
    return *this;
  }

  template <typename T, typename Spec, typename FillChar>
  BasicWriter &operator<<(const IntFormatSpec<T, Spec, FillChar> &spec) {
    internal::CharTraits<Char>::ConvertChar(FillChar());
    FormatInt(spec.value(), spec);
    return *this;
  }

  template <typename StringChar>
  BasicWriter &operator<<(const StrFormatSpec<StringChar> &spec) {
    const StringChar *s = spec.str();
    FormatString(s, std::char_traits<Char>::length(s), spec);
    return *this;
  }

  void Write(const std::basic_string<Char> &s, const FormatSpec &spec) {
    FormatString(s.data(), s.size(), spec);
  }

  void Clear() {
    buffer_.clear();
  }
};

template <typename Char>
template <typename StringChar>
typename BasicWriter<Char>::CharPtr BasicWriter<Char>::FormatString(
    const StringChar *s, std::size_t size, const AlignSpec &spec) {
  CharPtr out = CharPtr();
  if (spec.width() > size) {
    out = GrowBuffer(spec.width());
    Char fill = static_cast<Char>(spec.fill());
    if (spec.align() == ALIGN_RIGHT) {
      std::fill_n(out, spec.width() - size, fill);
      out += spec.width() - size;
    } else if (spec.align() == ALIGN_CENTER) {
      out = FillPadding(out, spec.width(), size, fill);
    } else {
      std::fill_n(out + size, spec.width() - size, fill);
    }
  } else {
    out = GrowBuffer(size);
  }
  std::copy(s, s + size, out);
  return out;
}

template <typename Char>
template <typename T, typename Spec>
void BasicWriter<Char>::FormatInt(T value, const Spec &spec) {
  unsigned size = 0;
  char sign = 0;
  typedef typename internal::IntTraits<T>::MainType UnsignedType;
  UnsignedType abs_value = value;
  if (internal::IsNegative(value)) {
    sign = '-';
    ++size;
    abs_value = 0 - abs_value;
  } else if (spec.sign_flag()) {
    sign = spec.plus_flag() ? '+' : ' ';
    ++size;
  }
  switch (spec.type()) {
  case 0: case 'd': {
    unsigned num_digits = internal::CountDigits(abs_value);
    CharPtr p =
        PrepareFilledBuffer(size + num_digits, spec, sign) + 1 - num_digits;
    internal::FormatDecimal(GetBase(p), abs_value, num_digits);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = spec.hash_flag();
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, spec, sign));
    n = abs_value;
    const char *digits = spec.type() == 'x' ?
        "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    if (print_prefix) {
      *p-- = spec.type();
      *p = '0';
    }
    break;
  }
  case 'b': case 'B': {
    UnsignedType n = abs_value;
    bool print_prefix = spec.hash_flag();
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 1) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, spec, sign));
    n = abs_value;
    do {
      *p-- = '0' + (n & 1);
    } while ((n >>= 1) != 0);
    if (print_prefix) {
      *p-- = spec.type();
      *p = '0';
    }
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    bool print_prefix = spec.hash_flag();
    if (print_prefix) ++size;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, spec, sign));
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    if (print_prefix)
      *p = '0';
    break;
  }
  default:
    internal::ReportUnknownType(spec.type(), "integer");
    break;
  }
}

template <typename Char>
BasicFormatter<Char> BasicWriter<Char>::Format(StringRef format) {
  BasicFormatter<Char> f(*this, format.c_str());
  return f;
}

// The default formatting function.
template <typename Char, typename T>
void Format(BasicWriter<Char> &w, const FormatSpec &spec, const T &value) {
  std::basic_ostringstream<Char> os;
  os << value;
  w.Write(os.str(), spec);
}

namespace internal {
// Formats an argument of a custom type, such as a user-defined class.
template <typename Char, typename T>
void FormatCustomArg(
    BasicWriter<Char> &w, const void *arg, const FormatSpec &spec) {
  Format(w, spec, *static_cast<const T*>(arg));
}
}

/**
  \rst
  The :cpp:class:`fmt::BasicFormatter` template provides string formatting
  functionality similar to Python's `str.format
  <http://docs.python.org/3/library/stdtypes.html#str.format>`__ function.
  The class provides operator<< for feeding formatting arguments and all
  the output is sent to a :cpp:class:`fmt::Writer` object.
  \endrst
 */
template <typename Char>
class BasicFormatter {
 private:
  BasicWriter<Char> *writer_;

  // An action used to ensure that formatting is performed before the
  // argument is destroyed.
  // Example:
  //
  //   Format("{}") << std::string("test");
  //
  // Here an Arg object wraps a temporary std::string which is destroyed at
  // the end of the full expression. Since the string object is constructed
  // before the Arg object, it will be destroyed after, so it will be alive
  // in the Arg's destructor where the action is invoked.
  // Note that the string object will not necessarily be alive when the
  // destructor of BasicFormatter is called. Otherwise we wouldn't need
  // this class.
  struct ArgAction {
    mutable BasicFormatter *formatter;

    ArgAction() : formatter(0) {}

    void operator()() const {
      if (formatter)
        formatter->CompleteFormatting();
    }
  };

  typedef typename BasicWriter<Char>::ArgInfo ArgInfo;
  typedef typename BasicWriter<Char>::template BasicArg<ArgAction> Arg;

  enum { NUM_INLINE_ARGS = 10 };
  internal::Array<ArgInfo, NUM_INLINE_ARGS> args_;  // Format arguments.

  const Char *format_;  // Format string.

  // Forbid copying from a temporary as in the following example:
  //
  //   fmt::Formatter<> f = Format("test"); // not allowed
  //
  // This is done because BasicFormatter objects should normally exist
  // only as temporaries returned by one of the formatting functions.
  FMT_DISALLOW_COPY_AND_ASSIGN(BasicFormatter);

 protected:
  const Char *TakeFormatString() {
    const Char *format = this->format_;
    this->format_ = 0;
    return format;
  }

  void CompleteFormatting() {
    if (!format_) return;
    const Char *format = format_;
    format_ = 0;
    writer_->VFormat(format, args_.size(), &args_[0]);
  }

 public:
  // Constructs a formatter with a writer to be used for output and a format
  // string.
  BasicFormatter(BasicWriter<Char> &w, const Char *format = 0)
  : writer_(&w), format_(format) {}

  // Performs formatting if the format string is non-null. The format string
  // can be null if its ownership has been transferred to another formatter.
  ~BasicFormatter() {
    CompleteFormatting();
  }

  BasicFormatter(BasicFormatter &f) : writer_(f.writer_), format_(f.format_) {
    f.format_ = 0;
  }

  // Feeds an argument to a formatter.
  BasicFormatter &operator<<(const Arg &arg) {
    arg.formatter = this;
    args_.push_back(arg);
    return *this;
  }

  operator BasicStringRef<Char>() {
    CompleteFormatting();
    return BasicStringRef<Char>(writer_->c_str(), writer_->size());
  }
};

template <typename Char>
inline std::basic_string<Char> str(const BasicWriter<Char> &f) {
  return f.str();
}

template <typename Char>
inline const Char *c_str(const BasicWriter<Char> &f) { return f.c_str(); }

/**
  Converts a string reference an `std::string`.
 */
inline std::string str(StringRef s) {
  return std::string(s.c_str(), s.size());
}

/**
  Returns the pointer to a C string.
 */
inline const char *c_str(StringRef s) {
  return s.c_str();
}

inline std::wstring str(WStringRef s) {
  return std::wstring(s.c_str(), s.size());
}

inline const wchar_t *c_str(WStringRef s) {
  return s.c_str();
}

/**
  A sink that discards all output written to it.
 */
class NullSink {
 public:
  /** Discards the output. */
  template <typename Char>
  void operator()(const BasicWriter<Char> &) const {}
};

/**
  \rst
  A formatter that sends output to a sink. Objects of this class normally
  exist only as temporaries returned by one of the formatting functions.
  You can use this class to create your own functions similar to
  :cpp:func:`fmt::Format()`.

  **Example**::

    struct ErrorSink {
      void operator()(const fmt::Writer &w) const {
        fmt::Print("Error: {}\n") << w.str();
      }
    };

    // Formats an error message and prints it to stdout.
    fmt::Formatter<ErrorSink> ReportError(const char *format) {
      fmt::Formatter f<ErrorSink>(format);
      return f;
    }

    ReportError("File not found: {}") << path;
  \endrst
 */
template <typename Sink = NullSink, typename Char = char>
class Formatter : private Sink, public BasicFormatter<Char> {
 private:
  BasicWriter<Char> writer_;
  bool inactive_;

  FMT_DISALLOW_COPY_AND_ASSIGN(Formatter);

 public:
  /**
    \rst
    Constructs a formatter with a format string and a sink.
    The sink should be an unary function object that takes a const
    reference to :cpp:class:`fmt::BasicWriter`, representing the
    formatting output, as an argument. See :cpp:class:`fmt::NullSink`
    and :cpp:class:`fmt::FileSink` for examples of sink classes.
    \endrst
  */
  explicit Formatter(BasicStringRef<Char> format, Sink s = Sink())
  : Sink(s), BasicFormatter<Char>(writer_, format.c_str()),
    inactive_(false) {
  }

  /**
    \rst
    A "move" constructor. Constructs a formatter transferring the format
    string from other to this object. This constructor is used to return
    a formatter object from a formatting function since the copy constructor
    taking a const reference is disabled to prevent misuse of the API.
    It is not implemented as a move constructor for compatibility with
    pre-C++11 compilers, but should be treated as such.

    **Example**::

      fmt::Formatter<> Format(fmt::StringRef format) {
        fmt::Formatter<> f(format);
        return f;
      }
    \endrst
   */
  Formatter(Formatter &other)
  : Sink(other), BasicFormatter<Char>(writer_, other.TakeFormatString()),
    inactive_(false) {
    other.inactive_ = true;
  }

  /**
    Performs the formatting, sends the output to the sink and destroys
    the object.
   */
  ~Formatter() FMT_NOEXCEPT(false) {
    if (!inactive_) {
      this->CompleteFormatting();
      (*this)(writer_);
    }
  }
};

/**
  \rst
  Formats a string similarly to Python's `str.format
  <http://docs.python.org/3/library/stdtypes.html#str.format>`__.
  Returns a temporary formatter object that accepts arguments via
  operator ``<<``.

  *format* is a format string that contains literal text and replacement
  fields surrounded by braces ``{}``. The formatter object replaces the
  fields with formatted arguments and stores the output in a memory buffer.
  The content of the buffer can be converted to ``std::string`` with
  :cpp:func:`fmt::str()` or accessed as a C string with
  :cpp:func:`fmt::c_str()`.

  **Example**::

    std::string message = str(Format("The answer is {}") << 42);

  See also `Format String Syntax`_.
  \endrst
 */
inline Formatter<> Format(StringRef format) {
  Formatter<> f(format);
  return f;
}

inline Formatter<NullSink, wchar_t> Format(WStringRef format) {
  Formatter<NullSink, wchar_t> f(format);
  return f;
}

/**
  A sink that gets the error message corresponding to a system error code
  as given by errno and throws SystemError.
 */
class SystemErrorSink {
 private:
  int error_code_;

 public:
  explicit SystemErrorSink(int error_code) : error_code_(error_code) {}

  void operator()(const Writer &w) const;
};

/**
  Formats a message and throws SystemError with the description of the form
  "<message>: <system-message>", where <message> is the formatted message and
  <system-message> is the system message corresponding to the error code.
  error_code is a system error code as given by errno.
 */
inline Formatter<SystemErrorSink> ThrowSystemError(
    int error_code, StringRef format) {
  Formatter<SystemErrorSink> f(format, SystemErrorSink(error_code));
  return f;
}

// Reports a system error without throwing an exception.
// Can be used to report errors from destructors.
void ReportSystemError(int error_code, StringRef message) FMT_NOEXCEPT(true);

#ifdef _WIN32

/**
  A sink that gets the error message corresponding to a Windows error code
  as given by GetLastError and throws SystemError.
 */
class WinErrorSink {
 private:
  int error_code_;

 public:
  explicit WinErrorSink(int error_code) : error_code_(error_code) {}

  void operator()(const Writer &w) const;
};

/**
  Formats a message and throws SystemError with the description of the form
  "<message>: <system-message>", where <message> is the formatted message and
  <system-message> is the system message corresponding to the error code.
  error_code is a Windows error code as given by GetLastError.
 */
inline Formatter<WinErrorSink> ThrowWinError(int error_code, StringRef format) {
  Formatter<WinErrorSink> f(format, WinErrorSink(error_code));
  return f;
}

// Reports a Windows error without throwing an exception.
// Can be used to report errors from destructors.
void ReportWinError(int error_code, StringRef message) FMT_NOEXCEPT(true);

#endif

/** A sink that writes output to a file. */
class FileSink {
 private:
  std::FILE *file_;

 public:
  explicit FileSink(std::FILE *f) : file_(f) {}

  /** Writes the output to a file. */
  void operator()(const BasicWriter<char> &w) const {
    if (std::fwrite(w.data(), w.size(), 1, file_) == 0)
      ThrowSystemError(errno, "cannot write to file");
  }
};

// Formats a string and prints it to stdout.
// Example:
//   Print("Elapsed time: {0:.2f} seconds") << 1.23;
inline Formatter<FileSink> Print(StringRef format) {
  Formatter<FileSink> f(format, FileSink(stdout));
  return f;
}

// Formats a string and prints it to a file.
// Example:
//   Print(stderr, "Don't {}!") << "panic";
inline Formatter<FileSink> Print(std::FILE *file, StringRef format) {
  Formatter<FileSink> f(format, FileSink(file));
  return f;
}

enum Color { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

/**
  A sink that writes output to a terminal using ANSI escape sequences
  to specify color.
 */
class ANSITerminalSink {
 private:
  std::FILE *file_;
  Color color_;

 public:
  ANSITerminalSink(std::FILE *f, Color c) : file_(f), color_(c) {}

  /**
    Writes the output to a terminal using ANSI escape sequences to
    specify color.
   */
  void operator()(const BasicWriter<char> &w) const;
};

/**
  Formats a string and prints it to stdout using ANSI escape sequences
  to specify color (experimental).
  Example:
    PrintColored(fmt::RED, "Elapsed time: {0:.2f} seconds") << 1.23;
 */
inline Formatter<ANSITerminalSink> PrintColored(Color c, StringRef format) {
  Formatter<ANSITerminalSink> f(format, ANSITerminalSink(stdout, c));
  return f;
}

#if FMT_USE_VARIADIC_TEMPLATES && FMT_USE_RVALUE_REFERENCES

template<typename... Args>
inline Writer Format(StringRef format, const Args & ... args) {
  Writer w;
  w.Format(format, args...);
  return std::move(w);
}

template<typename... Args>
inline WWriter Format(WStringRef format, const Args & ... args) {
  WWriter w;
  w.Format(format, args...);
  return std::move(w);
}

template<typename... Args>
void Print(StringRef format, const Args & ... args) {
  Writer w;
  w.Format(format, args...);
  std::fwrite(w.data(), 1, w.size(), stdout);
}

template<typename... Args>
void Print(std::FILE *f, StringRef format, const Args & ... args) {
  Writer w;
  w.Format(format, args...);
  std::fwrite(w.data(), 1, w.size(), f);
}

#endif  // FMT_USE_VARIADIC_TEMPLATES && FMT_USE_RVALUE_REFERENCES

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
  char *FormatDecimal(ULongLong value) {
    char *buffer_end = buffer_ + BUFFER_SIZE - 1;
    while (value >= 100) {
      // Integer division is slow so do it for a group of two digits instead
      // of for every digit. The idea comes from the talk by Alexandrescu
      // "Three Optimization Tips for C++". See speed-test for a comparison.
      unsigned index = (value % 100) * 2;
      value /= 100;
      *--buffer_end = internal::DIGITS[index + 1];
      *--buffer_end = internal::DIGITS[index];
    }
    if (value < 10) {
      *--buffer_end = static_cast<char>('0' + value);
      return buffer_end;
    }
    unsigned index = static_cast<unsigned>(value * 2);
    *--buffer_end = internal::DIGITS[index + 1];
    *--buffer_end = internal::DIGITS[index];
    return buffer_end;
  }

  void FormatSigned(LongLong value) {
    ULongLong abs_value = value;
    bool negative = value < 0;
    if (negative)
      abs_value = 0 - value;
    str_ = FormatDecimal(abs_value);
    if (negative)
      *--str_ = '-';
  }

 public:
  explicit FormatInt(int value) { FormatSigned(value); }
  explicit FormatInt(long value) { FormatSigned(value); }
  explicit FormatInt(LongLong value) { FormatSigned(value); }
  explicit FormatInt(unsigned value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(unsigned long value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(ULongLong value) : str_(FormatDecimal(value)) {}

  /**
    Returns the number of characters written to the output buffer.
   */
  std::size_t size() const { return buffer_ - str_ + BUFFER_SIZE - 1; }

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
    Returns the content of the output buffer as an `std::string`.
   */
  std::string str() const { return std::string(str_, size()); }
};

// Formats a decimal integer value writing into buffer and returns
// a pointer to the end of the formatted string. This function doesn't
// write a terminating null character.
template <typename T>
inline void FormatDec(char *&buffer, T value) {
  typename internal::IntTraits<T>::MainType abs_value = value;
  if (internal::IsNegative(value)) {
    *buffer++ = '-';
    abs_value = 0 - abs_value;
  }
  if (abs_value < 100) {
    if (abs_value < 10) {
      *buffer++ = static_cast<char>('0' + abs_value);
      return;
    }
    unsigned index = static_cast<unsigned>(abs_value * 2);
    *buffer++ = internal::DIGITS[index];
    *buffer++ = internal::DIGITS[index + 1];
    return;
  }
  unsigned num_digits = internal::CountDigits(abs_value);
  internal::FormatDecimal(buffer, abs_value, num_digits);
  buffer += num_digits;
}
}

// Restore warnings.
#if FMT_GCC_VERSION >= 406
# pragma GCC diagnostic pop
#elif _MSC_VER
# pragma warning(pop)
#endif

#endif  // FMT_FORMAT_H_
