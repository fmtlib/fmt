/*
 Formatting library for C++

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#ifndef FMT_PRINTF_H_
#define FMT_PRINTF_H_

#include <algorithm>  // std::fill_n
#include <limits>     // std::numeric_limits

#include "fmt/ostream.h"

namespace fmt {
namespace internal {

// Checks if a value fits in int - used to avoid warnings about comparing
// signed and unsigned integers.
template <bool IsSigned>
struct IntChecker {
  template <typename T>
  static bool fits_in_int(T value) {
    unsigned max = std::numeric_limits<int>::max();
    return value <= max;
  }
  static bool fits_in_int(bool) { return true; }
};

template <>
struct IntChecker<true> {
  template <typename T>
  static bool fits_in_int(T value) {
    return value >= std::numeric_limits<int>::min() &&
           value <= std::numeric_limits<int>::max();
  }
  static bool fits_in_int(int) { return true; }
};

class PrintfPrecisionHandler {
 public:
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, int>::type
      operator()(T value) {
    if (!IntChecker<std::numeric_limits<T>::is_signed>::fits_in_int(value))
      FMT_THROW(format_error("number is too big"));
    return static_cast<int>(value);
  }

  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value, int>::type
      operator()(T) {
    FMT_THROW(format_error("precision is not integer"));
    return 0;
  }
};

// An argument visitor that returns true iff arg is a zero integer.
class IsZeroInt {
 public:
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, bool>::type
      operator()(T value) { return value == 0; }

  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value, bool>::type
      operator()(T value) { return false; }
};

template <typename T, typename U>
struct is_same {
  enum { value = 0 };
};

template <typename T>
struct is_same<T, T> {
  enum { value = 1 };
};

template <typename T, typename Context>
class ArgConverter {
 private:
  typedef typename Context::char_type Char;

  basic_format_arg<Context> &arg_;
  typename Context::char_type type_;

 public:
  ArgConverter(basic_format_arg<Context> &arg, Char type)
    : arg_(arg), type_(type) {}

  void operator()(bool value) {
    if (type_ != 's')
      operator()<bool>(value);
  }

  template <typename U>
  typename std::enable_if<std::is_integral<U>::value>::type
      operator()(U value) {
    bool is_signed = type_ == 'd' || type_ == 'i';
    typedef typename internal::Conditional<
        is_same<T, void>::value, U, T>::type TargetType;
    typedef basic_format_context<Char> format_context;
    if (sizeof(TargetType) <= sizeof(int)) {
      // Extra casts are used to silence warnings.
      if (is_signed) {
        arg_ = internal::make_arg<Context>(
          static_cast<int>(static_cast<TargetType>(value)));
      } else {
        typedef typename internal::MakeUnsigned<TargetType>::Type Unsigned;
        arg_ = internal::make_arg<Context>(
          static_cast<unsigned>(static_cast<Unsigned>(value)));
      }
    } else {
      if (is_signed) {
        // glibc's printf doesn't sign extend arguments of smaller types:
        //   std::printf("%lld", -42);  // prints "4294967254"
        // but we don't have to do the same because it's a UB.
        arg_ = internal::make_arg<Context>(static_cast<LongLong>(value));
      } else {
        arg_ = internal::make_arg<Context>(
          static_cast<typename internal::MakeUnsigned<U>::Type>(value));
      }
    }
  }

  template <typename U>
  typename std::enable_if<!std::is_integral<U>::value>::type
      operator()(U value) {
    // No coversion needed for non-integral types.
  }
};

// Converts an integer argument to T for printf, if T is an integral type.
// If T is void, the argument is converted to corresponding signed or unsigned
// type depending on the type specifier: 'd' and 'i' - signed, other -
// unsigned).
template <typename T, typename Context, typename Char>
void convert_arg(basic_format_arg<Context> &arg, Char type) {
  visit(ArgConverter<T, Context>(arg, type), arg);
}

// Converts an integer argument to char for printf.
template <typename Context>
class CharConverter {
 private:
  basic_format_arg<Context> &arg_;

  FMT_DISALLOW_COPY_AND_ASSIGN(CharConverter);

 public:
  explicit CharConverter(basic_format_arg<Context> &arg) : arg_(arg) {}

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value>::type
      operator()(T value) {
    arg_ = internal::make_arg<Context>(static_cast<char>(value));
  }

  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value>::type operator()(T) {
    // No coversion needed for non-integral types.
  }
};

// Checks if an argument is a valid printf width specifier and sets
// left alignment if it is negative.
class PrintfWidthHandler {
 private:
  FormatSpec &spec_;

  FMT_DISALLOW_COPY_AND_ASSIGN(PrintfWidthHandler);

 public:
  explicit PrintfWidthHandler(FormatSpec &spec) : spec_(spec) {}

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, unsigned>::type
      operator()(T value) {
    typedef typename internal::IntTraits<T>::MainType UnsignedType;
    UnsignedType width = static_cast<UnsignedType>(value);
    if (internal::is_negative(value)) {
      spec_.align_ = ALIGN_LEFT;
      width = 0 - width;
    }
    unsigned int_max = std::numeric_limits<int>::max();
    if (width > int_max)
      FMT_THROW(format_error("number is too big"));
    return static_cast<unsigned>(width);
  }

  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value, unsigned>::type
      operator()(T value) {
    FMT_THROW(format_error("width is not integer"));
    return 0;
  }
};
}  // namespace internal

/**
  \rst
  The ``printf`` argument formatter.
  \endrst
 */
template <typename Char>
class PrintfArgFormatter : public internal::ArgFormatterBase<Char> {
 private:
  void write_null_pointer() {
    this->spec().type_ = 0;
    this->write("(nil)");
  }

  typedef internal::ArgFormatterBase<Char> Base;

 public:
  /**
    \rst
    Constructs an argument formatter object.
    *writer* is a reference to the output writer and *spec* contains format
    specifier information for standard argument types.
    \endrst
   */
  PrintfArgFormatter(basic_writer<Char> &writer, FormatSpec &spec)
  : internal::ArgFormatterBase<Char>(writer, spec) {}

  using Base::operator();

  /** Formats an argument of type ``bool``. */
  void operator()(bool value) {
    FormatSpec &fmt_spec = this->spec();
    if (fmt_spec.type_ != 's')
      return (*this)(value ? 1 : 0);
    fmt_spec.type_ = 0;
    this->write(value);
  }

  /** Formats a character. */
  void operator()(Char value) {
    const FormatSpec &fmt_spec = this->spec();
    basic_writer<Char> &w = this->writer();
    if (fmt_spec.type_ && fmt_spec.type_ != 'c')
      w.write_int(value, fmt_spec);
    typedef typename basic_writer<Char>::CharPtr CharPtr;
    CharPtr out = CharPtr();
    if (fmt_spec.width_ > 1) {
      Char fill = ' ';
      out = w.grow_buffer(fmt_spec.width_);
      if (fmt_spec.align_ != ALIGN_LEFT) {
        std::fill_n(out, fmt_spec.width_ - 1, fill);
        out += fmt_spec.width_ - 1;
      } else {
        std::fill_n(out + 1, fmt_spec.width_ - 1, fill);
      }
    } else {
      out = w.grow_buffer(1);
    }
    *out = static_cast<Char>(value);
  }

  /** Formats a null-terminated C string. */
  void operator()(const char *value) {
    if (value)
      Base::operator()(value);
    else if (this->spec().type_ == 'p')
      write_null_pointer();
    else
      this->write("(null)");
  }

  /** Formats a pointer. */
  void operator()(const void *value) {
    if (value)
      return Base::operator()(value);
    this->spec().type_ = 0;
    write_null_pointer();
  }

  /** Formats an argument of a custom (user-defined) type. */
  void operator()(internal::CustomValue<Char> c) {
    const Char format_str[] = {'}', '\0'};
    auto args = basic_format_args<basic_format_context<Char>>();
    basic_format_context<Char> ctx(format_str, args);
    c.format(this->writer(), c.value, &ctx);
  }
};

/** This template formats data and writes the output to a writer. */
template <typename Char,
          typename ArgFormatter = PrintfArgFormatter<Char> >
class printf_context :
  private internal::format_context_base<
    Char, printf_context<Char, ArgFormatter>> {
 public:
  /** The character type for the output. */
  typedef Char char_type;

 private:
  typedef internal::format_context_base<Char, printf_context> Base;
  typedef typename Base::format_arg format_arg;

  void parse_flags(FormatSpec &spec, const Char *&s);

  // Returns the argument with specified index or, if arg_index is equal
  // to the maximum unsigned value, the next argument.
  format_arg get_arg(
      const Char *s,
      unsigned arg_index = (std::numeric_limits<unsigned>::max)());

  // Parses argument index, flags and width and returns the argument index.
  unsigned parse_header(const Char *&s, FormatSpec &spec);

 public:
  /**
   \rst
   Constructs a ``printf_context`` object. References to the arguments and
   the writer are stored in the context object so make sure they have
   appropriate lifetimes.
   \endrst
   */
  explicit printf_context(BasicCStringRef<Char> format_str,
                          basic_format_args<printf_context> args)
    : Base(format_str.c_str(), args) {}

  /** Formats stored arguments and writes the output to the writer. */
  FMT_API void format(basic_writer<Char> &writer);
};

template <typename Char, typename AF>
void printf_context<Char, AF>::parse_flags(FormatSpec &spec, const Char *&s) {
  for (;;) {
    switch (*s++) {
      case '-':
        spec.align_ = ALIGN_LEFT;
        break;
      case '+':
        spec.flags_ |= SIGN_FLAG | PLUS_FLAG;
        break;
      case '0':
        spec.fill_ = '0';
        break;
      case ' ':
        spec.flags_ |= SIGN_FLAG;
        break;
      case '#':
        spec.flags_ |= HASH_FLAG;
        break;
      default:
        --s;
        return;
    }
  }
}

template <typename Char, typename AF>
typename printf_context<Char, AF>::format_arg printf_context<Char, AF>::get_arg(
    const Char *s, unsigned arg_index) {
  (void)s;
  const char *error = 0;
  format_arg arg = arg_index == std::numeric_limits<unsigned>::max() ?
    this->next_arg(error) : Base::get_arg(arg_index - 1, error);
  if (error)
    FMT_THROW(format_error(!*s ? "invalid format string" : error));
  return arg;
}

template <typename Char, typename AF>
unsigned printf_context<Char, AF>::parse_header(
  const Char *&s, FormatSpec &spec) {
  unsigned arg_index = std::numeric_limits<unsigned>::max();
  Char c = *s;
  if (c >= '0' && c <= '9') {
    // Parse an argument index (if followed by '$') or a width possibly
    // preceded with '0' flag(s).
    unsigned value = internal::parse_nonnegative_int(s);
    if (*s == '$') {  // value is an argument index
      ++s;
      arg_index = value;
    } else {
      if (c == '0')
        spec.fill_ = '0';
      if (value != 0) {
        // Nonzero value means that we parsed width and don't need to
        // parse it or flags again, so return now.
        spec.width_ = value;
        return arg_index;
      }
    }
  }
  parse_flags(spec, s);
  // Parse width.
  if (*s >= '0' && *s <= '9') {
    spec.width_ = internal::parse_nonnegative_int(s);
  } else if (*s == '*') {
    ++s;
    spec.width_ = visit(internal::PrintfWidthHandler(spec), get_arg(s));
  }
  return arg_index;
}

template <typename Char, typename AF>
void printf_context<Char, AF>::format(basic_writer<Char> &writer) {
  const Char *start = this->ptr();
  const Char *s = start;
  while (*s) {
    Char c = *s++;
    if (c != '%') continue;
    if (*s == c) {
      internal::write(writer, start, s);
      start = ++s;
      continue;
    }
    internal::write(writer, start, s - 1);

    FormatSpec spec;
    spec.align_ = ALIGN_RIGHT;

    // Parse argument index, flags and width.
    unsigned arg_index = parse_header(s, spec);

    // Parse precision.
    if (*s == '.') {
      ++s;
      if ('0' <= *s && *s <= '9') {
        spec.precision_ = static_cast<int>(internal::parse_nonnegative_int(s));
      } else if (*s == '*') {
        ++s;
        spec.precision_ = visit(internal::PrintfPrecisionHandler(), get_arg(s));
      }
    }

    format_arg arg = get_arg(s, arg_index);
    if (spec.flag(HASH_FLAG) && visit(internal::IsZeroInt(), arg))
      spec.flags_ &= ~internal::to_unsigned<int>(HASH_FLAG);
    if (spec.fill_ == '0') {
      if (arg.is_numeric())
        spec.align_ = ALIGN_NUMERIC;
      else
        spec.fill_ = ' ';  // Ignore '0' flag for non-numeric types.
    }

    // Parse length and convert the argument to the required type.
    using internal::convert_arg;
    switch (*s++) {
    case 'h':
      if (*s == 'h')
        convert_arg<signed char>(arg, *++s);
      else
        convert_arg<short>(arg, *s);
      break;
    case 'l':
      if (*s == 'l')
        convert_arg<fmt::LongLong>(arg, *++s);
      else
        convert_arg<long>(arg, *s);
      break;
    case 'j':
      convert_arg<intmax_t>(arg, *s);
      break;
    case 'z':
      convert_arg<std::size_t>(arg, *s);
      break;
    case 't':
      convert_arg<std::ptrdiff_t>(arg, *s);
      break;
    case 'L':
      // printf produces garbage when 'L' is omitted for long double, no
      // need to do the same.
      break;
    default:
      --s;
      convert_arg<void>(arg, *s);
    }

    // Parse type.
    if (!*s)
      FMT_THROW(format_error("invalid format string"));
    spec.type_ = static_cast<char>(*s++);
    if (arg.is_integral()) {
      // Normalize type.
      switch (spec.type_) {
      case 'i': case 'u':
        spec.type_ = 'd';
        break;
      case 'c':
        // TODO: handle wchar_t
        visit(internal::CharConverter<printf_context<Char, AF>>(arg), arg);
        break;
      }
    }

    start = s;

    // Format argument.
    visit(AF(writer, spec), arg);
  }
  internal::write(writer, start, s);
}

// Formats a value.
template <typename Char, typename T>
void format_value(basic_writer<Char> &w, const T &value,
                  printf_context<Char>& ctx) {
  internal::MemoryBuffer<Char, internal::INLINE_BUFFER_SIZE> buffer;
  w << internal::format_value(buffer, value);
}

template <typename Char>
void printf(basic_writer<Char> &w, BasicCStringRef<Char> format,
            basic_format_args<printf_context<Char>> args) {
  printf_context<Char>(format, args).format(w);
}

typedef basic_format_args<printf_context<char>> printf_args;

inline std::string vsprintf(CStringRef format, printf_args args) {
  MemoryWriter w;
  printf(w, format, args);
  return w.str();
}

/**
  \rst
  Formats arguments and returns the result as a string.

  **Example**::

    std::string message = fmt::sprintf("The answer is %d", 42);
  \endrst
*/
template <typename... Args>
inline std::string sprintf(CStringRef format_str, const Args & ... args) {
  return vsprintf(format_str, make_xformat_args<printf_context<char>>(args...));
}

inline std::wstring vsprintf(
    WCStringRef format, basic_format_args<printf_context<wchar_t>> args) {
  WMemoryWriter w;
  printf(w, format, args);
  return w.str();
}

template <typename... Args>
inline std::wstring sprintf(WCStringRef format_str, const Args & ... args) {
  auto vargs = make_xformat_args<printf_context<wchar_t>>(args...);
  return vsprintf(format_str, vargs);
}

FMT_API int vfprintf(std::FILE *f, CStringRef format, printf_args args);

/**
  \rst
  Prints formatted data to the file *f*.

  **Example**::

    fmt::fprintf(stderr, "Don't %s!", "panic");
  \endrst
 */
template <typename... Args>
inline int fprintf(std::FILE *f, CStringRef format_str, const Args & ... args) {
  auto vargs = make_xformat_args<printf_context<char>>(args...);
  return vfprintf(f, format_str, vargs);
}

inline int vprintf(CStringRef format, printf_args args) {
  return vfprintf(stdout, format, args);
}

/**
  \rst
  Prints formatted data to ``stdout``.

  **Example**::

    fmt::printf("Elapsed time: %.2f seconds", 1.23);
  \endrst
 */
template <typename... Args>
inline int printf(CStringRef format_str, const Args & ... args) {
  return vprintf(format_str, make_xformat_args<printf_context<char>>(args...));
}

inline int vfprintf(std::ostream &os, CStringRef format_str, printf_args args) {
  MemoryWriter w;
  printf(w, format_str, args);
  internal::write(os, w);
  return static_cast<int>(w.size());
}

/**
  \rst
  Prints formatted data to the stream *os*.

  **Example**::

    fprintf(cerr, "Don't %s!", "panic");
  \endrst
 */
template <typename... Args>
inline int fprintf(std::ostream &os, CStringRef format_str,
                   const Args & ... args) {
  auto vargs = make_xformat_args<printf_context<char>>(args...);
  return vfprintf(os, format_str, vargs);
}
}  // namespace fmt

#endif  // FMT_PRINTF_H_
