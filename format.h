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
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sstream>

namespace fmt {

namespace internal {

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
  capacity_ = std::max(size, capacity_ + capacity_ / 2);
  T *p = new T[capacity_];
  std::copy(ptr_, ptr_ + size_, p);
  if (ptr_ != data_)
    delete [] ptr_;
  ptr_ = p;
}

template <typename T, std::size_t SIZE>
void Array<T, SIZE>::append(const T *begin, const T *end) {
  std::ptrdiff_t num_elements = end - begin;
  if (size_ + num_elements > capacity_)
    Grow(num_elements);
  std::copy(begin, end, ptr_ + size_);
  size_ += num_elements;
}

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

template <typename T>
struct IsLongDouble { enum {VALUE = 0}; };

template <>
struct IsLongDouble<long double> { enum {VALUE = 1}; };

extern const char DIGITS[];

void ReportUnknownType(char code, const char *type);

// Returns the number of decimal digits in n. Trailing zeros are not counted
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

#ifndef _MSC_VER

inline int SignBit(double value) {
  // When compiled in C++11 mode signbit is no longer a macro but a function
  // defined in namespace std and the macro is undefined.
  using namespace std;
  return signbit(value);
}

inline int IsInf(double x) {
#ifdef isinf
  return isinf(x);
#else
  return std::isinf(x);
#endif
}

#define FMT_SNPRINTF snprintf

#else

inline int SignBit(double value) {
  if (value < 0) return 1;
  if (value == value) return 0;
  int dec = 0, sign = 0;
  char buffer[2];  // The buffer size must be >= 2 or _ecvt_s will fail.
  _ecvt_s(buffer, sizeof(buffer), value, 0, &dec, &sign);
  return sign;
}

inline int IsInf(double x) { return !_finite(x); }

#define FMT_SNPRINTF sprintf_s

#endif

template <typename Char>
class ArgInserter;

template <typename Char>
class FormatterProxy;
}

/**
  \rst
  A string reference. It can be constructed from a C string, ``std::string``
  or as a result of a formatting operation. It is most useful as a parameter
  type to allow passing different types of strings in a function, for example::

    TempFormatter<> Format(StringRef format);

    Format("{}") << 42;
    Format(std::string("{}")) << 42;
    Format(Format("{{}}")) << 42;
  \endrst
*/
class StringRef {
 private:
  const char *data_;
  mutable std::size_t size_;

 public:
  /**
    Constructs a string reference object from a C string and a size.
    If `size` is zero, which is the default, the size is computed with
    `strlen`.
   */
  StringRef(const char *s, std::size_t size = 0) : data_(s), size_(size) {}

  /**
    Constructs a string reference from an `std::string` object.
   */
  StringRef(const std::string &s) : data_(s.c_str()), size_(s.size()) {}

  /**
    Converts a string reference to an `std::string` object.
   */
  operator std::string() const { return std::string(data_, size()); }

  /**
    Returns the pointer to a C string.
   */
  const char *c_str() const { return data_; }

  /**
    Returns the string size.
   */
  std::size_t size() const {
    if (size_ == 0) size_ = std::strlen(data_);
    return size_;
  }
};

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
  char fill_;

  WidthSpec(unsigned width, char fill) : width_(width), fill_(fill) {}

  unsigned width() const { return width_; }
  char fill() const { return fill_; }
};

struct AlignSpec : WidthSpec {
  Alignment align_;

  AlignSpec(unsigned width, char fill)
  : WidthSpec(width, fill), align_(ALIGN_DEFAULT) {}

  Alignment align() const { return align_; }
};

template <char TYPE>
struct AlignTypeSpec : AlignSpec {
  AlignTypeSpec(unsigned width, char fill) : AlignSpec(width, fill) {}

  bool sign_flag() const { return false; }
  bool plus_flag() const { return false; }
  bool hash_flag() const { return false; }

  char type() const { return TYPE; }
};

struct FormatSpec : AlignSpec {
  unsigned flags_;
  char type_;

  FormatSpec(unsigned width = 0, char type = 0, char fill = ' ')
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

    std::string s = str(BasicWriter() << pad(hex(0xcafe), 8, '0'));
    // s == "0000cafe"

  \endrst
 */
template <char TYPE_CODE>
IntFormatter<int, AlignTypeSpec<TYPE_CODE> > pad(
    int value, unsigned width, char fill = ' ');

#define DEFINE_INT_FORMATTERS(TYPE) \
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
    unsigned width, char fill = ' ') { \
  return IntFormatter<TYPE, AlignTypeSpec<TYPE_CODE> >( \
      f.value(), AlignTypeSpec<TYPE_CODE>(width, fill)); \
} \
 \
inline IntFormatter<TYPE, AlignTypeSpec<0> > pad( \
    TYPE value, unsigned width, char fill = ' ') { \
  return IntFormatter<TYPE, AlignTypeSpec<0> >( \
      value, AlignTypeSpec<0>(width, fill)); \
}

DEFINE_INT_FORMATTERS(int)
DEFINE_INT_FORMATTERS(long)
DEFINE_INT_FORMATTERS(unsigned)
DEFINE_INT_FORMATTERS(unsigned long)

template <typename Char>
class BasicWriter {
 private:
  static void FormatDecimal(Char *buffer, uint64_t value, unsigned num_digits);

 protected:
  static Char *FillPadding(Char *buffer,
      unsigned total_size, std::size_t content_size, char fill);

  enum { INLINE_BUFFER_SIZE = 500 };
  mutable internal::Array<Char, INLINE_BUFFER_SIZE> buffer_;  // Output buffer.

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  Char *GrowBuffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return &buffer_[size];
  }

  Char *PrepareFilledBuffer(unsigned size, const Spec &, char sign) {
    Char *p = GrowBuffer(size);
    *p = sign;
    return p + size - 1;
  }

  Char *PrepareFilledBuffer(unsigned size, const AlignSpec &spec, char sign);

  // Formats an integer.
  template <typename T>
  void FormatInt(T value, const FormatSpec &spec) {
    *this << IntFormatter<T, FormatSpec>(value, spec);
  }

  // Formats a floating point number (double or long double).
  template <typename T>
  void FormatDouble(T value, const FormatSpec &spec, int precision);

  char *FormatString(const char *s, std::size_t size, const FormatSpec &spec);

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

  BasicWriter &operator<<(int value) {
    return *this << IntFormatter<int, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicWriter &operator<<(unsigned value) {
    return *this << IntFormatter<unsigned, TypeSpec<0> >(value, TypeSpec<0>());
  }

  BasicWriter &operator<<(double value) {
    FormatDouble(value, FormatSpec(), -1);
    return *this;
  }

  BasicWriter &operator<<(Char value) {
    *GrowBuffer(1) = value;
    return *this;
  }

  BasicWriter &operator<<(const Char *value) {
    std::size_t size = std::strlen(value);
    std::strncpy(GrowBuffer(size), value, size);
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

// Fills the padding around the content and returns the pointer to the
// content area.
template <typename Char>
Char *BasicWriter<Char>::FillPadding(Char *buffer,
    unsigned total_size, std::size_t content_size, char fill) {
  std::size_t padding = total_size - content_size;
  std::size_t left_padding = padding / 2;
  std::fill_n(buffer, left_padding, fill);
  buffer += left_padding;
  Char *content = buffer;
  std::fill_n(buffer + content_size, padding - left_padding, fill);
  return content;
}

template <typename Char>
void BasicWriter<Char>::FormatDecimal(
    Char *buffer, uint64_t value, unsigned num_digits) {
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

template <typename Char>
Char *BasicWriter<Char>::PrepareFilledBuffer(
    unsigned size, const AlignSpec &spec, char sign) {
  unsigned width = spec.width();
  if (width <= size) {
    Char *p = GrowBuffer(size);
    *p = sign;
    return p + size - 1;
  }
  char *p = GrowBuffer(width);
  char *end = p + width;
  Alignment align = spec.align();
  if (align == ALIGN_LEFT) {
    *p = sign;
    p += size;
    std::fill(p, end, spec.fill());
  } else if (align == ALIGN_CENTER) {
    p = FillPadding(p, width, size, spec.fill());
    *p = sign;
    p += size;
  } else {
    if (align == ALIGN_NUMERIC) {
      if (sign) {
        *p++ = sign;
        --size;
      }
    } else {
      *(end - size) = sign;
    }
    std::fill(p, end - size, spec.fill());
    p = end;
  }
  return p - 1;
}

template <typename Char>
template <typename T>
void BasicWriter<Char>::FormatDouble(
    T value, const FormatSpec &spec, int precision) {
  // Check type.
  char type = spec.type();
  bool upper = false;
  switch (type) {
  case 0:
    type = 'g';
    break;
  case 'e': case 'f': case 'g':
    break;
  case 'F':
#ifdef _MSC_VER
    // MSVC's printf doesn't support 'F'.
    type = 'f';
#endif
    // Fall through.
  case 'E': case 'G':
    upper = true;
    break;
  default:
    internal::ReportUnknownType(type, "double");
    break;
  }

  char sign = 0;
  // Use SignBit instead of value < 0 because the latter is always
  // false for NaN.
  if (internal::SignBit(value)) {
    sign = '-';
    value = -value;
  } else if (spec.sign_flag()) {
    sign = spec.plus_flag() ? '+' : ' ';
  }

  if (value != value) {
    // Format NaN ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t size = 4;
    const char *nan = upper ? " NAN" : " nan";
    if (!sign) {
      --size;
      ++nan;
    }
    char *out = FormatString(nan, size, spec);
    if (sign)
      *out = sign;
    return;
  }

  if (internal::IsInf(value)) {
    // Format infinity ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t size = 4;
    const char *inf = upper ? " INF" : " inf";
    if (!sign) {
      --size;
      ++inf;
    }
    char *out = FormatString(inf, size, spec);
    if (sign)
      *out = sign;
    return;
  }

  std::size_t offset = buffer_.size();
  unsigned width = spec.width();
  if (sign) {
    buffer_.reserve(buffer_.size() + std::max(width, 1u));
    if (width > 0)
      --width;
    ++offset;
  }

  // Build format string.
  enum { MAX_FORMAT_SIZE = 10}; // longest format: %#-*.*Lg
  char format[MAX_FORMAT_SIZE];
  char *format_ptr = format;
  *format_ptr++ = '%';
  unsigned width_for_sprintf = width;
  if (spec.hash_flag())
    *format_ptr++ = '#';
  if (spec.align() == ALIGN_CENTER) {
    width_for_sprintf = 0;
  } else {
    if (spec.align() == ALIGN_LEFT)
      *format_ptr++ = '-';
    if (width != 0)
      *format_ptr++ = '*';
  }
  if (precision >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (internal::IsLongDouble<T>::VALUE)
    *format_ptr++ = 'L';
  *format_ptr++ = type;
  *format_ptr = '\0';

  // Format using snprintf.
  for (;;) {
    std::size_t size = buffer_.capacity() - offset;
    int n = 0;
    Char *start = &buffer_[offset];
    if (width_for_sprintf == 0) {
      n = precision < 0 ?
          FMT_SNPRINTF(start, size, format, value) :
          FMT_SNPRINTF(start, size, format, precision, value);
    } else {
      n = precision < 0 ?
          FMT_SNPRINTF(start, size, format, width_for_sprintf, value) :
          FMT_SNPRINTF(start, size, format, width_for_sprintf,
              precision, value);
    }
    if (n >= 0 && offset + n < buffer_.capacity()) {
      if (sign) {
        if ((spec.align() != ALIGN_RIGHT && spec.align() != ALIGN_DEFAULT) ||
            *start != ' ') {
          *(start - 1) = sign;
          sign = 0;
        } else {
          *(start - 1) = spec.fill();
        }
        ++n;
      }
      if (spec.align() == ALIGN_CENTER &&
          spec.width() > static_cast<unsigned>(n)) {
        char *p = GrowBuffer(spec.width());
        std::copy(p, p + n, p + (spec.width() - n) / 2);
        FillPadding(p, spec.width(), n, spec.fill());
        return;
      }
      if (spec.fill() != ' ' || sign) {
        while (*start == ' ')
          *start++ = spec.fill();
        if (sign)
          *(start - 1) = sign;
      }
      GrowBuffer(n);
      return;
    }
    buffer_.reserve(n >= 0 ? offset + n + 1 : 2 * buffer_.capacity());
  }
}

template <typename Char>
char *BasicWriter<Char>::FormatString(
    const char *s, std::size_t size, const FormatSpec &spec) {
  char *out = 0;
  if (spec.width() > size) {
    out = GrowBuffer(spec.width());
    if (spec.align() == ALIGN_RIGHT) {
      std::fill_n(out, spec.width() - size, spec.fill());
      out += spec.width() - size;
    } else if (spec.align() == ALIGN_CENTER) {
      out = FillPadding(out, spec.width(), size, spec.fill());
    } else {
      std::fill_n(out + size, spec.width() - size, spec.fill());
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
    Char *p = PrepareFilledBuffer(size + num_digits, f, sign) - num_digits + 1;
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
    Char *p = PrepareFilledBuffer(size, f, sign);
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
  case 'o': {
    UnsignedType n = abs_value;
    bool print_prefix = f.hash_flag();
    if (print_prefix) ++size;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    Char *p = PrepareFilledBuffer(size, f, sign);
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
  The output is stored in a memory buffer that grows dynamically.

  **Example**::

     Formatter out;
     out("Current point:\n");
     out("(-{:+f}, {:+f})") << 3.14 << -3.14;

  This will populate the buffer of the ``out`` object with the following
  output:

  .. code-block:: none

     Current point:
     (-3.140000, +3.140000)

  The buffer can be accessed using :meth:`data` or :meth:`c_str`.
  \endrst
 */
template <typename Char>
class BasicFormatter : public BasicWriter<Char> {
 private:
  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, DOUBLE, LONG_DOUBLE,
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

    // This method is private to disallow formatting of wide characters.
    // If you want to output a wide character cast it to integer type.
    // Do not implement!
    Arg(wchar_t value);

   public:
    Type type;
    union {
      int int_value;
      unsigned uint_value;
      double double_value;
      long long_value;
      unsigned long ulong_value;
      long double long_double_value;
      const void *pointer_value;
      struct {
        const char *value;
        std::size_t size;
      } string;
      struct {
        const void *value;
        FormatFunc format;
      } custom;
    };
    mutable BasicFormatter *formatter;

    Arg(int value) : type(INT), int_value(value), formatter(0) {}
    Arg(unsigned value) : type(UINT), uint_value(value), formatter(0) {}
    Arg(long value) : type(LONG), long_value(value), formatter(0) {}
    Arg(unsigned long value) : type(ULONG), ulong_value(value), formatter(0) {}
    Arg(double value) : type(DOUBLE), double_value(value), formatter(0) {}
    Arg(long double value)
    : type(LONG_DOUBLE), long_double_value(value), formatter(0) {}
    Arg(char value) : type(CHAR), int_value(value), formatter(0) {}

    Arg(const char *value) : type(STRING), formatter(0) {
      string.value = value;
      string.size = 0;
    }

    Arg(char *value) : type(STRING), formatter(0) {
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

    template <typename T>
    Arg(const T &value) : type(CUSTOM), formatter(0) {
      custom.value = &value;
      custom.format = &internal::FormatCustomArg<Char, T>;
    }

    ~Arg() {
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
      // the destructor of ArgInserter is called.
      if (formatter)
        formatter->CompleteFormatting();
    }
  };

  enum { NUM_INLINE_ARGS = 10 };
  internal::Array<const Arg*, NUM_INLINE_ARGS> args_;  // Format arguments.

  const Char *format_;  // Format string.
  int num_open_braces_;
  int next_arg_index_;

  friend class internal::ArgInserter<Char>;
  friend class internal::FormatterProxy<Char>;

  void Add(const Arg &arg) {
    args_.push_back(&arg);
  }

  void ReportError(const char *s, StringRef message) const;

  unsigned ParseUInt(const char *&s) const;

  // Parses argument index and returns an argument with this index.
  const Arg &ParseArgIndex(const char *&s);

  void CheckSign(const char *&s, const Arg &arg);

  void DoFormat();

  void CompleteFormatting() {
    if (!format_) return;
    DoFormat();
  }

 public:
  /**
    Constructs a formatter with an empty output buffer.
   */
  BasicFormatter() : format_(0) {}

  /**
    Formats a string appending the output to the internal buffer.
    Arguments are accepted through the returned `ArgInserter` object
    using inserter operator `<<`.
  */
  internal::ArgInserter<Char> operator()(StringRef format);
};

typedef BasicFormatter<char> Formatter;
typedef BasicFormatter<wchar_t> WFormatter;

template <typename Char>
inline std::basic_string<Char> str(const BasicWriter<Char> &f) {
  return f.str();
}

template <typename Char>
inline const Char *c_str(const BasicWriter<Char> &f) { return f.c_str(); }

std::string str(internal::FormatterProxy<char> p);
const char *c_str(internal::FormatterProxy<char> p);

namespace internal {

using fmt::str;
using fmt::c_str;

template <typename Char>
class FormatterProxy {
 private:
  BasicFormatter<Char> *formatter_;

 public:
  explicit FormatterProxy(BasicFormatter<Char> *f) : formatter_(f) {}

  BasicFormatter<Char> *Format() {
    formatter_->CompleteFormatting();
    return formatter_;
  }
};

// This is a transient object that normally exists only as a temporary
// returned by one of the formatting functions. It stores a reference
// to a formatter and provides operator<< that feeds arguments to the
// formatter.
template <typename Char>
class ArgInserter {
 private:
  mutable BasicFormatter<Char> *formatter_;

  friend class fmt::BasicFormatter<Char>;
  friend class fmt::StringRef;

  // Do not implement.
  void operator=(const ArgInserter& other);

 protected:
  explicit ArgInserter(BasicFormatter<Char> *f = 0) : formatter_(f) {}

  void Init(BasicFormatter<Char> &f, const char *format) {
    const ArgInserter &other = f(format);
    formatter_ = other.formatter_;
    other.formatter_ = 0;
  }

  ArgInserter(const ArgInserter& other)
  : formatter_(other.formatter_) {
    other.formatter_ = 0;
  }

  const BasicFormatter<Char> *Format() const {
    BasicFormatter<Char> *f = formatter_;
    if (f) {
      formatter_ = 0;
      f->CompleteFormatting();
    }
    return f;
  }

  BasicFormatter<Char> *formatter() const { return formatter_; }
  const char *format() const { return formatter_->format_; }

  void ResetFormatter() const { formatter_ = 0; }

 public:
  ~ArgInserter() {
    if (formatter_)
      formatter_->CompleteFormatting();
  }

  // Feeds an argument to a formatter.
  ArgInserter &operator<<(const typename BasicFormatter<Char>::Arg &arg) {
    arg.formatter = formatter_;
    formatter_->Add(arg);
    return *this;
  }

  operator FormatterProxy<Char>() {
    BasicFormatter<Char> *f = formatter_;
    formatter_ = 0;
    return FormatterProxy<Char>(f);
  }

  operator StringRef() {
    const BasicFormatter<Char> *f = Format();
    return StringRef(f->c_str(), f->size());
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

template <typename Char>
inline internal::ArgInserter<Char>
BasicFormatter<Char>::operator()(StringRef format) {
  internal::ArgInserter<Char> inserter(this);
  format_ = format.c_str();
  args_.clear();
  return inserter;
}

/**
  A formatting action that does nothing.
 */
class NoAction {
 public:
  /** Does nothing. */
  template <typename Char>
  void operator()(const BasicFormatter<Char> &) const {}
};

/**
  A formatter with an action performed when formatting is complete.
  Objects of this class normally exist only as temporaries returned
  by one of the formatting functions which explains the name.
 */
template <typename Action = NoAction, typename Char = char>
class TempFormatter : public internal::ArgInserter<Char> {
 private:
  BasicFormatter<Char> formatter_;
  Action action_;

  // Forbid copying other than from a temporary. Do not implement.
  TempFormatter(TempFormatter &);

  // Do not implement.
  TempFormatter& operator=(const TempFormatter &);

  struct Proxy {
    const char *format;
    Action action;

    Proxy(const char *fmt, Action a) : format(fmt), action(a) {}
  };

 public:
  /**
    \rst
    Constructs a temporary formatter with a format string and an action.
    The action should be an unary function object that takes a const
    reference to :cpp:class:`fmt::BasicFormatter` as an argument.
    See :cpp:class:`fmt::NoAction` and :cpp:class:`fmt::Write` for
    examples of action classes.
    \endrst
  */
  explicit TempFormatter(StringRef format, Action a = Action())
  : action_(a) {
    this->Init(formatter_, format.c_str());
  }

  /**
    Constructs a temporary formatter from a proxy object.
   */
  TempFormatter(const Proxy &p)
  : internal::ArgInserter<Char>(0), action_(p.action) {
    this->Init(formatter_, p.format);
  }

  /**
    Performs the actual formatting, invokes the action and destroys the object.
   */
  ~TempFormatter() {
    if (this->formatter())
      action_(*this->Format());
  }

  /**
    Converts a temporary formatter into a proxy object.
   */
  operator Proxy() {
    const char *fmt = this->format();
    this->ResetFormatter();
    return Proxy(fmt, action_);
  }
};

/**
  \rst
  Formats a string. Returns a temporary formatter object that accepts
  arguments via operator ``<<``. *format* is a format string that contains
  literal text and replacement fields surrounded by braces ``{}``.
  The formatter object replaces the fields with formatted arguments
  and stores the output in a memory buffer. The content of the buffer can
  be converted to ``std::string`` with :cpp:func:`fmt::str()` or
  accessed as a C string with :cpp:func:`fmt::c_str()`.

  **Example**::

    std::string message = str(Format("The answer is {}") << 42);

  See also `Format String Syntax`_.
  \endrst
*/
inline TempFormatter<> Format(StringRef format) {
  return TempFormatter<>(format);
}

// A formatting action that writes formatted output to stdout.
struct Write {
  void operator()(const BasicFormatter<char> &f) const {
    std::fwrite(f.data(), 1, f.size(), stdout);
  }
};

// Formats a string and prints it to stdout.
// Example:
//   Print("Elapsed time: {0:.2f} seconds") << 1.23;
inline TempFormatter<Write> Print(StringRef format) {
  return TempFormatter<Write>(format);
}

// Throws Exception(message) if format contains '}', otherwise throws
// FormatError reporting unmatched '{'. The idea is that unmatched '{'
// should override other errors.
template <typename Char>
void BasicFormatter<Char>::ReportError(const char *s, StringRef message) const {
  for (int num_open_braces = num_open_braces_; *s; ++s) {
    if (*s == '{') {
      ++num_open_braces;
    } else if (*s == '}') {
      if (--num_open_braces == 0)
        throw fmt::FormatError(message);
    }
  }
  throw fmt::FormatError("unmatched '{' in format");
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
template <typename Char>
unsigned BasicFormatter<Char>::ParseUInt(const char *&s) const {
  assert('0' <= *s && *s <= '9');
  unsigned value = 0;
  do {
    unsigned new_value = value * 10 + (*s++ - '0');
    if (new_value < value)  // Check if value wrapped around.
      ReportError(s, "number is too big in format");
    value = new_value;
  } while ('0' <= *s && *s <= '9');
  return value;
}

template <typename Char>
inline const typename BasicFormatter<Char>::Arg
    &BasicFormatter<Char>::ParseArgIndex(const char *&s) {
  unsigned arg_index = 0;
  if (*s < '0' || *s > '9') {
    if (*s != '}' && *s != ':')
      ReportError(s, "invalid argument index in format string");
    if (next_arg_index_ < 0) {
      ReportError(s,
          "cannot switch from manual to automatic argument indexing");
    }
    arg_index = next_arg_index_++;
  } else {
    if (next_arg_index_ > 0) {
      ReportError(s,
          "cannot switch from automatic to manual argument indexing");
    }
    next_arg_index_ = -1;
    arg_index = ParseUInt(s);
  }
  if (arg_index >= args_.size())
    ReportError(s, "argument index is out of range in format");
  return *args_[arg_index];
}

template <typename Char>
void BasicFormatter<Char>::CheckSign(const char *&s, const Arg &arg) {
  if (arg.type > LAST_NUMERIC_TYPE) {
    ReportError(s,
        Format("format specifier '{0}' requires numeric argument") << *s);
  }
  if (arg.type == UINT || arg.type == ULONG) {
    ReportError(s,
        Format("format specifier '{0}' requires signed argument") << *s);
  }
  ++s;
}

template <typename Char>
void BasicFormatter<Char>::DoFormat() {
  const Char *start = format_;
  format_ = 0;
  next_arg_index_ = 0;
  const Char *s = start;
  while (*s) {
    char c = *s++;
    if (c != '{' && c != '}') continue;
    if (*s == c) {
      this->buffer_.append(start, s);
      start = ++s;
      continue;
    }
    if (c == '}')
      throw FormatError("unmatched '}' in format");
    num_open_braces_= 1;
    this->buffer_.append(start, s - 1);

    const Arg &arg = ParseArgIndex(s);

    FormatSpec spec;
    int precision = -1;
    if (*s == ':') {
      ++s;

      // Parse fill and alignment.
      if (char c = *s) {
        const char *p = s + 1;
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
                ReportError(s, "invalid fill character '{'");
              s += 2;
              spec.fill_ = c;
            } else ++s;
            if (spec.align_ == ALIGN_NUMERIC && arg.type > LAST_NUMERIC_TYPE)
              ReportError(s, "format specifier '=' requires numeric argument");
            break;
          }
        } while (--p >= s);
      }

      // Parse sign.
      switch (*s) {
      case '+':
        CheckSign(s, arg);
        spec.flags_ |= SIGN_FLAG | PLUS_FLAG;
        break;
      case '-':
        CheckSign(s, arg);
        break;
      case ' ':
        CheckSign(s, arg);
        spec.flags_ |= SIGN_FLAG;
        break;
      }

      if (*s == '#') {
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '#' requires numeric argument");
        spec.flags_ |= HASH_FLAG;
        ++s;
      }

      // Parse width and zero flag.
      if ('0' <= *s && *s <= '9') {
        if (*s == '0') {
          if (arg.type > LAST_NUMERIC_TYPE)
            ReportError(s, "format specifier '0' requires numeric argument");
          spec.align_ = ALIGN_NUMERIC;
          spec.fill_ = '0';
        }
        // Zero may be parsed again as a part of the width, but it is simpler
        // and more efficient than checking if the next char is a digit.
        unsigned value = ParseUInt(s);
        if (value > INT_MAX)
          ReportError(s, "number is too big in format");
        spec.width_ = value;
      }

      // Parse precision.
      if (*s == '.') {
        ++s;
        precision = 0;
        if ('0' <= *s && *s <= '9') {
          unsigned value = ParseUInt(s);
          if (value > INT_MAX)
            ReportError(s, "number is too big in format");
          precision = value;
        } else if (*s == '{') {
          ++s;
          ++num_open_braces_;
          const Arg &precision_arg = ParseArgIndex(s);
          unsigned long value = 0;
          switch (precision_arg.type) {
          case INT:
            if (precision_arg.int_value < 0)
              ReportError(s, "negative precision in format");
            value = precision_arg.int_value;
            break;
          case UINT:
            value = precision_arg.uint_value;
            break;
          case LONG:
            if (precision_arg.long_value < 0)
              ReportError(s, "negative precision in format");
            value = precision_arg.long_value;
            break;
          case ULONG:
            value = precision_arg.ulong_value;
            break;
          default:
            ReportError(s, "precision is not integer");
          }
          if (value > INT_MAX)
            ReportError(s, "number is too big in format");
          precision = value;
          if (*s++ != '}')
            throw FormatError("unmatched '{' in format");
          --num_open_braces_;
        } else {
          ReportError(s, "missing precision in format");
        }
        if (arg.type != DOUBLE && arg.type != LONG_DOUBLE) {
          ReportError(s,
              "precision specifier requires floating-point argument");
        }
      }

      // Parse type.
      if (*s != '}' && *s)
        spec.type_ = *s++;
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case INT:
      this->FormatInt(arg.int_value, spec);
      break;
    case UINT:
      this->FormatInt(arg.uint_value, spec);
      break;
    case LONG:
      this->FormatInt(arg.long_value, spec);
      break;
    case ULONG:
      this->FormatInt(arg.ulong_value, spec);
      break;
    case DOUBLE:
      this->FormatDouble(arg.double_value, spec, precision);
      break;
    case LONG_DOUBLE:
      this->FormatDouble(arg.long_double_value, spec, precision);
      break;
    case CHAR: {
      if (spec.type_ && spec.type_ != 'c')
        internal::ReportUnknownType(spec.type_, "char");
      char *out = 0;
      if (spec.width_ > 1) {
        out = this->GrowBuffer(spec.width_);
        if (spec.align_ == ALIGN_RIGHT) {
          std::fill_n(out, spec.width_ - 1, spec.fill_);
          out += spec.width_ - 1;
        } else if (spec.align_ == ALIGN_CENTER) {
          out = this->FillPadding(out, spec.width_, 1, spec.fill_);
        } else {
          std::fill_n(out + 1, spec.width_ - 1, spec.fill_);
        }
      } else {
        out = this->GrowBuffer(1);
      }
      *out = arg.int_value;
      break;
    }
    case STRING: {
      if (spec.type_ && spec.type_ != 's')
        internal::ReportUnknownType(spec.type_, "string");
      const char *str = arg.string.value;
      std::size_t size = arg.string.size;
      if (size == 0) {
        if (!str)
          throw FormatError("string pointer is null");
        if (*str)
          size = std::strlen(str);
      }
      this->FormatString(str, size, spec);
      break;
    }
    case POINTER:
      if (spec.type_ && spec.type_ != 'p')
        internal::ReportUnknownType(spec.type_, "pointer");
      spec.flags_= HASH_FLAG;
      spec.type_ = 'x';
      this->FormatInt(reinterpret_cast<uintptr_t>(arg.pointer_value), spec);
      break;
    case CUSTOM:
      if (spec.type_)
        internal::ReportUnknownType(spec.type_, "object");
      arg.custom.format(*this, arg.custom.value, spec);
      break;
    default:
      assert(false);
      break;
    }
  }
  this->buffer_.append(start, s);
}
}

#endif  // FORMAT_H_
