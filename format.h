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

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace format {

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

class ArgInserter;
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

#define DEFINE_INT_FORMATTERS(TYPE) \
/* Returns an integer formatter that formats value in the octal base. */ \
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

class BasicFormatter {
 private:
  static unsigned CountDigits(uint64_t n) {
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

  static void FormatDecimal(char *buffer, uint64_t value, unsigned num_digits);

 protected:
  static void ReportUnknownType(char code, const char *type);

  enum { INLINE_BUFFER_SIZE = 500 };
  mutable internal::Array<char, INLINE_BUFFER_SIZE> buffer_;  // Output buffer.

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  char *GrowBuffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return &buffer_[size];
  }

  char *PrepareFilledBuffer(unsigned size, const Spec &, char sign) {
    char *p = GrowBuffer(size);
    *p = sign;
    return p + size - 1;
  }

  char *PrepareFilledBuffer(unsigned size, const AlignSpec &spec, char sign);

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
  const char *data() const { return &buffer_[0]; }

  /**
    Returns a pointer to the output buffer content with terminating null
    character appended.
   */
  const char *c_str() const {
    std::size_t size = buffer_.size();
    buffer_.reserve(size + 1);
    buffer_[size] = '\0';
    return &buffer_[0];
  }

  /**
    Returns the content of the output buffer as an `std::string`.
   */
  std::string str() const { return std::string(&buffer_[0], buffer_.size()); }

  BasicFormatter &operator<<(int value) {
    return *this << IntFormatter<int, TypeSpec<0> >(value, TypeSpec<0>());
  }
  BasicFormatter &operator<<(unsigned value) {
    return *this << IntFormatter<unsigned, TypeSpec<0> >(value, TypeSpec<0>());
  }

  BasicFormatter &operator<<(char value) {
    *GrowBuffer(1) = value;
    return *this;
  }

  BasicFormatter &operator<<(const char *value) {
    std::size_t size = std::strlen(value);
    std::strncpy(GrowBuffer(size), value, size);
    return *this;
  }

  template <typename T, typename Spec>
  BasicFormatter &operator<<(const IntFormatter<T, Spec> &f);

  void Write(const std::string &s, const FormatSpec &spec) {
    FormatString(s.data(), s.size(), spec);
  }

  void Clear() {
    buffer_.clear();
  }
};

template <typename T, typename Spec>
BasicFormatter &BasicFormatter::operator<<(const IntFormatter<T, Spec> &f) {
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
    unsigned num_digits = BasicFormatter::CountDigits(abs_value);
    char *p = PrepareFilledBuffer(size + num_digits, f, sign)
        - num_digits + 1;
    BasicFormatter::FormatDecimal(p, abs_value, num_digits);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = f.hash_flag();
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    char *p = PrepareFilledBuffer(size, f, sign);
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
    char *p = PrepareFilledBuffer(size, f, sign);
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    if (print_prefix)
      *p = '0';
    break;
  }
  default:
    BasicFormatter::ReportUnknownType(f.type(), "integer");
    break;
  }
  return *this;
}

// The default formatting function.
template <typename T>
void Format(BasicFormatter &f, const FormatSpec &spec, const T &value) {
  std::ostringstream os;
  os << value;
  f.Write(os.str(), spec);
}

/**
  \rst
  The :cpp:class:`format::Formatter` class provides string formatting
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
class Formatter : public BasicFormatter {
 private:
  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, DOUBLE, LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };

  typedef void (Formatter::*FormatFunc)(
      const void *arg, const FormatSpec &spec);

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
    mutable Formatter *formatter;

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
      custom.format = &Formatter::FormatCustomArg<T>;
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
      formatter->CompleteFormatting();
    }
  };

  enum { NUM_INLINE_ARGS = 10 };
  internal::Array<const Arg*, NUM_INLINE_ARGS> args_;  // Format arguments.

  const char *format_;  // Format string.
  int num_open_braces_;
  int next_arg_index_;

  friend class internal::ArgInserter;
  friend class internal::FormatterProxy;

  void Add(const Arg &arg) {
    args_.push_back(&arg);
  }

  void ReportError(const char *s, StringRef message) const;

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  void FormatCustomArg(const void *arg, const FormatSpec &spec) {
    BasicFormatter &f = *this;
    Format(f, spec, *static_cast<const T*>(arg));
  }

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
  Formatter() : format_(0) {}

  /**
    Formats a string appending the output to the internal buffer.
    Arguments are accepted through the returned `ArgInserter` object
    using inserter operator `<<`.
  */
  internal::ArgInserter operator()(StringRef format);
};

std::string str(internal::FormatterProxy p);
const char *c_str(internal::FormatterProxy p);

namespace internal {

using format::str;
using format::c_str;

class FormatterProxy {
 private:
  Formatter *formatter_;

 public:
  explicit FormatterProxy(Formatter *f) : formatter_(f) {}

  Formatter *Format() {
    formatter_->CompleteFormatting();
    return formatter_;
  }
};

// This is a transient object that normally exists only as a temporary
// returned by one of the formatting functions. It stores a reference
// to a formatter and provides operator<< that feeds arguments to the
// formatter.
class ArgInserter {
 private:
  mutable Formatter *formatter_;

  friend class format::Formatter;
  friend class format::StringRef;

  // Do not implement.
  void operator=(const ArgInserter& other);

 protected:
  explicit ArgInserter(Formatter *f = 0) : formatter_(f) {}

  void Init(Formatter &f, const char *format) {
    const ArgInserter &other = f(format);
    formatter_ = other.formatter_;
    other.formatter_ = 0;
  }

  ArgInserter(const ArgInserter& other)
  : formatter_(other.formatter_) {
    other.formatter_ = 0;
  }

  const Formatter *Format() const {
    Formatter *f = formatter_;
    if (f) {
      formatter_ = 0;
      f->CompleteFormatting();
    }
    return f;
  }

  Formatter *formatter() const { return formatter_; }
  const char *format() const { return formatter_->format_; }

  void ResetFormatter() const { formatter_ = 0; }

 public:
  ~ArgInserter() {
    if (formatter_)
      formatter_->CompleteFormatting();
  }

  // Feeds an argument to a formatter.
  ArgInserter &operator<<(const Formatter::Arg &arg) {
    arg.formatter = formatter_;
    formatter_->Add(arg);
    return *this;
  }

  operator FormatterProxy() {
    Formatter *f = formatter_;
    formatter_ = 0;
    return FormatterProxy(f);
  }

  operator StringRef() {
    const Formatter *f = Format();
    return StringRef(f->c_str(), f->size());
  }
};
}

/**
  Returns the content of the output buffer as an `std::string`.
 */
inline std::string str(internal::FormatterProxy p) {
  return p.Format()->str();
}

/**
  Returns a pointer to the output buffer content with terminating null
  character appended.
 */
inline const char *c_str(internal::FormatterProxy p) {
  return p.Format()->c_str();
}

inline internal::ArgInserter Formatter::operator()(StringRef format) {
  internal::ArgInserter formatter(this);
  format_ = format.c_str();
  args_.clear();
  return formatter;
}

/**
  A formatting action that does nothing.
 */
class NoAction {
 public:
  /** Does nothing. */
  void operator()(const Formatter &) const {}
};

/**
  A formatter with an action performed when formatting is complete.
  Objects of this class normally exist only as temporaries returned
  by one of the formatting functions which explains the name.
 */
template <typename Action = NoAction>
class TempFormatter : public internal::ArgInserter {
 private:
  Formatter formatter_;
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
    reference to :cpp:class:`format::Formatter` as an argument.
    See :cpp:class:`format::NoAction` and :cpp:class:`format::Write` for
    examples of action classes.
    \endrst
  */
  explicit TempFormatter(StringRef format, Action a = Action())
  : action_(a) {
    Init(formatter_, format.c_str());
  }

  /**
    Constructs a temporary formatter from a proxy object.
   */
  TempFormatter(const Proxy &p)
  : ArgInserter(0), action_(p.action) {
    Init(formatter_, p.format);
  }

  /**
    Performs the actual formatting, invokes the action and destroys the object.
   */
  ~TempFormatter() {
    if (formatter())
      action_(*Format());
  }

  /**
    Converts a temporary formatter into a proxy object.
   */
  operator Proxy() {
    const char *fmt = format();
    ResetFormatter();
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
  be converted to ``std::string`` with :cpp:func:`format::str()` or
  accessed as a C string with :cpp:func:`format::c_str()`.

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
  void operator()(const Formatter &f) const {
    std::fwrite(f.data(), 1, f.size(), stdout);
  }
};

// Formats a string and prints it to stdout.
// Example:
//   Print("Elapsed time: {0:.2f} seconds") << 1.23;
inline TempFormatter<Write> Print(StringRef format) {
  return TempFormatter<Write>(format);
}
}

namespace fmt = format;

#endif  // FORMAT_H_
