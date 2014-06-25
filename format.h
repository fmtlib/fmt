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
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>

#if _SECURE_SCL
# include <iterator>
#endif

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
#else
# define FMT_HAS_FEATURE(x) 0
#endif

#ifdef __has_builtin
# define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
# define FMT_HAS_BUILTIN(x) 0
#endif

#ifdef _MSC_VER
# define FMT_MSC_VER _MSC_VER
#else
# define FMT_MSC_VER 0
#endif

#ifndef FMT_USE_VARIADIC_TEMPLATES
// Variadic templates are available in GCC since version 4.4
// (http://gcc.gnu.org/projects/cxx0x.html) and in Visual C++
// since version 2013.
# define FMT_USE_VARIADIC_TEMPLATES \
   (FMT_HAS_FEATURE(cxx_variadic_templates) || \
       (FMT_GCC_VERSION >= 404 && __cplusplus >= 201103) || FMT_MSC_VER >= 1800)
#endif

#ifndef FMT_USE_RVALUE_REFERENCES
// Don't use rvalue references when compiling with clang and an old libstdc++
// as the latter doesn't provide std::move.
# if defined(FMT_GNUC_LIBSTD_VERSION) && FMT_GNUC_LIBSTD_VERSION <= 402
#  define FMT_USE_RVALUE_REFERENCES 0
# else
#  define FMT_USE_RVALUE_REFERENCES \
    (FMT_HAS_FEATURE(cxx_rvalue_references) || \
        (FMT_GCC_VERSION >= 403 && __cplusplus >= 201103) || FMT_MSC_VER >= 1600)
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

#if FMT_MSC_VER
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

/**
  A formatting error such as invalid format string.
*/
class FormatError : public std::runtime_error {
public:
  explicit FormatError(const std::string &message)
  : std::runtime_error(message) {}
};

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
    Grow(size_ + num_elements);
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

int SignBitNoInline(double value);

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
    buffer[num_digits] = DIGITS[index + 1];
    buffer[num_digits - 1] = DIGITS[index];
    num_digits -= 2;
  }
  if (value < 10) {
    *buffer = static_cast<char>('0' + value);
    return;
  }
  unsigned index = static_cast<unsigned>(value * 2);
  buffer[1] = DIGITS[index + 1];
  buffer[0] = DIGITS[index];
}

template <typename Char, typename T>
void FormatCustomArg(void *writer, const void *arg, const FormatSpec &spec);

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

struct SimpleErrorReporter {
  void operator()(const void *, fmt::StringRef message) const {
    throw fmt::FormatError(message);
  }
};

// Throws Exception(message) if format contains '}', otherwise throws
// FormatError reporting unmatched '{'. The idea is that unmatched '{'
// should override other errors.
template <typename Char>
struct FormatErrorReporter {
  int num_open_braces;
  void operator()(const Char *s, fmt::StringRef message) const;
};

// Parses a nonnegative integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
template <typename Char>
int ParseNonnegativeInt(
  const Char *&s, const char *&error) FMT_NOEXCEPT(true);

// Computes max(Arg, 1) at compile time. It is used to avoid errors about
// allocating an array of 0 size.
template <unsigned Arg>
struct NonZero {
  enum { VALUE = Arg };
};

template <>
struct NonZero<0> {
  enum { VALUE = 1 };
};

// Information about a format argument. It is a POD type to allow
// storage in internal::Array.
struct ArgInfo {
  enum Type {
    // Integer types should go first,
    INT, UINT, LONG_LONG, ULONG_LONG, LAST_INTEGER_TYPE = ULONG_LONG,
    // followed by floating-point types.
    DOUBLE, LONG_DOUBLE, LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };
  Type type;

  template <typename Char>
  struct StringValue {
    const Char *value;
    std::size_t size;
  };

  typedef void (*FormatFunc)(
      void *writer, const void *arg, const FormatSpec &spec);

  struct CustomValue {
    const void *value;
    FormatFunc format;
  };

  union {
    int int_value;
    unsigned uint_value;
    double double_value;
    LongLong long_long_value;
    ULongLong ulong_long_value;
    long double long_double_value;
    const void *pointer_value;
    StringValue<char> string;
    StringValue<wchar_t> wstring;
    CustomValue custom;
  };
};
}  // namespace internal

/**
  An error returned by an operating system or a language runtime,
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
  int precision() const { return -1; }

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

  int precision() const { return -1; }
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
  int precision_;
  char type_;

  FormatSpec(
    unsigned width = 0, char type = 0, wchar_t fill = ' ')
  : AlignSpec(width, fill), flags_(0), precision_(-1), type_(type) {}

  bool sign_flag() const { return (flags_ & SIGN_FLAG) != 0; }
  bool plus_flag() const { return (flags_ & PLUS_FLAG) != 0; }
  bool hash_flag() const { return (flags_ & HASH_FLAG) != 0; }

  int precision() const { return precision_; }

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

    Writer out;
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

class ArgList {
 private:
  const internal::ArgInfo *args_;
  std::size_t size_;

public:
  ArgList() : size_(0) {}
  ArgList(const internal::ArgInfo *args, std::size_t size)
  : args_(args), size_(size) {}

  std::size_t size() const { return size_; }

  const internal::ArgInfo &operator[](std::size_t index) const {
    return args_[index];
  }
};

// Generates a comma-separated list by applying f to numbers 1..n.
#define FMT_GEN(n, f) FMT_GEN##n(f)
#define FMT_GEN1(f) f(1)
#define FMT_GEN2(f) FMT_GEN1(f), f(2)
#define FMT_GEN3(f) FMT_GEN2(f), f(3)
#define FMT_GEN4(f) FMT_GEN3(f), f(4)
#define FMT_GEN5(f) FMT_GEN4(f), f(5)
#define FMT_GEN6(f) FMT_GEN5(f), f(6)
#define FMT_GEN7(f) FMT_GEN6(f), f(7)
#define FMT_GEN8(f) FMT_GEN7(f), f(8)
#define FMT_GEN9(f) FMT_GEN8(f), f(9)

#define FMT_MAKE_TEMPLATE_ARG(n) typename T##n
#define FMT_MAKE_ARG(n) const T##n &v##n
#define FMT_MAKE_REF(n) MakeArg(v##n)

#define FMT_TEMPLATE(func_name, n) \
  template <FMT_GEN(n, FMT_MAKE_TEMPLATE_ARG)> \
  inline void func_name(BasicStringRef<Char> format, FMT_GEN(n, FMT_MAKE_ARG)) { \
    const fmt::internal::ArgInfo args[] = {FMT_GEN(n, FMT_MAKE_REF)}; \
    func_name(format, fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  }

// Defines a variadic function returning void.
#define FMT_VARIADIC_VOID(func_name) \
  FMT_TEMPLATE(format, 1) FMT_TEMPLATE(format, 2) FMT_TEMPLATE(format, 3) \
  FMT_TEMPLATE(format, 4) FMT_TEMPLATE(format, 5) FMT_TEMPLATE(format, 6) \
  FMT_TEMPLATE(format, 7) FMT_TEMPLATE(format, 8) FMT_TEMPLATE(format, 9)

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

  typedef internal::ArgInfo Arg;

  static const Arg DUMMY_ARG;

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

  // Prepare a buffer for integer formatting.
  CharPtr PrepareBufferForInt(unsigned num_digits,
      const EmptySpec &, const char *prefix, unsigned prefix_size) {
    unsigned size = prefix_size + num_digits;
    CharPtr p = GrowBuffer(size);
    std::copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }

  template <typename Spec>
  CharPtr PrepareBufferForInt(unsigned num_digits,
    const Spec &spec, const char *prefix, unsigned prefix_size);

  // Formats an integer.
  template <typename T, typename Spec>
  void FormatInt(T value, const Spec &spec);

  // Formats a floating-point number (double or long double).
  template <typename T>
  void FormatDouble(T value, const FormatSpec &spec);

  // Formats a string.
  template <typename StringChar>
  CharPtr FormatString(
      const StringChar *s, std::size_t size, const AlignSpec &spec);

  template <typename StringChar>
  void FormatString(
      const Arg::StringValue<StringChar> &str, const FormatSpec &spec);

    // This method is private to disallow writing a wide string to a
  // char stream and vice versa. If you want to print a wide string
  // as a pointer as std::ostream does, cast it to const void*.
  // Do not implement!
  void operator<<(typename internal::CharTraits<Char>::UnsupportedStrType);

  static ULongLong GetIntValue(const Arg &arg);

  // An argument action that does nothing.
  struct NullArgAction {
    void operator()() const {}
  };

  // A wrapper around a format argument.
  template <typename Action = NullArgAction>
  class BasicArg : public Action, public Arg {
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
    using Arg::type;

    BasicArg() {}
    // TODO: unsigned char & signed char
    BasicArg(short value) { type = Arg::INT; Arg::int_value = value; }
    BasicArg(unsigned short value) {
      type = Arg::UINT;
      Arg::uint_value = value;
    }
    BasicArg(int value) { type = Arg::INT; Arg::int_value = value; }
    BasicArg(unsigned value) { type = Arg::UINT; Arg::uint_value = value; }
    BasicArg(long value) {
      if (sizeof(long) == sizeof(int)) {
        type = Arg::INT;
        Arg::int_value = static_cast<int>(value);
      } else {
        type = Arg::LONG_LONG;
        Arg::long_long_value = value;
      }
    }
    BasicArg(unsigned long value) {
      if (sizeof(unsigned long) == sizeof(unsigned)) {
        type = Arg::UINT;
        Arg::uint_value = static_cast<unsigned>(value);
      } else {
        type = Arg::ULONG_LONG;
        Arg::ulong_long_value = value;
      }
    }
    BasicArg(LongLong value) {
      type = Arg::LONG_LONG;
      Arg::long_long_value = value;
    }
    BasicArg(ULongLong value) {
      type = Arg::ULONG_LONG;
      Arg::ulong_long_value = value;
    }
    BasicArg(float value) { type = Arg::DOUBLE; Arg::double_value = value; }
    BasicArg(double value) { type = Arg::DOUBLE; Arg::double_value = value; }
    BasicArg(long double value) {
      type = Arg::LONG_DOUBLE;
      Arg::long_double_value = value;
    }
    BasicArg(char value) { type = Arg::CHAR; Arg::int_value = value; }
    BasicArg(wchar_t value) {
      type = Arg::CHAR;
      Arg::int_value = internal::CharTraits<Char>::ConvertChar(value);
    }

    BasicArg(const char *value) {
      type = Arg::STRING;
      Arg::string.value = value;
      Arg::string.size = 0;
    }

    BasicArg(const wchar_t *value) {
      type = Arg::WSTRING;
      Arg::wstring.value = value;
      Arg::wstring.size = 0;
    }

    BasicArg(Char *value) {
      type = Arg::STRING;
      Arg::string.value = value;
      Arg::string.size = 0;
    }

    BasicArg(const void *value) {
      type = Arg::POINTER;
      Arg::pointer_value = value;
    }
    BasicArg(void *value) { type = Arg::POINTER; Arg::pointer_value = value; }

    BasicArg(const std::basic_string<Char> &value) {
      type = Arg::STRING;
      Arg::string.value = value.c_str();
      Arg::string.size = value.size();
    }

    BasicArg(BasicStringRef<Char> value) {
      type = Arg::STRING;
      Arg::string.value = value.c_str();
      Arg::string.size = value.size();
    }

    template <typename T>
    BasicArg(const T &value) {
      type = Arg::CUSTOM;
      Arg::custom.value = &value;
      Arg::custom.format = &internal::FormatCustomArg<Char, T>;
    }

    // The destructor is declared noexcept(false) because the action may throw
    // an exception.
    ~BasicArg() FMT_NOEXCEPT(false) {
      // Invoke the action.
      (*this)();
    }
  };

  // Format string parser.
  class FormatParser {
   private:
    ArgList args_;
    int next_arg_index_;
    fmt::internal::FormatErrorReporter<Char> report_error_;

    // Parses argument index and returns an argument with this index.
    const Arg &ParseArgIndex(const Char *&s);

    void CheckSign(const Char *&s, const Arg &arg);

   public:
    void Format(BasicWriter<Char> &writer,
      BasicStringRef<Char> format, const ArgList &args);
  };

  // Printf format string parser.
  class PrintfParser {
   private:
    ArgList args_;
    int next_arg_index_;

    void ParseFlags(FormatSpec &spec, const Char *&s);

    // Parses argument index, flags and width and returns the parsed
    // argument index.
    unsigned ParseHeader(const Char *&s, FormatSpec &spec, const char *&error);

    const Arg &HandleArgIndex(unsigned arg_index, const char *&error);

   public:
    void Format(BasicWriter<Char> &writer,
      BasicStringRef<Char> format, const ArgList &args);
  };

 public:
  /**
    Constructs a ``BasicWriter`` object.
   */
  BasicWriter() {}

#if FMT_USE_RVALUE_REFERENCES
  /**
    Constructs a ``BasicWriter`` object moving the content of the other
    object to it.
   */
  BasicWriter(BasicWriter &&other) : buffer_(std::move(other.buffer_)) {}

  /**
    Moves the content of the other ``BasicWriter`` object to this one.
   */
  BasicWriter& operator=(BasicWriter &&other) {
    assert(this != &other);
    buffer_ = std::move(other.buffer_);
    return *this;
  }
#endif

  /**
    Returns the total number of characters written.
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

  inline void format(BasicStringRef<Char> format, const ArgList &args) {
    FormatParser().Format(*this, format, args);
  }

  inline void printf(BasicStringRef<Char> format, const ArgList &args) {
    PrintfParser().Format(*this, format, args);
  }

  /**
    \rst
    Formats a string sending the output to the writer. Arguments are
    accepted through the returned :cpp:class:`fmt::BasicFormatter` object
    using operator ``<<``.

    **Example**::

       Writer out;
       out.Format("Current point:\n");
       out.Format("({:+f}, {:+f})") << -3.14 << 3.14;

    This will write the following output to the ``out`` object:

    .. code-block:: none

       Current point:
       (-3.140000, +3.140000)

    The output can be accessed using :meth:`data`, :meth:`c_str` or :meth:`str`
    methods.

    See also `Format String Syntax`_.
    \endrst
   */
  BasicFormatter<Char> Format(StringRef format);

#if FMT_USE_VARIADIC_TEMPLATES
  /**
    \rst
    Formats a string sending the output to the writer.

    This version of the Format method uses C++11 features such as
    variadic templates and rvalue references. For C++98 version, see
    the overload taking a single ``StringRef`` argument above.

    **Example**::

       Writer out;
       out.Format("Current point:\n");
       out.Format("({:+f}, {:+f})", -3.14, 3.14);

    This will write the following output to the ``out`` object:

    .. code-block:: none

       Current point:
       (-3.140000, +3.140000)

    The output can be accessed using :meth:`data`, :meth:`c_str` or :meth:`str`
    methods.

    See also `Format String Syntax`_.
    \endrst
   */
  template<typename... Args>
  void Format(BasicStringRef<Char> format, const Args & ... args) {
    BasicArg<> arg_array[] = {args...};
    this->format(format, ArgList(arg_array, sizeof...(Args)));
  }

  template<typename... Args>
  void printf(BasicStringRef<Char> format, const Args & ... args) {
    BasicArg<> arg_array[internal::NonZero<sizeof...(Args)>::VALUE] = {args...};
    this->printf(format, ArgList(arg_array, sizeof...(Args)));
  }
  
#else
  FMT_VARIADIC_VOID(format)
#endif

  template <typename T>
  static Arg MakeArg(const T &arg) { return BasicArg<>(arg); }

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
    FormatDouble(value, FormatSpec());
    return *this;
  }

  /**
    Formats *value* using the general format for floating-point numbers
    (``'g'``) and writes it to the stream.
   */
  BasicWriter &operator<<(long double value) {
    FormatDouble(value, FormatSpec());
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
template <typename Spec>
typename fmt::BasicWriter<Char>::CharPtr
  fmt::BasicWriter<Char>::PrepareBufferForInt(
    unsigned num_digits, const Spec &spec,
    const char *prefix, unsigned prefix_size) {
  unsigned width = spec.width();
  Alignment align = spec.align();
  if (spec.precision() > static_cast<int>(num_digits)) {
    // Octal prefix '0' is counted as a digit, so ignore it if precision
    // is specified.
    if (prefix_size > 0 && prefix[prefix_size - 1] == '0')
      --prefix_size;
    unsigned number_size = prefix_size + spec.precision();
    AlignSpec subspec(number_size, '0', ALIGN_NUMERIC);
    if (number_size >= width)
      return PrepareBufferForInt(num_digits, subspec, prefix, prefix_size);
    buffer_.reserve(width);
    unsigned fill_size = width - number_size;
    if (align != ALIGN_LEFT) {
      CharPtr p = GrowBuffer(fill_size);
      std::fill(p, p + fill_size, spec.fill());
    }
    CharPtr result = PrepareBufferForInt(num_digits, subspec, prefix, prefix_size);
    if (align == ALIGN_LEFT) {
      CharPtr p = GrowBuffer(fill_size);
      std::fill(p, p + fill_size, spec.fill());
    }
    return result;
  }
  unsigned size = prefix_size + num_digits;
  if (width <= size) {
    CharPtr p = GrowBuffer(size);
    std::copy(prefix, prefix + prefix_size, p);
    return p + size - 1;
  }
  CharPtr p = GrowBuffer(width);
  CharPtr end = p + width;
  // TODO: error if fill is not convertible to Char
  Char fill = static_cast<Char>(spec.fill());
  if (align == ALIGN_LEFT) {
    std::copy(prefix, prefix + prefix_size, p);
    p += size;
    std::fill(p, end, fill);
  } else if (align == ALIGN_CENTER) {
    p = FillPadding(p, width, size, fill);
    std::copy(prefix, prefix + prefix_size, p);
    p += size;
  } else {
    if (align == ALIGN_NUMERIC) {
      if (prefix_size != 0) {
        p = std::copy(prefix, prefix + prefix_size, p);
        size -= prefix_size;
      }
    } else {
      std::copy(prefix, prefix + prefix_size, end - size);
    }
    std::fill(p, end - size, fill);
    p = end;
  }
  return p - 1;
}

template <typename Char>
template <typename T, typename Spec>
void BasicWriter<Char>::FormatInt(T value, const Spec &spec) {
  unsigned prefix_size = 0;
  typedef typename internal::IntTraits<T>::MainType UnsignedType;
  UnsignedType abs_value = value;
  char prefix[4] = "";
  if (internal::IsNegative(value)) {
    prefix[0] = '-';
    ++prefix_size;
    abs_value = 0 - abs_value;
  } else if (spec.sign_flag()) {
    prefix[0] = spec.plus_flag() ? '+' : ' ';
    ++prefix_size;
  }
  switch (spec.type()) {
  case 0: case 'd': {
    unsigned num_digits = internal::CountDigits(abs_value);
    CharPtr p = PrepareBufferForInt(
      num_digits, spec, prefix, prefix_size) + 1 - num_digits;
    internal::FormatDecimal(GetBase(p), abs_value, num_digits);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    if (spec.hash_flag()) {
      prefix[prefix_size++] = '0';
      prefix[prefix_size++] = spec.type();
    }
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 4) != 0);
    Char *p = GetBase(PrepareBufferForInt(
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
    if (spec.hash_flag()) {
      prefix[prefix_size++] = '0';
      prefix[prefix_size++] = spec.type();
    }
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 1) != 0);
    Char *p = GetBase(PrepareBufferForInt(num_digits, spec, prefix, prefix_size));
    n = abs_value;
    do {
      *p-- = '0' + (n & 1);
    } while ((n >>= 1) != 0);
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    if (spec.hash_flag())
      prefix[prefix_size++] = '0';
    unsigned num_digits = 0;
    do {
      ++num_digits;
    } while ((n >>= 3) != 0);
    Char *p = GetBase(PrepareBufferForInt(
      num_digits, spec, prefix, prefix_size));
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
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
void FormatCustomArg(void *writer, const void *arg, const FormatSpec &spec) {
  Format(*static_cast<BasicWriter<Char>*>(writer),
      spec, *static_cast<const T*>(arg));
}
}

/**
  \rst
  The :cpp:class:`fmt::BasicFormatter` template provides operator<< for
  feeding arbitrary arguments to the :cpp:func:`fmt::Format()` function.
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

  typedef typename internal::ArgInfo ArgInfo;
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
    writer_->format(format, ArgList(&args_[0], args_.size()));
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
  Converts a string reference to `std::string`.
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
  <http://docs.python.org/3/library/stdtypes.html#str.format>`__ function.
  Returns a temporary :cpp:class:`fmt::Formatter` object that accepts arguments
  via operator ``<<``.

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
  \rst
  Formats a message and throws :cpp:class:`fmt::SystemError` with
  the description of the form "*<message>*: *<system-message>*",
  where *<message>* is the formatted message and *<system-message>*
  is the system message corresponding to the error code.
  *error_code* is a system error code as given by ``errno``.
  \endrst
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
  \rst
  Formats a message and throws :cpp:class:`fmt::SystemError` with
  the description of the form "*<message>*: *<system-message>*",
  where *<message>* is the formatted message and *<system-message>*
  is the system message corresponding to the error code.
  *error_code* is a Windows error code as given by ``GetLastError``.

  This function is only available on Windows.
  \endrst
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

/**
  \rst
  Formats a string and writes the result to ``stdout``.

  **Example**::

    Print("Elapsed time: {0:.2f} seconds") << 1.23;
  \endrst
 */
inline Formatter<FileSink> Print(StringRef format) {
  Formatter<FileSink> f(format, FileSink(stdout));
  return f;
}

/**
  \rst
  Formats a string and writes the result to a file.

  **Example**::

    Print(stderr, "Don't {}!") << "panic";
  \endrst
 */
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

/**
  \rst
  Formats a string similarly to Python's `str.format
  <http://docs.python.org/3/library/stdtypes.html#str.format>`__ function
  and returns an :cpp:class:`fmt::BasicWriter` object containing the output.

  This version of the Format function uses C++11 features such as
  variadic templates and rvalue references. For C++98 version, see
  the :cpp:func:`fmt::Format()` overload above.

  *format* is a format string that contains literal text and replacement
  fields surrounded by braces ``{}``. The formatter object replaces the
  fields with formatted arguments and stores the output in a memory buffer.
  The content of the buffer can be converted to ``std::string`` with
  :cpp:func:`fmt::str()` or accessed as a C string with
  :cpp:func:`fmt::c_str()`.

  **Example**::

    std::string message = str(Format("The answer is {}", 42));

  See also `Format String Syntax`_.
  \endrst
 */
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

template<typename... Args>
inline Writer sprintf(StringRef format, const Args & ... args) {
  Writer w;
  w.printf(format, args...);
  return std::move(w);
}

template<typename... Args>
void printf(StringRef format, const Args & ... args) {
  Writer w;
  w.printf(format, args...);
  std::fwrite(w.data(), 1, w.size(), stdout);
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

#if FMT_GCC_VERSION
// Use the system_header pragma to suppress warnings about variadic macros
// because suppressing -Wvariadic-macros with the diagnostic pragma doesn't work.
# pragma GCC system_header
#endif

#define FMT_CONCATENATE(arg1, arg2)   FMT_CONCATENATE1(arg1, arg2)
#define FMT_CONCATENATE1(arg1, arg2)  FMT_CONCATENATE2(arg1, arg2)
#define FMT_CONCATENATE2(arg1, arg2)  arg1##arg2

#define FMT_EXPAND(args) args
#define FMT_FOR_EACH_1(func, x, ...) func(x, 1)
#define FMT_FOR_EACH_2(func, x, ...) \
  func(x, 2), FMT_EXPAND(FMT_FOR_EACH_1(func, __VA_ARGS__))
#define FMT_FOR_EACH_3(func, x, ...) \
  func(x, 3), FMT_EXPAND(FMT_FOR_EACH_2(func, __VA_ARGS__))
#define FMT_FOR_EACH_4(func, x, ...) \
  func(x, 4), FMT_EXPAND(FMT_FOR_EACH_3(func, __VA_ARGS__))
#define FMT_FOR_EACH_5(func, x, ...) \
  func(x, 5), FMT_EXPAND(FMT_FOR_EACH_4(func, __VA_ARGS__))
#define FMT_FOR_EACH_6(func, x, ...) \
  func(x, 6), FMT_EXPAND(FMT_FOR_EACH_5(func, __VA_ARGS__))
#define FMT_FOR_EACH_7(func, x, ...) \
  func(x, 7), FMT_EXPAND(FMT_FOR_EACH_6(func, __VA_ARGS__))
#define FMT_FOR_EACH_8(func, x, ...) \
  func(x, 8), FMT_EXPAND(FMT_FOR_EACH_7(func, __VA_ARGS__))

#define FMT_FOR_EACH_NARG(...) FMT_FOR_EACH_NARG_(__VA_ARGS__, FMT_FOR_EACH_RSEQ_N())
#define FMT_FOR_EACH_NARG_(...) FMT_EXPAND(FMT_FOR_EACH_ARG_N(__VA_ARGS__))
#define FMT_FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define FMT_FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FMT_FOR_EACH_(N, func, ...) \
  FMT_CONCATENATE(FMT_FOR_EACH_, N)(func, __VA_ARGS__)
#define FMT_FOR_EACH(func, ...) \
  FMT_EXPAND(FMT_FOR_EACH_(FMT_FOR_EACH_NARG(__VA_ARGS__), func, __VA_ARGS__))

#define FMT_ADD_ARG_NAME(type, index) type arg##index
#define FMT_GET_ARG_NAME(type, index) arg##index
#define FMT_MAKE_ARG(arg, index) fmt::Writer::MakeArg(arg)

#define FMT_VARIADIC(return_type, func_name, ...) \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__)) { \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList()); \
  } \
  template <typename T1> \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
      const T1 &v1) { \
    const fmt::internal::ArgInfo args[] = {FMT_FOR_EACH(FMT_MAKE_ARG, v1)}; \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  } \
  template <typename T1, typename T2> \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
      const T1 &v1, const T2 &v2) { \
    const fmt::internal::ArgInfo args[] = {FMT_FOR_EACH(FMT_MAKE_ARG, v1, v2)}; \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  } \
  template <typename T1, typename T2, typename T3> \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
      const T1 &v1, const T2 &v2, const T3 &v3) { \
    const fmt::internal::ArgInfo args[] = { \
      FMT_FOR_EACH(FMT_MAKE_ARG, v1, v2, v3) \
    }; \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  } \
  template <typename T1, typename T2, typename T3, typename T4> \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
    const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4) { \
    const fmt::internal::ArgInfo args[] = { \
      FMT_FOR_EACH(FMT_MAKE_ARG, v1, v2, v3, v4) \
    }; \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  } \
  template <typename T1, typename T2, typename T3, typename T4, typename T5> \
  inline return_type func_name(FMT_FOR_EACH(FMT_ADD_ARG_NAME, __VA_ARGS__), \
    const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4, const T5 &v5) { \
    const fmt::internal::ArgInfo args[] = { \
      FMT_FOR_EACH(FMT_MAKE_ARG, v1, v2, v3, v4, v5) \
    }; \
    return func_name(FMT_FOR_EACH(FMT_GET_ARG_NAME, __VA_ARGS__), \
      fmt::ArgList(args, sizeof(args) / sizeof(*args))); \
  }

// Restore warnings.
#if FMT_GCC_VERSION >= 406
# pragma GCC diagnostic pop
#elif FMT_MSC_VER
# pragma warning(pop)
#endif

#endif  // FMT_FORMAT_H_
