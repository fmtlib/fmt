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

// Disable useless MSVC warnings.
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#undef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#include "format.h"

#include <string.h>

#include <cctype>
#include <climits>
#include <cmath>
#include <cstdarg>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# ifdef __MINGW32__
#  include <cstring>
# endif
# include <windows.h>
# undef ERROR
#endif

using fmt::ULongLong;

#if _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4127) // conditional expression is constant
#endif

namespace {

#ifndef _MSC_VER

inline int SignBit(double value) {
  // When compiled in C++11 mode signbit is no longer a macro but a function
  // defined in namespace std and the macro is undefined.
#ifdef signbit
  return signbit(value);
#else
  return std::signbit(value);
#endif
}

inline int IsInf(double x) {
#ifdef isinf
  return isinf(x);
#else
  return std::isinf(x);
#endif
}

#define FMT_SNPRINTF snprintf

#else  // _MSC_VER

inline int SignBit(double value) {
  if (value < 0) return 1;
  if (value == value) return 0;
  int dec = 0, sign = 0;
  char buffer[2];  // The buffer size must be >= 2 or _ecvt_s will fail.
  _ecvt_s(buffer, sizeof(buffer), value, 0, &dec, &sign);
  return sign;
}

inline int IsInf(double x) { return !_finite(x); }

inline int FMT_SNPRINTF(char *buffer, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(buffer, size, _TRUNCATE, format, args);
  va_end(args);
  return result;
}

#endif  // _MSC_VER

const char RESET_COLOR[] = "\x1b[0m";

typedef void (*FormatFunc)(fmt::Writer &, int , fmt::StringRef);

void ReportError(FormatFunc func,
    int error_code, fmt::StringRef message) FMT_NOEXCEPT(true) {
  try {
    fmt::Writer full_message;
    func(full_message, error_code, message); // TODO: make sure this doesn't throw
    std::fwrite(full_message.c_str(), full_message.size(), 1, stderr);
    std::fputc('\n', stderr);
  } catch (...) {}
}

const fmt::internal::ArgInfo DUMMY_ARG = {fmt::internal::ArgInfo::INT, 0};

fmt::ULongLong GetIntValue(const fmt::internal::ArgInfo &arg) {
  typedef fmt::internal::ArgInfo Arg;
  switch (arg.type) {
    case Arg::INT:
      return arg.int_value;
    case Arg::UINT:
      return arg.uint_value;
    case Arg::LONG_LONG:
      return arg.long_long_value;
    case Arg::ULONG_LONG:
      return arg.ulong_long_value;
    default:
      return -1;
  }
}
}  // namespace

int fmt::internal::SignBitNoInline(double value) { return SignBit(value); }

void fmt::SystemError::init(
    int error_code, StringRef format_str, const ArgList &args) {
  error_code_ = error_code;
  Writer w;
  internal::FormatSystemErrorMessage(w, error_code, format(format_str, args));
  std::runtime_error &base = *this;
  base = std::runtime_error(w.str());
}

template <typename T>
int fmt::internal::CharTraits<char>::FormatFloat(
    char *buffer, std::size_t size, const char *format,
    unsigned width, int precision, T value) {
  if (width == 0) {
    return precision < 0 ?
        FMT_SNPRINTF(buffer, size, format, value) :
        FMT_SNPRINTF(buffer, size, format, precision, value);
  }
  return precision < 0 ?
      FMT_SNPRINTF(buffer, size, format, width, value) :
      FMT_SNPRINTF(buffer, size, format, width, precision, value);
}

template <typename T>
int fmt::internal::CharTraits<wchar_t>::FormatFloat(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    unsigned width, int precision, T value) {
  if (width == 0) {
    return precision < 0 ?
        swprintf(buffer, size, format, value) :
        swprintf(buffer, size, format, precision, value);
  }
  return precision < 0 ?
      swprintf(buffer, size, format, width, value) :
      swprintf(buffer, size, format, width, precision, value);
}

const char fmt::internal::DIGITS[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

#define FMT_POWERS_OF_10(factor) \
  factor * 10, \
  factor * 100, \
  factor * 1000, \
  factor * 10000, \
  factor * 100000, \
  factor * 1000000, \
  factor * 10000000, \
  factor * 100000000, \
  factor * 1000000000

const uint32_t fmt::internal::POWERS_OF_10_32[] = {0, FMT_POWERS_OF_10(1)};
const uint64_t fmt::internal::POWERS_OF_10_64[] = {
  0,
  FMT_POWERS_OF_10(1),
  FMT_POWERS_OF_10(ULongLong(1000000000)),
  // Multiply several constants instead of using a single long long constants
  // to avoid warnings about C++98 not supporting long long.
  ULongLong(1000000000) * ULongLong(1000000000) * 10
};

void fmt::internal::ReportUnknownType(char code, const char *type) {
  if (std::isprint(static_cast<unsigned char>(code))) {
    throw fmt::FormatError(
        fmt::format("unknown format code '{}' for {}", code, type));
  }
  throw fmt::FormatError(
      fmt::format("unknown format code '\\x{:02x}' for {}",
        static_cast<unsigned>(code), type));
}

#ifdef _WIN32

fmt::internal::UTF8ToUTF16::UTF8ToUTF16(fmt::StringRef s) {
  int length = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, 0, 0);
  static const char ERROR[] = "cannot convert string from UTF-8 to UTF-16";
  if (length == 0)
    throw WindowsError(GetLastError(), ERROR);
  buffer_.resize(length);
  length = MultiByteToWideChar(
    CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, &buffer_[0], length);
  if (length == 0)
    throw WindowsError(GetLastError(), ERROR);
}

fmt::internal::UTF16ToUTF8::UTF16ToUTF8(fmt::WStringRef s) {
  if (int error_code = Convert(s)) {
    throw WindowsError(GetLastError(),
        "cannot convert string from UTF-16 to UTF-8");
  }
}

int fmt::internal::UTF16ToUTF8::Convert(fmt::WStringRef s) {
  int length = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, 0, 0, 0, 0);
  if (length == 0)
    return GetLastError();
  buffer_.resize(length);
  length = WideCharToMultiByte(
    CP_UTF8, 0, s.c_str(), -1, &buffer_[0], length, 0, 0);
  if (length == 0)
    return GetLastError();
  return 0;
}

void fmt::WindowsError::init(
    int error_code, StringRef format_str, const ArgList &args) {
  error_code_ = error_code;
  Writer w;
  internal::FormatWinErrorMessage(w, error_code, format(format_str, args));
  std::runtime_error &base = *this;
  base = std::runtime_error(w.str());
}

#endif

int fmt::internal::StrError(
    int error_code, char *&buffer, std::size_t buffer_size) FMT_NOEXCEPT(true) {
  assert(buffer != 0 && buffer_size != 0);
  int result = 0;
#ifdef _GNU_SOURCE
  char *message = strerror_r(error_code, buffer, buffer_size);
  // If the buffer is full then the message is probably truncated.
  if (message == buffer && strlen(buffer) == buffer_size - 1)
    result = ERANGE;
  buffer = message;
#elif _WIN32
# ifdef __MINGW32__
  strerror(result);
# else
  result = strerror_s(buffer, buffer_size, error_code);
# endif
  // If the buffer is full then the message is probably truncated.
  if (result == 0 && std::strlen(buffer) == buffer_size - 1)
    result = ERANGE;
#else
  result = strerror_r(error_code, buffer, buffer_size);
  if (result == -1)
    result = errno;  // glibc versions before 2.13 return result in errno.
#endif
  return result;
}

void fmt::internal::FormatSystemErrorMessage(
    fmt::Writer &out, int error_code, fmt::StringRef message) {
  Array<char, INLINE_BUFFER_SIZE> buffer;
  buffer.resize(INLINE_BUFFER_SIZE);
  char *system_message = 0;
  for (;;) {
    system_message = &buffer[0];
    int result = StrError(error_code, system_message, buffer.size());
    if (result == 0)
      break;
    if (result != ERANGE) {
      // Can't get error message, report error code instead.
      out << message << ": error code = " << error_code;
      return;
    }
    buffer.resize(buffer.size() * 2);
  }
  out << message << ": " << system_message;
}

#ifdef _WIN32
void fmt::internal::FormatWinErrorMessage(
    fmt::Writer &out, int error_code, fmt::StringRef message) {
  class String {
   private:
    LPWSTR str_;

   public:
    String() : str_() {}
    ~String() { LocalFree(str_); }
    LPWSTR *ptr() { return &str_; }
    LPCWSTR c_str() const { return str_; }
  };
  String system_message;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(system_message.ptr()), 0, 0)) {
    UTF16ToUTF8 utf8_message;
    if (!utf8_message.Convert(system_message.c_str())) {
      out << message << ": " << utf8_message;
      return;
    }
  }
  // Can't get error message, report error code instead.
  out << message << ": error code = " << error_code;
}
#endif

template <typename Char>
void fmt::internal::FormatErrorReporter<Char>::operator()(
        const Char *s, fmt::StringRef message) const {
  for (int n = num_open_braces; *s; ++s) {
    if (*s == '{') {
      ++n;
    } else if (*s == '}') {
      if (--n == 0)
        throw fmt::FormatError(message);
    }
  }
  throw fmt::FormatError("unmatched '{' in format");
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
template <typename Char>
int fmt::internal::ParseNonnegativeInt(
    const Char *&s, const char *&error) FMT_NOEXCEPT(true) {
  assert('0' <= *s && *s <= '9');
  unsigned value = 0;
  do {
    unsigned new_value = value * 10 + (*s++ - '0');
    // Check if value wrapped around.
    value = new_value >= value ? new_value : UINT_MAX;
  } while ('0' <= *s && *s <= '9');
  if (value > INT_MAX) {
    if (!error)
      error = "number is too big in format";
    return 0;
  }
  return value;
}

// Fills the padding around the content and returns the pointer to the
// content area.
template <typename Char>
typename fmt::BasicWriter<Char>::CharPtr
  fmt::BasicWriter<Char>::FillPadding(CharPtr buffer,
    unsigned total_size, std::size_t content_size, wchar_t fill) {
  std::size_t padding = total_size - content_size;
  std::size_t left_padding = padding / 2;
  Char fill_char = static_cast<Char>(fill);
  std::fill_n(buffer, left_padding, fill_char);
  buffer += left_padding;
  CharPtr content = buffer;
  std::fill_n(buffer + content_size, padding - left_padding, fill_char);
  return content;
}

template <typename Char>
template <typename T>
void fmt::BasicWriter<Char>::FormatDouble(T value, const FormatSpec &spec) {
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
#ifdef _MSC_VER
    // MSVC's printf doesn't support 'F'.
    type = 'f';
#endif
    // Fall through.
  case 'E': case 'G': case 'A':
    upper = true;
    break;
  default:
    internal::ReportUnknownType(type, "double");
    break;
  }

  char sign = 0;
  // Use SignBit instead of value < 0 because the latter is always
  // false for NaN.
  if (SignBit(static_cast<double>(value))) {
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
    CharPtr out = write_str(nan, size, spec);
    if (sign)
      *out = sign;
    return;
  }

  if (IsInf(static_cast<double>(value))) {
    // Format infinity ourselves because sprintf's output is not consistent
    // across platforms.
    std::size_t size = 4;
    const char *inf = upper ? " INF" : " inf";
    if (!sign) {
      --size;
      ++inf;
    }
    CharPtr out = write_str(inf, size, spec);
    if (sign)
      *out = sign;
    return;
  }

  std::size_t offset = buffer_.size();
  unsigned width = spec.width();
  if (sign) {
    buffer_.reserve(buffer_.size() + (std::max)(width, 1u));
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
  if (spec.precision() >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (internal::IsLongDouble<T>::VALUE)
    *format_ptr++ = 'L';
  *format_ptr++ = type;
  *format_ptr = '\0';

  // Format using snprintf.
  Char fill = static_cast<Char>(spec.fill());
  for (;;) {
    std::size_t size = buffer_.capacity() - offset;
#if _MSC_VER
    // MSVC's vsnprintf_s doesn't work with zero size, so reserve
    // space for at least one extra character to make the size non-zero.
    // Note that the buffer's capacity will increase by more than 1.
    if (size == 0) {
      buffer_.reserve(offset + 1);
      size = buffer_.capacity() - offset;
    }
#endif
    Char *start = &buffer_[offset];
    int n = internal::CharTraits<Char>::FormatFloat(
        start, size, format, width_for_sprintf, spec.precision(), value);
    if (n >= 0 && offset + n < buffer_.capacity()) {
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
      if (spec.align() == ALIGN_CENTER &&
          spec.width() > static_cast<unsigned>(n)) {
        unsigned width = spec.width();
        CharPtr p = GrowBuffer(width);
        std::copy(p, p + n, p + (width - n) / 2);
        FillPadding(p, spec.width(), n, fill);
        return;
      }
      if (spec.fill() != ' ' || sign) {
        while (*start == ' ')
          *start++ = fill;
        if (sign)
          *(start - 1) = sign;
      }
      GrowBuffer(n);
      return;
    }
    // If n is negative we ask to increase the capacity by at least 1,
    // but as std::vector, the buffer grows exponentially.
    buffer_.reserve(n >= 0 ? offset + n + 1 : buffer_.capacity() + 1);
  }
}

template <typename Char>
template <typename StringChar>
void fmt::BasicWriter<Char>::write_str(
    const internal::StringValue<StringChar> &str, const FormatSpec &spec) {
  if (spec.type_ && spec.type_ != 's')
    internal::ReportUnknownType(spec.type_, "string");
  const StringChar *s = str.value;
  std::size_t size = str.size;
  if (size == 0) {
    if (!s)
      throw FormatError("string pointer is null");
    if (*s)
      size = std::char_traits<StringChar>::length(s);
  }
  write_str(s, size, spec);
}

template <typename Char>
inline const typename fmt::BasicWriter<Char>::Arg
    &fmt::BasicWriter<Char>::FormatParser::ParseArgIndex(const Char *&s) {
  unsigned arg_index = 0;
  if (*s < '0' || *s > '9') {
    if (*s != '}' && *s != ':')
      report_error_(s, "invalid argument index in format string");
    if (next_arg_index_ < 0) {
      report_error_(s,
          "cannot switch from manual to automatic argument indexing");
    }
    arg_index = next_arg_index_++;
  } else {
    if (next_arg_index_ > 0) {
      report_error_(s,
          "cannot switch from automatic to manual argument indexing");
    }
    next_arg_index_ = -1;
    const char *error = 0;
    arg_index = internal::ParseNonnegativeInt(s, error);
    if (error)
      report_error_(s, error); // TODO
  }
  if (arg_index >= args_.size())
    report_error_(s, "argument index is out of range in format");
  return args_[arg_index];
}

template <typename Char>
void fmt::BasicWriter<Char>::FormatParser::CheckSign(
    const Char *&s, const Arg &arg) {
  char sign = static_cast<char>(*s);
  if (arg.type > Arg::LAST_NUMERIC_TYPE) {
    report_error_(s,
        fmt::format("format specifier '{}' requires numeric argument", sign).c_str());
  }
  if (arg.type == Arg::UINT || arg.type == Arg::ULONG_LONG) {
    report_error_(s,
        fmt::format("format specifier '{}' requires signed argument", sign).c_str());
  }
  ++s;
}

template <typename Char>
void fmt::internal::PrintfParser<Char>::ParseFlags(
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

template <typename Char>
unsigned fmt::internal::PrintfParser<Char>::ParseHeader(
  const Char *&s, FormatSpec &spec, const char *&error) {
  unsigned arg_index = UINT_MAX;
  Char c = *s;
  if (c >= '0' && c <= '9') {
    // Parse an argument index (if followed by '$') or a width possibly
    // preceded with '0' flag(s).
    unsigned value = internal::ParseNonnegativeInt(s, error);
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
  ParseFlags(spec, s);
  // Parse width.
  if (*s >= '0' && *s <= '9') {
    spec.width_ = internal::ParseNonnegativeInt(s, error);
  } else if (*s == '*') {
    ++s;
    const Arg &arg = HandleArgIndex(UINT_MAX, error);
    // TODO: use ArgVisitor
    ULongLong width = 0;
    switch (arg.type) {
    case Arg::INT:
      width = arg.int_value;
      if (arg.int_value < 0) {
        spec.align_ = ALIGN_LEFT;
        width = 0 - width;
      }
      break;
    case Arg::UINT:
      width = arg.uint_value;
      break;
    case Arg::LONG_LONG:
      width = arg.long_long_value;
      if (arg.long_long_value < 0) {
        spec.align_ = ALIGN_LEFT;
        width = 0 - width;
      }
      break;
    case Arg::ULONG_LONG:
      width = arg.ulong_long_value;
      break;
    default:
      if (!error)
        error = "width is not integer";
    }
    if (width <= INT_MAX)
      spec.width_ = static_cast<unsigned>(width);
    else if (!error)
      error = "number is too big in format";
  }
  return arg_index;
}

// TODO: move to a base class that doesn't depend on template argument
template <typename Char>
const fmt::internal::ArgInfo
  &fmt::internal::PrintfParser<Char>::HandleArgIndex(
    unsigned arg_index, const char *&error) {
  if (arg_index != UINT_MAX) {
    if (next_arg_index_ <= 0) {
      next_arg_index_ = -1;
      --arg_index;
    } else if (!error) {
      error = "cannot switch from automatic to manual argument indexing";
    }
  } else if (next_arg_index_ >= 0) {
    arg_index = next_arg_index_++;
  } else if (!error) {
    error = "cannot switch from manual to automatic argument indexing";
  }
  if (arg_index < args_.size())
    return args_[arg_index];
  if (!error)
    error = "argument index is out of range in format";
  return DUMMY_ARG;
}

template <typename Char>
void fmt::internal::PrintfParser<Char>::Format(
    BasicWriter<Char> &writer, BasicStringRef<Char> format,
    const ArgList &args) {
  const Char *start = format.c_str();
  args_ = args;
  next_arg_index_ = 0;
  const Char *s = start;
  while (*s) {
    Char c = *s++;
    if (c != '%') continue;
    if (*s == c) {
      writer.buffer_.append(start, s);
      start = ++s;
      continue;
    }
    writer.buffer_.append(start, s - 1);

    FormatSpec spec;
    spec.align_ = ALIGN_RIGHT;

    // Reporting errors is delayed till the format specification is
    // completely parsed. This is done to avoid potentially confusing
    // error messages for incomplete format strings. For example, in
    //   sprintf("%2$", 42);
    // the format specification is incomplete. In naive approach we
    // would parse 2 as an argument index and report an error that the
    // index is out of range which would be rather confusing if the
    // use meant "%2d$" rather than "%2$d". If we delay an error, the
    // user will get an error that the format string is invalid which
    // is OK for both cases.
    const char *error = 0;

    // Parse argument index, flags and width.
    unsigned arg_index = ParseHeader(s, spec, error);

    // Parse precision.
    if (*s == '.') {
      ++s;
      if ('0' <= *s && *s <= '9') {
        spec.precision_ = internal::ParseNonnegativeInt(s, error);
      } else if (*s == '*') {
        ++s;
        const Arg &arg = HandleArgIndex(UINT_MAX, error);
        if (arg.type <= Arg::LAST_INTEGER_TYPE)
          spec.precision_ = static_cast<int>(GetIntValue(arg)); // TODO: check for overflow
        else if (!error)
          error = "precision is not integer";
      }
    }

    const Arg &arg = HandleArgIndex(arg_index, error);
    if (spec.hash_flag() && GetIntValue(arg) == 0)
      spec.flags_ &= ~HASH_FLAG;
    if (spec.fill_ == '0') {
      if (arg.type <= Arg::LAST_NUMERIC_TYPE)
        spec.align_ = ALIGN_NUMERIC;
      else
        spec.fill_ = ' ';  // Ignore '0' flag for non-numeric types.
    }

    // Parse length.
    switch (*s) {
    case 'h':
      // TODO: convert to short
    case 'l':
    case 'j':
    case 'z':
    case 't':
    case 'L':
      // TODO: handle length
      ++s;
      break;
    }

    // Parse type.
    if (!*s)
      throw FormatError("invalid format string");
    if (error)
      throw FormatError(error);
    spec.type_ = static_cast<char>(*s++);

    start = s;

    // Format argument.
    switch (arg.type) {
    case Arg::INT:
      writer.FormatInt(arg.int_value, spec);
      break;
    case Arg::UINT:
      writer.FormatInt(arg.uint_value, spec);
      break;
    case Arg::LONG_LONG:
      writer.FormatInt(arg.long_long_value, spec);
      break;
    case Arg::ULONG_LONG:
      writer.FormatInt(arg.ulong_long_value, spec);
      break;
    case Arg::DOUBLE:
      writer.FormatDouble(arg.double_value, spec);
      break;
    case Arg::LONG_DOUBLE:
      writer.FormatDouble(arg.long_double_value, spec);
      break;
    case Arg::CHAR: {
      if (spec.type_ && spec.type_ != 'c')
        internal::ReportUnknownType(spec.type_, "char");
      typedef typename BasicWriter<Char>::CharPtr CharPtr;
      CharPtr out = CharPtr();
      if (spec.width_ > 1) {
        Char fill = static_cast<Char>(spec.fill());
        out = writer.GrowBuffer(spec.width_);
        if (spec.align_ == ALIGN_RIGHT) {
          std::fill_n(out, spec.width_ - 1, fill);
          out += spec.width_ - 1;
        } else if (spec.align_ == ALIGN_CENTER) {
          out = writer.FillPadding(out, spec.width_, 1, fill);
        } else {
          std::fill_n(out + 1, spec.width_ - 1, fill);
        }
      } else {
        out = writer.GrowBuffer(1);
      }
      *out = static_cast<Char>(arg.int_value);
      break;
    }
    case Arg::STRING:
      writer.write_str(arg.string, spec);
      break;
    case Arg::WSTRING:
      writer.write_str(internal::CharTraits<Char>::convert(arg.wstring), spec);
      break;
    case Arg::POINTER:
      if (spec.type_ && spec.type_ != 'p')
        internal::ReportUnknownType(spec.type_, "pointer");
      spec.flags_= HASH_FLAG;
      spec.type_ = 'x';
      writer.FormatInt(reinterpret_cast<uintptr_t>(arg.pointer_value), spec);
      break;
    case Arg::CUSTOM:
      if (spec.type_)
        internal::ReportUnknownType(spec.type_, "object");
      arg.custom.format(&writer, arg.custom.value, spec);
      break;
    default:
      assert(false);
      break;
    }
  }
  writer.buffer_.append(start, s);
}

template <typename Char>
void fmt::BasicWriter<Char>::FormatParser::Format(
    BasicWriter<Char> &writer, BasicStringRef<Char> format,
    const ArgList &args) {
  const char *error = 0;
  const Char *start = format.c_str();
  args_ = args;
  next_arg_index_ = 0;
  const Char *s = start;
  while (*s) {
    Char c = *s++;
    if (c != '{' && c != '}') continue;
    if (*s == c) {
      writer.buffer_.append(start, s);
      start = ++s;
      continue;
    }
    if (c == '}')
      throw FormatError("unmatched '}' in format");
    report_error_.num_open_braces = 1;
    writer.buffer_.append(start, s - 1);

    const Arg &arg = ParseArgIndex(s);

    FormatSpec spec;
    if (*s == ':') {
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
                report_error_(s, "invalid fill character '{'");
              s += 2;
              spec.fill_ = c;
            } else ++s;
            if (spec.align_ == ALIGN_NUMERIC && arg.type > Arg::LAST_NUMERIC_TYPE)
              report_error_(s, "format specifier '=' requires numeric argument");
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
        if (arg.type > Arg::LAST_NUMERIC_TYPE)
          report_error_(s, "format specifier '#' requires numeric argument");
        spec.flags_ |= HASH_FLAG;
        ++s;
      }

      // Parse width and zero flag.
      if ('0' <= *s && *s <= '9') {
        if (*s == '0') {
          if (arg.type > Arg::LAST_NUMERIC_TYPE)
            report_error_(s, "format specifier '0' requires numeric argument");
          spec.align_ = ALIGN_NUMERIC;
          spec.fill_ = '0';
        }
        // Zero may be parsed again as a part of the width, but it is simpler
        // and more efficient than checking if the next char is a digit.
        spec.width_ = internal::ParseNonnegativeInt(s, error);
        if (error)
          report_error_(s, error);
      }

      // Parse precision.
      if (*s == '.') {
        ++s;
        spec.precision_ = 0;
        if ('0' <= *s && *s <= '9') {
          spec.precision_ = internal::ParseNonnegativeInt(s, error);
          if (error)
            report_error_(s, error);
        } else if (*s == '{') {
          ++s;
          ++report_error_.num_open_braces;
          const Arg &precision_arg = ParseArgIndex(s);
          ULongLong value = 0;
          switch (precision_arg.type) {
          case Arg::INT:
            if (precision_arg.int_value < 0)
              report_error_(s, "negative precision in format");
            value = precision_arg.int_value;
            break;
          case Arg::UINT:
            value = precision_arg.uint_value;
            break;
          case Arg::LONG_LONG:
            if (precision_arg.long_long_value < 0)
              report_error_(s, "negative precision in format");
            value = precision_arg.long_long_value;
            break;
          case Arg::ULONG_LONG:
            value = precision_arg.ulong_long_value;
            break;
          default:
            report_error_(s, "precision is not integer");
          }
          if (value > INT_MAX)
            report_error_(s, "number is too big in format");
          spec.precision_ = static_cast<int>(value);
          if (*s++ != '}')
            throw FormatError("unmatched '{' in format");
          --report_error_.num_open_braces;
        } else {
          report_error_(s, "missing precision in format");
        }
        if (arg.type != Arg::DOUBLE && arg.type != Arg::LONG_DOUBLE) {
          report_error_(s,
              "precision specifier requires floating-point argument");
        }
      }

      // Parse type.
      if (*s != '}' && *s)
        spec.type_ = static_cast<char>(*s++);
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case Arg::INT:
      writer.FormatInt(arg.int_value, spec);
      break;
    case Arg::UINT:
      writer.FormatInt(arg.uint_value, spec);
      break;
    case Arg::LONG_LONG:
      writer.FormatInt(arg.long_long_value, spec);
      break;
    case Arg::ULONG_LONG:
      writer.FormatInt(arg.ulong_long_value, spec);
      break;
    case Arg::DOUBLE:
      writer.FormatDouble(arg.double_value, spec);
      break;
    case Arg::LONG_DOUBLE:
      writer.FormatDouble(arg.long_double_value, spec);
      break;
    case Arg::CHAR: {
      if (spec.type_ && spec.type_ != 'c')
        internal::ReportUnknownType(spec.type_, "char");
      typedef typename BasicWriter<Char>::CharPtr CharPtr;
      CharPtr out = CharPtr();
      if (spec.width_ > 1) {
        Char fill = static_cast<Char>(spec.fill());
        out = writer.GrowBuffer(spec.width_);
        if (spec.align_ == ALIGN_RIGHT) {
          std::fill_n(out, spec.width_ - 1, fill);
          out += spec.width_ - 1;
        } else if (spec.align_ == ALIGN_CENTER) {
          out = writer.FillPadding(out, spec.width_, 1, fill);
        } else {
          std::fill_n(out + 1, spec.width_ - 1, fill);
        }
      } else {
        out = writer.GrowBuffer(1);
      }
      *out = static_cast<Char>(arg.int_value);
      break;
    }
    case Arg::STRING:
      writer.write_str(arg.string, spec);
      break;
    case Arg::WSTRING:
      writer.write_str(internal::CharTraits<Char>::convert(arg.wstring), spec);
      break;
    case Arg::POINTER:
      if (spec.type_ && spec.type_ != 'p')
        internal::ReportUnknownType(spec.type_, "pointer");
      spec.flags_= HASH_FLAG;
      spec.type_ = 'x';
      writer.FormatInt(reinterpret_cast<uintptr_t>(arg.pointer_value), spec);
      break;
    case Arg::CUSTOM:
      if (spec.type_)
        internal::ReportUnknownType(spec.type_, "object");
      arg.custom.format(&writer, arg.custom.value, spec);
      break;
    default:
      assert(false);
      break;
    }
  }
  writer.buffer_.append(start, s);
}

void fmt::ReportSystemError(
    int error_code, fmt::StringRef message) FMT_NOEXCEPT(true) {
  // FIXME: FormatSystemErrorMessage may throw
  ReportError(internal::FormatSystemErrorMessage, error_code, message);
}

#ifdef _WIN32
void fmt::WinErrorSink::operator()(const Writer &w) const {
  throw WindowsError(error_code_, w.c_str());
}

void fmt::ReportWinError(
    int error_code, fmt::StringRef message) FMT_NOEXCEPT(true) {
  // FIXME: FormatWinErrorMessage may throw
  ReportError(internal::FormatWinErrorMessage, error_code, message);
}
#endif

void fmt::ANSITerminalSink::operator()(
    const fmt::BasicWriter<char> &w) const {
  char escape[] = "\x1b[30m";
  escape[3] = '0' + static_cast<char>(color_);
  std::fputs(escape, file_);
  std::fwrite(w.data(), 1, w.size(), file_);
  std::fputs(RESET_COLOR, file_);
}

void fmt::print(StringRef format, const ArgList &args) {
  Writer w;
  w.write(format, args);
  std::fwrite(w.data(), 1, w.size(), stdout);
}

void fmt::print(std::FILE *f, StringRef format, const ArgList &args) {
  Writer w;
  w.write(format, args);
  std::fwrite(w.data(), 1, w.size(), f);
}

void fmt::printf(StringRef format, const ArgList &args) {
  Writer w;
  printf(w, format, args);
  std::fwrite(w.data(), 1, w.size(), stdout);
}

// Explicit instantiations for char.

template fmt::BasicWriter<char>::CharPtr
  fmt::BasicWriter<char>::FillPadding(CharPtr buffer,
    unsigned total_size, std::size_t content_size, wchar_t fill);

template void fmt::BasicWriter<char>::FormatParser::Format(
  BasicWriter<char> &writer, BasicStringRef<char> format, const ArgList &args);

template void fmt::internal::PrintfParser<char>::Format(
  BasicWriter<char> &writer, BasicStringRef<char> format, const ArgList &args);

// Explicit instantiations for wchar_t.

template fmt::BasicWriter<wchar_t>::CharPtr
  fmt::BasicWriter<wchar_t>::FillPadding(CharPtr buffer,
    unsigned total_size, std::size_t content_size, wchar_t fill);

template void fmt::BasicWriter<wchar_t>::FormatParser::Format(
    BasicWriter<wchar_t> &writer, BasicStringRef<wchar_t> format,
    const ArgList &args);

template void fmt::internal::PrintfParser<wchar_t>::Format(
    BasicWriter<wchar_t> &writer, BasicStringRef<wchar_t> format,
    const ArgList &args);

#if _MSC_VER
# pragma warning(pop)
#endif
