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

#ifndef FMT_PRINTF_H_
#define FMT_PRINTF_H_


#include "format.h"
#include <climits>
#include <cerrno>

#include <cstdarg>//< Only for windows?


#if defined(_WIN32) && defined(__MINGW32__)
# include <cstring>
#endif

#if FMT_USE_WINDOWS_H
# if defined(NOMINMAX) || defined(FMT_WIN_MINMAX)
#  include <windows.h>
# else
#  define NOMINMAX
#  include <windows.h>
#  undef NOMINMAX
# endif
#endif

using fmt::internal::Arg;

#if FMT_EXCEPTIONS
# define FMT_TRY try
# define FMT_CATCH(x) catch (x)
#else
# define FMT_TRY if (true)
# define FMT_CATCH(x) if (false)
#endif

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4127)  // conditional expression is constant
# pragma warning(disable: 4702)  // unreachable code
// Disable deprecation warning for strerror. The latter is not called but
// MSVC fails to detect it.
# pragma warning(disable: 4996)
#endif

// Dummy implementations of strerror_r and strerror_s called if corresponding
// system functions are not available.
static inline fmt::internal::Null<> strerror_r(int, char *, ...) {
  return fmt::internal::Null<>();
}
static inline fmt::internal::Null<> strerror_s(char *, std::size_t, ...) {
  return fmt::internal::Null<>();
}

namespace fmt {
namespace internal {

template <typename Char> class DefaultPrintfArgFormatter;

template <typename CharType,
          typename PFAF = fmt::internal::DefaultPrintfArgFormatter<CharType> >
class PrintfFormatter;

template <typename Impl, typename CharT>
class PrintfArgFormatter : public internal::ArgFormatterBase<Impl, CharT> {

private:
  void write_null_pointer() {
    this->spec().type_ = 0;
    this->write("(nil)");
  }

public:
  PrintfArgFormatter(BasicWriter<CharT> &w, FormatSpec &s)
    : internal::ArgFormatterBase<Impl, CharT>(w, s) {}

  void visit_bool(bool value) {
    FormatSpec &fmt_spec = this->spec();
    if (fmt_spec.type_ != 's')
      return this->visit_any_int(value);
    fmt_spec.type_ = 0;
    this->write(value);
  }

  void visit_char(int value) {
    const FormatSpec &fmt_spec = this->spec();
    BasicWriter<CharT> &w = this->writer();
    if (fmt_spec.type_ && fmt_spec.type_ != 'c')
      w.write_int(value, fmt_spec);
    typedef typename BasicWriter<CharT>::CharPtr CharPtr;
    CharPtr out = CharPtr();
    if (fmt_spec.width_ > 1) {
      CharT fill = ' ';
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
    *out = static_cast<CharT>(value);
  }

  typedef ArgFormatterBase<Impl, CharT> Base;
  void visit_cstring(const char *value) {
    if (value)
      Base::visit_cstring(value);
    else if (this->spec().type_ == 'p')
      write_null_pointer();
    else
      this->write("(null)");
  }

  void visit_pointer(const void *value) {
    if (value)
      return Base::visit_pointer(value);
    this->spec().type_ = 0;
    write_null_pointer();
  }

  void visit_custom(Arg::CustomValue c) {
    BasicFormatter<CharT> formatter(ArgList(), this->writer());
    const CharT format_str[] = {'}', 0};
    const CharT *format = format_str;
    c.format(&formatter, c.value, &format);
  }
};

/** The default printf argument formatter. */
template <typename CharT>
class DefaultPrintfArgFormatter
    : public PrintfArgFormatter<DefaultPrintfArgFormatter<CharT>, CharT> {
public:
  /** Constructs an argument formatter object. */
  DefaultPrintfArgFormatter(BasicWriter<CharT> &w, FormatSpec &spec)
    : PrintfArgFormatter<DefaultPrintfArgFormatter<CharT>, CharT>(w, spec) {}
};

 template <typename Char, typename PrintfArgFormatterT>
   class PrintfFormatter : private FormatterBase {
 private:
  void parse_flags(FormatSpec &spec, const Char *&s);

  // Returns the argument with specified index or, if arg_index is equal
  // to the maximum unsigned value, the next argument.
  Arg get_arg(const Char *s,
      unsigned arg_index = (std::numeric_limits<unsigned>::max)());

  // Parses argument index, flags and width and returns the argument index.
  unsigned parse_header(const Char *&s, FormatSpec &spec);

public:
  explicit PrintfFormatter(const ArgList &args) : FormatterBase(args) {}
  FMT_API void format(BasicWriter<Char> &writer,
                      BasicCStringRef<Char> format_str);
};


}

template <typename Char>
inline void printf(BasicWriter<Char> &w, BasicCStringRef<Char> format, ArgList args) {
  internal::PrintfFormatter<Char>(args).format(w, format);
}


/**
  \rst
  Formats arguments and returns the result as a string.

  **Example**::

    std::string message = fmt::sprintf("The answer is %d", 42);
  \endrst
*/
inline std::string sprintf(CStringRef format, ArgList args) {
	MemoryWriter w;
	printf(w, format, args);
	return w.str();
}

inline  std::wstring sprintf(WCStringRef format, ArgList args) {
  WMemoryWriter w;
  printf(w, format, args);
  return w.str();
}


/**
  \rst
  Prints formatted data to the file *f*.

  **Example**::

    fmt::fprintf(stderr, "Don't %s!", "panic");
  \endrst
 */
inline FMT_API int fprintf(std::FILE *f, CStringRef format, ArgList args);

/**
  \rst
  Prints formatted data to ``stdout``.

  **Example**::

    fmt::printf("Elapsed time: %.2f seconds", 1.23);
  \endrst
 */
inline  int printf(CStringRef format, ArgList args) {
  return fprintf(stdout, format, args);
}

FMT_VARIADIC(std::string, sprintf, CStringRef)
FMT_VARIADIC_W(std::wstring, sprintf, WCStringRef)
FMT_VARIADIC(int, printf, CStringRef)
FMT_VARIADIC(int, fprintf, std::FILE *, CStringRef)

FMT_FUNC int fprintf(std::FILE *f, CStringRef format, ArgList args) {
  MemoryWriter w;
  printf(w, format, args);
  std::size_t size = w.size();
  return std::fwrite(w.data(), 1, size, f) < size ? -1 : static_cast<int>(size);
}


#ifndef _MSC_VER
# define FMT_SNPRINTF snprintf
#else  // _MSC_VER
inline int fmt_snprintf(char *buffer, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(buffer, size, _TRUNCATE, format, args);
  va_end(args);
  return result;
}
# define FMT_SNPRINTF fmt_snprintf
#endif  // _MSC_VER

#if defined(_WIN32) && defined(__MINGW32__) && !defined(__NO_ISOCEXT)
# define FMT_SWPRINTF snwprintf
#else
# define FMT_SWPRINTF swprintf
#endif // defined(_WIN32) && defined(__MINGW32__) && !defined(__NO_ISOCEXT)

// Checks if a value fits in int - used to avoid warnings about comparing
// signed and unsigned integers.
template <bool IsSigned>
struct IntChecker {
  template <typename T>
  static bool fits_in_int(T value) {
    unsigned max = INT_MAX;
    return value <= max;
  }
  static bool fits_in_int(bool) { return true; }
};

template <>
struct IntChecker<true> {
  template <typename T>
  static bool fits_in_int(T value) {
    return value >= INT_MIN && value <= INT_MAX;
  }
  static bool fits_in_int(int) { return true; }
};

const char RESET_COLOR[] = "\x1b[0m";

typedef void (*FormatFunc)(Writer &, int, StringRef);

// Portable thread-safe version of strerror.
// Sets buffer to point to a string describing the error code.
// This can be either a pointer to a string stored in buffer,
// or a pointer to some static immutable string.
// Returns one of the following values:
//   0      - success
//   ERANGE - buffer is not large enough to store the error message
//   other  - failure
// Buffer should be at least of size 1.
///GAS :take out of header!
inline int safe_strerror(
    int error_code, char *&buffer, std::size_t buffer_size) FMT_NOEXCEPT {
  FMT_ASSERT(buffer != 0 && buffer_size != 0, "invalid buffer");

  class StrError {
   private:
    int error_code_;
    char *&buffer_;
    std::size_t buffer_size_;

    // A noop assignment operator to avoid bogus warnings.
    void operator=(const StrError &) {}

    // Handle the result of XSI-compliant version of strerror_r.
    int handle(int result) {
      // glibc versions before 2.13 return result in errno.
      return result == -1 ? errno : result;
    }

    // Handle the result of GNU-specific version of strerror_r.
    int handle(char *message) {
      // If the buffer is full then the message is probably truncated.
      if (message == buffer_ && strlen(buffer_) == buffer_size_ - 1)
        return ERANGE;
      buffer_ = message;
      return 0;
    }

    // Handle the case when strerror_r is not available.
    int handle(internal::Null<>) {
      return fallback(strerror_s(buffer_, buffer_size_, error_code_));
    }

    // Fallback to strerror_s when strerror_r is not available.
    int fallback(int result) {
      // If the buffer is full then the message is probably truncated.
      return result == 0 && strlen(buffer_) == buffer_size_ - 1 ?
            ERANGE : result;
    }

    // Fallback to strerror if strerror_r and strerror_s are not available.
    int fallback(internal::Null<>) {
      errno = 0;
      buffer_ = strerror(error_code_);
      return errno;
    }

   public:
    StrError(int err_code, char *&buf, std::size_t buf_size)
      : error_code_(err_code), buffer_(buf), buffer_size_(buf_size) {}

    int run() {
      strerror_r(0, 0, "");  // Suppress a warning about unused strerror_r.
      return handle(strerror_r(error_code_, buffer_, buffer_size_));
    }
  };
  return StrError(error_code, buffer, buffer_size).run();
}

inline void format_error_code(Writer &out, int error_code,
                       StringRef message) FMT_NOEXCEPT {
  // Report error code making sure that the output fits into
  // INLINE_BUFFER_SIZE to avoid dynamic memory allocation and potential
  // bad_alloc.
  out.clear();
  static const char SEP[] = ": ";
  static const char ERROR_STR[] = "error ";
  // Subtract 2 to account for terminating null characters in SEP and ERROR_STR.
  std::size_t error_code_size = sizeof(SEP) + sizeof(ERROR_STR) - 2;
  typedef internal::IntTraits<int>::MainType MainType;
  MainType abs_value = static_cast<MainType>(error_code);
  if (internal::is_negative(error_code)) {
    abs_value = 0 - abs_value;
    ++error_code_size;
  }
  error_code_size += internal::count_digits(abs_value);
  if (message.size() <= internal::INLINE_BUFFER_SIZE - error_code_size)
    out << message << SEP;
  out << ERROR_STR << error_code;
  assert(out.size() <= internal::INLINE_BUFFER_SIZE);
}

inline void report_error(FormatFunc func, int error_code,
                  StringRef message) FMT_NOEXCEPT {
  MemoryWriter full_message;
  func(full_message, error_code, message);
  // Use Writer::data instead of Writer::c_str to avoid potential memory
  // allocation.
  std::fwrite(full_message.data(), full_message.size(), 1, stderr);
  std::fputc('\n', stderr);
}

// IsZeroInt::visit(arg) returns true iff arg is a zero integer.
class IsZeroInt : public ArgVisitor<IsZeroInt, bool> {
 public:
  template <typename T>
  bool visit_any_int(T value) { return value == 0; }
};

// Checks if an argument is a valid printf width specifier and sets
// left alignment if it is negative.
class WidthHandler : public ArgVisitor<WidthHandler, unsigned> {
 private:
  FormatSpec &spec_;

  FMT_DISALLOW_COPY_AND_ASSIGN(WidthHandler);

 public:
  explicit WidthHandler(FormatSpec &spec) : spec_(spec) {}

  void report_unhandled_arg() {
    FMT_THROW(FormatError("width is not integer"));
  }

  template <typename T>
  unsigned visit_any_int(T value) {
    typedef typename internal::IntTraits<T>::MainType UnsignedType;
    UnsignedType width = static_cast<UnsignedType>(value);
    if (internal::is_negative(value)) {
      spec_.align_ = ALIGN_LEFT;
      width = 0 - width;
    }
    if (width > INT_MAX)
      FMT_THROW(FormatError("number is too big"));
    return static_cast<unsigned>(width);
  }
};

class PrecisionHandler : public ArgVisitor<PrecisionHandler, int> {
 public:
  void report_unhandled_arg() {
    FMT_THROW(FormatError("precision is not integer"));
  }

  template <typename T>
  int visit_any_int(T value) {
    if (!IntChecker<std::numeric_limits<T>::is_signed>::fits_in_int(value))
      FMT_THROW(FormatError("number is too big"));
    return static_cast<int>(value);
  }
};

template <typename T, typename U>
struct is_same {
  enum { value = 0 };
};

template <typename T>
struct is_same<T, T> {
  enum { value = 1 };
};

// An argument visitor that converts an integer argument to T for printf,
// if T is an integral type. If T is void, the argument is converted to
// corresponding signed or unsigned type depending on the type specifier:
// 'd' and 'i' - signed, other - unsigned)
template <typename T = void>
class ArgConverter : public ArgVisitor<ArgConverter<T>, void> {
 private:
  internal::Arg &arg_;
  wchar_t type_;

  FMT_DISALLOW_COPY_AND_ASSIGN(ArgConverter);

 public:
  ArgConverter(internal::Arg &arg, wchar_t type)
    : arg_(arg), type_(type) {}

  void visit_bool(bool value) {
    if (type_ != 's')
      visit_any_int(value);
  }

  template <typename U>
  void visit_any_int(U value) {
    bool is_signed = type_ == 'd' || type_ == 'i';
    using internal::Arg;
    typedef typename internal::Conditional<
        is_same<T, void>::value, U, T>::type TargetType;
    if (sizeof(TargetType) <= sizeof(int)) {
      // Extra casts are used to silence warnings.
      if (is_signed) {
        arg_.type = Arg::INT;
        arg_.int_value = static_cast<int>(static_cast<TargetType>(value));
      } else {
        arg_.type = Arg::UINT;
        typedef typename internal::MakeUnsigned<TargetType>::Type Unsigned;
        arg_.uint_value = static_cast<unsigned>(static_cast<Unsigned>(value));
      }
    } else {
      if (is_signed) {
        arg_.type = Arg::LONG_LONG;
        // glibc's printf doesn't sign extend arguments of smaller types:
        //   std::printf("%lld", -42);  // prints "4294967254"
        // but we don't have to do the same because it's a UB.
        arg_.long_long_value = static_cast<LongLong>(value);
      } else {
        arg_.type = Arg::ULONG_LONG;
        arg_.ulong_long_value =
            static_cast<typename internal::MakeUnsigned<U>::Type>(value);
      }
    }
  }
};

// Converts an integer argument to char for printf.
class CharConverter : public ArgVisitor<CharConverter, void> {
 private:
  internal::Arg &arg_;

  FMT_DISALLOW_COPY_AND_ASSIGN(CharConverter);

 public:
  explicit CharConverter(internal::Arg &arg) : arg_(arg) {}

  template <typename T>
  void visit_any_int(T value) {
    arg_.type = internal::Arg::CHAR;
    arg_.int_value = static_cast<char>(value);
  }
};

template <typename Char, typename PAF>
void fmt::internal::PrintfFormatter<Char, PAF>::format(
    BasicWriter<Char> &writer, BasicCStringRef<Char> format_str) {
  const Char *start = format_str.c_str();
  const Char *s = start;
  while (*s) {
    Char c = *s++;
    if (c != '%') continue;
    if (*s == c) {
      write(writer, start, s);
      start = ++s;
      continue;
    }
    write(writer, start, s - 1);

    FormatSpec spec;
    spec.align_ = ALIGN_RIGHT;

    // Parse argument index, flags and width.
    unsigned arg_index = parse_header(s, spec);

    // Parse precision.
    if (*s == '.') {
      ++s;
      if ('0' <= *s && *s <= '9') {
        spec.precision_ = static_cast<int>(parse_nonnegative_int(s));
      } else if (*s == '*') {
        ++s;
        spec.precision_ = PrecisionHandler().visit(get_arg(s));
      }
    }

    Arg arg = get_arg(s, arg_index);
    if (spec.flag(HASH_FLAG) && IsZeroInt().visit(arg))
      spec.flags_ &= ~to_unsigned<int>(HASH_FLAG);
    if (spec.fill_ == '0') {
      if (arg.type <= Arg::LAST_NUMERIC_TYPE)
        spec.align_ = ALIGN_NUMERIC;
      else
        spec.fill_ = ' ';  // Ignore '0' flag for non-numeric types.
    }

    // Parse length and convert the argument to the required type.
    switch (*s++) {
    case 'h':
      if (*s == 'h')
        ArgConverter<signed char>(arg, *++s).visit(arg);
      else
        ArgConverter<short>(arg, *s).visit(arg);
      break;
    case 'l':
      if (*s == 'l')
        ArgConverter<fmt::LongLong>(arg, *++s).visit(arg);
      else
        ArgConverter<long>(arg, *s).visit(arg);
      break;
    case 'j':
      ArgConverter<intmax_t>(arg, *s).visit(arg);
      break;
    case 'z':
      ArgConverter<std::size_t>(arg, *s).visit(arg);
      break;
    case 't':
      ArgConverter<std::ptrdiff_t>(arg, *s).visit(arg);
      break;
    case 'L':
      // printf produces garbage when 'L' is omitted for long double, no
      // need to do the same.
      break;
    default:
      --s;
      ArgConverter<void>(arg, *s).visit(arg);
    }

    // Parse type.
    if (!*s)
      FMT_THROW(FormatError("invalid format string"));
    spec.type_ = static_cast<char>(*s++);
    if (arg.type <= Arg::LAST_INTEGER_TYPE) {
      // Normalize type.
      switch (spec.type_) {
      case 'i': case 'u':
        spec.type_ = 'd';
        break;
      case 'c':
        // TODO: handle wchar_t
        CharConverter(arg).visit(arg);
        break;
      }
    }

    start = s;

    // Format argument.
    PAF(writer, spec).visit(arg);
  }
  write(writer, start, s);
}

template void fmt::internal::PrintfFormatter<char>::format(
  BasicWriter<char> &writer, CStringRef format);

template void fmt::internal::PrintfFormatter<wchar_t>::format(
    BasicWriter<wchar_t> &writer, WCStringRef format);

}

template <typename Char, typename PAF>
void fmt::internal::PrintfFormatter<Char,PAF>::parse_flags(
    FormatSpec &spec, const Char *&s) {
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

template <typename Char, typename PAF>
Arg fmt::internal::PrintfFormatter<Char,PAF>::get_arg(
    const Char *s, unsigned arg_index) {
  (void)s;
  const char *error = 0;
  Arg arg = arg_index == UINT_MAX ?
    next_arg(error) : FormatterBase::get_arg(arg_index - 1, error);
  if (error)
    FMT_THROW(FormatError(!*s ? "invalid format string" : error));
  return arg;
}

template <typename Char, typename PAF>
unsigned fmt::internal::PrintfFormatter<Char, PAF>::parse_header(
  const Char *&s, FormatSpec &spec) {
  unsigned arg_index = UINT_MAX;
  Char c = *s;
  if (c >= '0' && c <= '9') {
    // Parse an argument index (if followed by '$') or a width possibly
    // preceded with '0' flag(s).
    unsigned value = parse_nonnegative_int(s);
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
    spec.width_ = parse_nonnegative_int(s);
  } else if (*s == '*') {
    ++s;
    spec.width_ = WidthHandler(spec).visit(get_arg(s));
  }
  return arg_index;
}

#endif
