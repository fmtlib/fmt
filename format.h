/*
 String formatting library for C++

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

#ifndef FORMAT_H_
#define FORMAT_H_

#include <stdint.h>

#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <sstream>

#ifdef __GNUC__
# define FMT_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#endif

// Compatibility with compilers other than clang.
#ifndef __has_feature
# define __has_feature(x) 0
#endif

#ifndef FMT_USE_INITIALIZER_LIST
# define FMT_USE_INITIALIZER_LIST \
   (__has_feature(cxx_generalized_initializers) || \
       (FMT_GCC_VERSION >= 404 && __cplusplus >= 201103) || _MSC_VER >= 1700)
#endif

#if FMT_USE_INITIALIZER_LIST
# include <initializer_list>
#endif

// Define FMT_USE_NOEXCEPT to make format use noexcept (C++11 feature).
#if FMT_USE_NOEXCEPT || __has_feature(cxx_noexcept) || \
  (FMT_GCC_VERSION >= 408 && __cplusplus >= 201103)
# define FMT_NOEXCEPT(expr) noexcept(expr)
#else
# define FMT_NOEXCEPT(expr)
#endif

#if FMT_GCC_VERSION >= 406
# define FMT_GCC_DIAGNOSTIC
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wlong-long"
#endif

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4521)
#endif

namespace fmt {

namespace internal {

#if _SECURE_SCL
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

  // Do not implement!
  Array(const Array &);
  void operator=(const Array &);

 public:
  Array() : size_(0), capacity_(SIZE), ptr_(data_) {}
  ~Array() {
    if (ptr_ != data_) delete [] ptr_;
  }

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

template <>
class CharTraits<char> {
 private:
  // Conversion from wchar_t to char is not supported.
  static char ConvertWChar(wchar_t);

 public:
  typedef const wchar_t *UnsupportedStrType;

  template <typename T>
  static int FormatFloat(char *buffer, std::size_t size,
      const char *format, unsigned width, int precision, T value);
};

template <>
class CharTraits<wchar_t> {
 public:
  typedef const char *UnsupportedStrType;

  static wchar_t ConvertWChar(wchar_t value) { return value; }

  template <typename T>
  static int FormatFloat(wchar_t *buffer, std::size_t size,
      const wchar_t *format, unsigned width, int precision, T value);
};

// Information about an integer type.
// IntTraits is not specialized for integer types smaller than int,
// since these are promoted to int.
template <typename T>
struct IntTraits {
  typedef T UnsignedType;
  static bool IsNegative(T) { return false; }
};

template <typename T, typename UnsignedT>
struct SignedIntTraits {
  typedef UnsignedT UnsignedType;
  static bool IsNegative(T value) { return value < 0; }
};

template <>
struct IntTraits<int> : SignedIntTraits<int, unsigned> {};

template <>
struct IntTraits<long> : SignedIntTraits<long, unsigned long> {};

template <>
struct IntTraits<long long> : SignedIntTraits<long long, unsigned long long> {};

template <typename T>
struct IsLongDouble { enum {VALUE = 0}; };

template <>
struct IsLongDouble<long double> { enum {VALUE = 1}; };

void ReportUnknownType(char code, const char *type);

// Returns the number of decimal digits in n. Leading zeros are not counted
// except for n == 0 in which case CountDigits returns 1.
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

extern const char DIGITS[];

template <typename Char>
class FormatterProxy;
}

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

class FormatError : public std::runtime_error {
 public:
  explicit FormatError(const std::string &message)
  : std::runtime_error(message) {}
};

enum Alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

// Flags.
enum { SIGN_FLAG = 1, PLUS_FLAG = 2, HASH_FLAG = 4 };

struct Spec {};

template <char TYPE>
struct TypeSpec : Spec {
  Alignment align() const { return ALIGN_DEFAULT; }
  unsigned width() const { return 0; }

  bool sign_flag() const { return false; }
  bool plus_flag() const { return false; }
  bool hash_flag() const { return false; }

  char type() const { return TYPE; }
  char fill() const { return ' '; }
};

struct WidthSpec {
  unsigned width_;
  // Fill is always wchar_t and cast to char if necessary to avoid having
  // two specialization of WidthSpec and its subclasses.
  wchar_t fill_;

  WidthSpec(unsigned width, wchar_t fill) : width_(width), fill_(fill) {}

  unsigned width() const { return width_; }
  wchar_t fill() const { return fill_; }
};

struct AlignSpec : WidthSpec {
  Alignment align_;

  AlignSpec(unsigned width, wchar_t fill)
  : WidthSpec(width, fill), align_(ALIGN_DEFAULT) {}

  Alignment align() const { return align_; }
};

template <char TYPE>
struct AlignTypeSpec : AlignSpec {
  AlignTypeSpec(unsigned width, wchar_t fill) : AlignSpec(width, fill) {}

  bool sign_flag() const { return false; }
  bool plus_flag() const { return false; }
  bool hash_flag() const { return false; }

  char type() const { return TYPE; }
};

struct FormatSpec : AlignSpec {
  unsigned flags_;
  char type_;

  FormatSpec(unsigned width = 0, char type = 0, wchar_t fill = ' ')
  : AlignSpec(width, fill), flags_(0), type_(type) {}

  Alignment align() const { return align_; }

  bool sign_flag() const { return (flags_ & SIGN_FLAG) != 0; }
  bool plus_flag() const { return (flags_ & PLUS_FLAG) != 0; }
  bool hash_flag() const { return (flags_ & HASH_FLAG) != 0; }

  char type() const { return type_; }
};

template <typename T, typename SpecT>
class IntFormatter : public SpecT {
 private:
  T value_;

 public:
  IntFormatter(T value, const SpecT &spec = SpecT())
  : SpecT(spec), value_(value) {}

  T value() const { return value_; }
};

/**
  Returns an integer formatter that formats the value in base 2.
 */
IntFormatter<int, TypeSpec<'b'> > bin(int value);

/**
  Returns an integer formatter that formats the value in base 8.
 */
IntFormatter<int, TypeSpec<'o'> > oct(int value);

/**
  Returns an integer formatter that formats the value in base 16 using
  lower-case letters for the digits above 9.
 */
IntFormatter<int, TypeSpec<'x'> > hex(int value);

/**
  Returns an integer formatter that formats the value in base 16 using
  upper-case letters for the digits above 9.
 */
IntFormatter<int, TypeSpec<'X'> > hexu(int value);

/**
  \rst
  Returns an integer formatter that pads the formatted argument with the fill
  character to the specified width using the default (right) alignment.

  **Example**::

    std::string s = str(Writer() << pad(hex(0xcafe), 8, '0'));
    // s == "0000cafe"

  \endrst
 */
template <char TYPE_CODE>
IntFormatter<int, AlignTypeSpec<TYPE_CODE> > pad(
    int value, unsigned width, wchar_t fill = ' ');

#define DEFINE_INT_FORMATTERS(TYPE) \
inline IntFormatter<TYPE, TypeSpec<'b'> > bin(TYPE value) { \
  return IntFormatter<TYPE, TypeSpec<'b'> >(value, TypeSpec<'b'>()); \
} \
 \
inline IntFormatter<TYPE, TypeSpec<'o'> > oct(TYPE value) { \
  return IntFormatter<TYPE, TypeSpec<'o'> >(value, TypeSpec<'o'>()); \
} \
 \
inline IntFormatter<TYPE, TypeSpec<'x'> > hex(TYPE value) { \
  return IntFormatter<TYPE, TypeSpec<'x'> >(value, TypeSpec<'x'>()); \
} \
 \
inline IntFormatter<TYPE, TypeSpec<'X'> > hexu(TYPE value) { \
  return IntFormatter<TYPE, TypeSpec<'X'> >(value, TypeSpec<'X'>()); \
} \
 \
template <char TYPE_CODE> \
inline IntFormatter<TYPE, AlignTypeSpec<TYPE_CODE> > pad( \
    IntFormatter<TYPE, TypeSpec<TYPE_CODE> > f, \
    unsigned width, wchar_t fill = ' ') { \
  return IntFormatter<TYPE, AlignTypeSpec<TYPE_CODE> >( \
      f.value(), AlignTypeSpec<TYPE_CODE>(width, fill)); \
} \
 \
inline IntFormatter<TYPE, AlignTypeSpec<0> > pad( \
    TYPE value, unsigned width, wchar_t fill = ' ') { \
  return IntFormatter<TYPE, AlignTypeSpec<0> >( \
      value, AlignTypeSpec<0>(width, fill)); \
}

DEFINE_INT_FORMATTERS(int)
DEFINE_INT_FORMATTERS(long)
DEFINE_INT_FORMATTERS(unsigned)
DEFINE_INT_FORMATTERS(unsigned long)
DEFINE_INT_FORMATTERS(long long)
DEFINE_INT_FORMATTERS(unsigned long long)

template <typename Char>
class BasicFormatter;

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
  enum { INLINE_BUFFER_SIZE = 500 };
  mutable internal::Array<Char, INLINE_BUFFER_SIZE> buffer_;  // Output buffer.

  friend class BasicFormatter<Char>;

#if _SECURE_SCL
  typedef stdext::checked_array_iterator<Char*> CharPtr;
  static Char *GetBase(CharPtr p) { return p.base(); }
#else
  typedef Char *CharPtr;
  static Char *GetBase(Char *p) { return p; }
#endif

  static void FormatDecimal(
      CharPtr buffer, uint64_t value, unsigned num_digits);

  static CharPtr FillPadding(CharPtr buffer,
      unsigned total_size, std::size_t content_size, wchar_t fill);

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  CharPtr GrowBuffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return internal::CheckPtr(&buffer_[size], n);
  }

  CharPtr PrepareFilledBuffer(unsigned size, const Spec &, char sign) {
    CharPtr p = GrowBuffer(size);
    *p = sign;
    return p + size - 1;
  }

  CharPtr PrepareFilledBuffer(unsigned size, const AlignSpec &spec, char sign);

  // Formats an integer.
  template <typename T>
  void FormatInt(T value, const FormatSpec &spec) {
    *this << IntFormatter<T, FormatSpec>(value, spec);
  }

  // Formats a floating-point number (double or long double).
  template <typename T>
  void FormatDouble(T value, const FormatSpec &spec, int precision);

  template <typename StringChar>
  CharPtr FormatString(const StringChar *s,
      std::size_t size, const FormatSpec &spec);

  // This method is private to disallow writing a wide string to a
  // char stream and vice versa. If you want to print a wide string
  // as a pointer as std::ostream does, cast it to const void*.
  // Do not implement!
  void operator<<(typename internal::CharTraits<Char>::UnsupportedStrType);

 public:
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

  BasicWriter &operator<<(int value) {
    return *this << IntFormatter<int, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicWriter &operator<<(unsigned value) {
    return *this << IntFormatter<unsigned, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicWriter &operator<<(long value) {
    return *this << IntFormatter<long, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicWriter &operator<<(unsigned long value) {
    return *this <<
        IntFormatter<unsigned long, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicWriter &operator<<(long long value) {
    return *this << IntFormatter<long long, TypeSpec<0> >(value, TypeSpec<0>());
  }

  /**
    Formats *value* and writes it to the stream.
   */
  BasicWriter &operator<<(unsigned long long value) {
    return *this <<
        IntFormatter<unsigned long long, TypeSpec<0> >(value, TypeSpec<0>());
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

  BasicWriter &operator<<(char value) {
    *GrowBuffer(1) = value;
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

  template <typename T, typename Spec>
  BasicWriter &operator<<(const IntFormatter<T, Spec> &f);

  void Write(const std::basic_string<char> &s, const FormatSpec &spec) {
    FormatString(s.data(), s.size(), spec);
  }

  void Clear() {
    buffer_.clear();
  }
};

template <typename Char>
template <typename StringChar>
typename BasicWriter<Char>::CharPtr BasicWriter<Char>::FormatString(
    const StringChar *s, std::size_t size, const FormatSpec &spec) {
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
BasicWriter<Char> &BasicWriter<Char>::operator<<(
    const IntFormatter<T, Spec> &f) {
  T value = f.value();
  unsigned size = 0;
  char sign = 0;
  typedef typename internal::IntTraits<T>::UnsignedType UnsignedType;
  UnsignedType abs_value = value;
  if (internal::IntTraits<T>::IsNegative(value)) {
    sign = '-';
    ++size;
    abs_value = 0 - abs_value;
  } else if (f.sign_flag()) {
    sign = f.plus_flag() ? '+' : ' ';
    ++size;
  }
  switch (f.type()) {
  case 0: case 'd': {
    unsigned num_digits = internal::CountDigits(abs_value);
    CharPtr p =
        PrepareFilledBuffer(size + num_digits, f, sign) + 1 - num_digits;
    BasicWriter::FormatDecimal(p, abs_value, num_digits);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = f.hash_flag();
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, f, sign));
    n = abs_value;
    const char *digits = f.type() == 'x' ?
        "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    if (print_prefix) {
      *p-- = f.type();
      *p = '0';
    }
    break;
  }
  case 'b': case 'B': {
    UnsignedType n = abs_value;
    bool print_prefix = f.hash_flag();
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 1) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, f, sign));
    n = abs_value;
    do {
      *p-- = '0' + (n & 1);
    } while ((n >>= 1) != 0);
    if (print_prefix) {
      *p-- = f.type();
      *p = '0';
    }
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    bool print_prefix = f.hash_flag();
    if (print_prefix) ++size;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    Char *p = GetBase(PrepareFilledBuffer(size, f, sign));
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    if (print_prefix)
      *p = '0';
    break;
  }
  default:
    internal::ReportUnknownType(f.type(), "integer");
    break;
  }
  return *this;
}

template <typename Char>
BasicFormatter<Char> BasicWriter<Char>::Format(StringRef format) {
  BasicFormatter<Char> f(*this, format.c_str());
  return f;
}

typedef BasicWriter<char> Writer;
typedef BasicWriter<wchar_t> WWriter;

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
  <http://docs.python.org/3/library/stdtypes.html#str.format>`__.
  The class provides operator<< for feeding formatting arguments and all
  the output is sent to a :cpp:class:`fmt::Writer` object.
  \endrst
 */
template <typename Char>
class BasicFormatter {
 private:
  BasicWriter<Char> *writer_;

  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, LONG_LONG, ULONG_LONG, DOUBLE, LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };

  typedef void (*FormatFunc)(
      BasicWriter<Char> &w, const void *arg, const FormatSpec &spec);

  // A format argument.
  class Arg {
   private:
    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to const void*. Do not implement!
    template <typename T>
    Arg(const T *value);

    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to void*. Do not implement!
    template <typename T>
    Arg(T *value);

    struct StringValue {
      const Char *value;
      std::size_t size;
    };

    struct CustomValue {
      const void *value;
      FormatFunc format;
    };

   public:
    Type type;
    union {
      int int_value;
      unsigned uint_value;
      double double_value;
      long long_value;
      unsigned long ulong_value;
      long long long_long_value;
      unsigned long long ulong_long_value;
      long double long_double_value;
      const void *pointer_value;
      StringValue string;
      CustomValue custom;
    };
    mutable BasicFormatter *formatter;

    Arg(short value) : type(INT), int_value(value), formatter(0) {}
    Arg(unsigned short value) : type(UINT), int_value(value), formatter(0) {}
    Arg(int value) : type(INT), int_value(value), formatter(0) {}
    Arg(unsigned value) : type(UINT), uint_value(value), formatter(0) {}
    Arg(long value) : type(LONG), long_value(value), formatter(0) {}
    Arg(unsigned long value) : type(ULONG), ulong_value(value), formatter(0) {}
    Arg(long long value)
    : type(LONG_LONG), long_long_value(value), formatter(0) {}
    Arg(unsigned long long value)
    : type(ULONG_LONG), ulong_long_value(value), formatter(0) {}
    Arg(float value) : type(DOUBLE), double_value(value), formatter(0) {}
    Arg(double value) : type(DOUBLE), double_value(value), formatter(0) {}
    Arg(long double value)
    : type(LONG_DOUBLE), long_double_value(value), formatter(0) {}
    Arg(char value) : type(CHAR), int_value(value), formatter(0) {}
    Arg(wchar_t value)
    : type(CHAR), int_value(internal::CharTraits<Char>::ConvertWChar(value)),
      formatter(0) {}

    Arg(const Char *value) : type(STRING), formatter(0) {
      string.value = value;
      string.size = 0;
    }

    Arg(Char *value) : type(STRING), formatter(0) {
      string.value = value;
      string.size = 0;
    }

    Arg(const void *value)
    : type(POINTER), pointer_value(value), formatter(0) {}

    Arg(void *value) : type(POINTER), pointer_value(value), formatter(0) {}

    Arg(const std::string &value) : type(STRING), formatter(0) {
      string.value = value.c_str();
      string.size = value.size();
    }

    Arg(StringRef value) : type(STRING), formatter(0) {
      string.value = value.c_str();
      string.size = value.size();
    }

    template <typename T>
    Arg(const T &value) : type(CUSTOM), formatter(0) {
      custom.value = &value;
      custom.format = &internal::FormatCustomArg<Char, T>;
    }

    ~Arg() FMT_NOEXCEPT(false) {
      // Format is called here to make sure that a referred object is
      // still alive, for example:
      //
      //   Print("{0}") << std::string("test");
      //
      // Here an Arg object refers to a temporary std::string which is
      // destroyed at the end of the statement. Since the string object is
      // constructed before the Arg object, it will be destroyed after,
      // so it will be alive in the Arg's destructor where Format is called.
      // Note that the string object will not necessarily be alive when
      // the destructor of BasicFormatter is called.
      if (formatter)
        formatter->CompleteFormatting();
    }
  };

  enum { NUM_INLINE_ARGS = 10 };
  internal::Array<const Arg*, NUM_INLINE_ARGS> args_;  // Format arguments.

  const Char *format_;  // Format string.
  int num_open_braces_;
  int next_arg_index_;

  typedef unsigned long long ULongLong;

  friend class internal::FormatterProxy<Char>;

  // Forbid copying from a temporary as in the following example:
  //   fmt::Formatter<> f = Format("test"); // not allowed
  // This is done because BasicFormatter objects should normally exist
  // only as temporaries returned by one of the formatting functions.
  // Do not implement.
  BasicFormatter(const BasicFormatter &);
  BasicFormatter& operator=(const BasicFormatter &);

  void ReportError(const Char *s, StringRef message) const;

  unsigned ParseUInt(const Char *&s) const;

  // Parses argument index and returns an argument with this index.
  const Arg &ParseArgIndex(const Char *&s);

  void CheckSign(const Char *&s, const Arg &arg);

  // Parses the format string and performs the actual formatting,
  // writing the output to writer_.
  void DoFormat();

  struct Proxy {
    BasicWriter<Char> *writer;
    const Char *format;

    Proxy(BasicWriter<Char> *w, const Char *fmt) : writer(w), format(fmt) {}
  };

 protected:
  const Char *TakeFormatString() {
    const Char *format = this->format_;
    this->format_ = 0;
    return format;
  }

  void CompleteFormatting() {
    if (!format_) return;
    DoFormat();
  }

 public:
  // Constructs a formatter with a writer to be used for output and a format
  // string.
  BasicFormatter(BasicWriter<Char> &w, const Char *format = 0)
  : writer_(&w), format_(format) {}

#if FMT_USE_INITIALIZER_LIST
  // Constructs a formatter with formatting arguments.
  BasicFormatter(BasicWriter<Char> &w,
      const Char *format, std::initializer_list<Arg> args)
  : writer_(&w), format_(format) {
    args_.reserve(args.size());
    for (const Arg &arg: args)
      args_.push_back(&arg);
  }
#endif

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
    args_.push_back(&arg);
    return *this;
  }

  operator internal::FormatterProxy<Char>() {
    return internal::FormatterProxy<Char>(this);
  }

  operator StringRef() {
    CompleteFormatting();
    return StringRef(writer_->c_str(), writer_->size());
  }
};

template <typename Char>
inline std::basic_string<Char> str(const BasicWriter<Char> &f) {
  return f.str();
}

template <typename Char>
inline const Char *c_str(const BasicWriter<Char> &f) { return f.c_str(); }

namespace internal {

template <typename Char>
class FormatterProxy {
 private:
  BasicFormatter<Char> *formatter_;

 public:
  explicit FormatterProxy(BasicFormatter<Char> *f) : formatter_(f) {}

  BasicWriter<Char> *Format() {
    formatter_->CompleteFormatting();
    return formatter_->writer_;
  }
};
}

/**
  Returns the content of the output buffer as an `std::string`.
 */
inline std::string str(internal::FormatterProxy<char> p) {
  return p.Format()->str();
}

/**
  Returns a pointer to the output buffer content with terminating null
  character appended.
 */
inline const char *c_str(internal::FormatterProxy<char> p) {
  return p.Format()->c_str();
}

inline std::wstring str(internal::FormatterProxy<wchar_t> p) {
  return p.Format()->str();
}

inline const wchar_t *c_str(internal::FormatterProxy<wchar_t> p) {
  return p.Format()->c_str();
}

/**
  A formatting action that does nothing.
 */
class NoAction {
 public:
  /** Does nothing. */
  template <typename Char>
  void operator()(const BasicWriter<Char> &) const {}
};

/**
  \rst
  A formatter with an action performed when formatting is complete.
  Objects of this class normally exist only as temporaries returned
  by one of the formatting functions. You can use this class to create
  your own functions similar to :cpp:func:`fmt::Format()`.

  **Example**::

    struct PrintError {
      void operator()(const fmt::Writer &w) const {
        fmt::Print("Error: {}\n") << w.str();
      }
    };

    // Formats an error message and prints it to stdout.
    fmt::Formatter<PrintError> ReportError(const char *format) {
      fmt::Formatter f<PrintError>(format);
      return f;
    }

    ReportError("File not found: {}") << path;
  \endrst
 */
template <typename Action = NoAction, typename Char = char>
class Formatter : private Action, public BasicFormatter<Char> {
 private:
  BasicWriter<Char> writer_;
  bool inactive_;

  // Forbid copying other than from a temporary. Do not implement.
  Formatter(const Formatter &);
  Formatter& operator=(const Formatter &);

 public:
  /**
    \rst
    Constructs a formatter with a format string and an action.
    The action should be an unary function object that takes a const
    reference to :cpp:class:`fmt::BasicWriter` as an argument.
    See :cpp:class:`fmt::NoAction` and :cpp:class:`fmt::Write` for
    examples of action classes.
    \endrst
  */
  explicit Formatter(BasicStringRef<Char> format, Action a = Action())
  : Action(a), BasicFormatter<Char>(writer_, format.c_str()),
    inactive_(false) {
  }

  Formatter(Formatter &f)
  : Action(f), BasicFormatter<Char>(writer_, f.TakeFormatString()),
    inactive_(false) {
    f.inactive_ = true;
  }

  /**
    Performs the actual formatting, invokes the action and destroys the object.
   */
  ~Formatter() FMT_NOEXCEPT(false) {
    if (!inactive_) {
      this->CompleteFormatting();
      (*this)(writer_);
    }
  }
};

/**
  Fast integer formatter.
 */
class FormatInt {
 private:
  // Buffer should be large enough to hold all digits (digits10 + 1),
  // a sign and a null character.
  enum {BUFFER_SIZE = std::numeric_limits<uint64_t>::digits10 + 3};
  char buffer_[BUFFER_SIZE];
  char *str_;

  // Formats value in reverse and returns the number of digits.
  char *FormatDecimal(uint64_t value) {
    char *buffer_end = buffer_ + BUFFER_SIZE;
    *--buffer_end = '\0';
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

 public:
  explicit FormatInt(int value) {
    unsigned abs_value = value;
    bool negative = value < 0;
    if (negative)
      abs_value = 0 - value;
    str_ = FormatDecimal(abs_value);
    if (negative)
      *--str_ = '-';
  }
  explicit FormatInt(unsigned value) : str_(FormatDecimal(value)) {}
  explicit FormatInt(uint64_t value) : str_(FormatDecimal(value)) {}

  const char *c_str() const { return str_; }
  std::string str() const { return str_; }
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

inline Formatter<NoAction, wchar_t> Format(WStringRef format) {
  Formatter<NoAction, wchar_t> f(format);
  return f;
}

/** A formatting action that writes formatted output to stdout. */
class Write {
 public:
  /** Writes the output to stdout. */
  void operator()(const BasicWriter<char> &w) const {
    std::fwrite(w.data(), 1, w.size(), stdout);
  }
};

// Formats a string and prints it to stdout.
// Example:
//   Print("Elapsed time: {0:.2f} seconds") << 1.23;
inline Formatter<Write> Print(StringRef format) {
  Formatter<Write> f(format);
  return f;
}
}

#ifdef FMT_GCC_DIAGNOSTIC
# pragma GCC diagnostic pop
#endif

#if _MSC_VER
# pragma warning(pop)
#endif

#endif  // FORMAT_H_
