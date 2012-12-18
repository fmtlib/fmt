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

// Disable useless MSVC warnings.
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "format.h"

#include <stdint.h>

#include <cassert>
#include <cctype>
#include <climits>
#include <cstring>
#include <algorithm>

using std::size_t;
using fmt::Formatter;

#if _MSC_VER
# undef snprintf
# define snprintf _snprintf
#endif

namespace {

// Flags.
enum { PLUS_FLAG = 1, ZERO_FLAG = 2, HEX_PREFIX_FLAG = 4 };

void ReportUnknownType(char code, const char *type) {
  if (std::isprint(static_cast<unsigned char>(code))) {
    throw fmt::FormatError(
        str(fmt::Format("unknown format code '{0}' for {1}") << code << type));
  }
  throw fmt::FormatError(
      str(fmt::Format("unknown format code '\\x{0:02x}' for {1}")
        << static_cast<unsigned>(code) << type));
}

// Information about an integer type.
template <typename T>
struct IntTraits {
  typedef T UnsignedType;
  static bool IsNegative(T) { return false; }
};

template <>
struct IntTraits<int> {
  typedef unsigned UnsignedType;
  static bool IsNegative(int value) { return value < 0; }
};

template <>
struct IntTraits<long> {
  typedef unsigned long UnsignedType;
  static bool IsNegative(long value) { return value < 0; }
};

template <typename T>
struct IsLongDouble { enum {VALUE = 0}; };

template <>
struct IsLongDouble<long double> { enum {VALUE = 1}; };
}

// Throws Exception(message) if format contains '}', otherwise throws
// FormatError reporting unmatched '{'. The idea is that unmatched '{'
// should override other errors.
void Formatter::ReportError(const char *s, const std::string &message) const {
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

template <typename T>
void Formatter::FormatInt(T value, unsigned flags, int width, char type) {
  int size = 0;
  char sign = 0;
  typedef typename IntTraits<T>::UnsignedType UnsignedType;
  UnsignedType abs_value = value;
  if (IntTraits<T>::IsNegative(value)) {
    sign = '-';
    ++size;
    abs_value = -value;
  } else if ((flags & PLUS_FLAG) != 0) {
    sign = '+';
    ++size;
  }
  char fill = (flags & ZERO_FLAG) != 0 ? '0' : ' ';
  size_t start = buffer_.size();
  char *p = 0;
  switch (type) {
  case 0: case 'd': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n /= 10) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n % 10);
    } while ((n /= 10) != 0);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = (flags & HEX_PREFIX_FLAG) != 0;
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    const char *digits = type == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    if (print_prefix) {
      *p-- = type;
      *p-- = '0';
    }
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    width = std::max(width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    break;
  }
  default:
    ReportUnknownType(type, "integer");
    break;
  }
  if (sign) {
    if ((flags & ZERO_FLAG) != 0)
      buffer_[start++] = sign;
    else
      *p-- = sign;
  }
  std::fill(&buffer_[start], p + 1, fill);
}

template <typename T>
void Formatter::FormatDouble(
    T value, unsigned flags, int width, int precision, char type) {
  // Check type.
  switch (type) {
  case 0:
    type = 'g';
    break;
  case 'e': case 'E': case 'f': case 'g': case 'G':
    break;
  case 'F':
#ifdef _MSC_VER
    // MSVC's printf doesn't support 'F'.
    type = 'f';
#endif
    break;
  default:
    ReportUnknownType(type, "double");
    break;
  }

  // Build format string.
  enum { MAX_FORMAT_SIZE = 9}; // longest format: %+0*.*Lg
  char format[MAX_FORMAT_SIZE];
  char *format_ptr = format;
  *format_ptr++ = '%';
  if ((flags & PLUS_FLAG) != 0)
    *format_ptr++ = '+';
  if ((flags & ZERO_FLAG) != 0)
    *format_ptr++ = '0';
  if (width > 0)
    *format_ptr++ = '*';
  if (precision >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (IsLongDouble<T>::VALUE)
    *format_ptr++ = 'L';
  *format_ptr++ = type;
  *format_ptr = '\0';

  // Format using snprintf.
  size_t offset = buffer_.size();
  for (;;) {
    size_t size = buffer_.capacity() - offset;
    int n = 0;
    if (width <= 0) {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, value) :
          snprintf(&buffer_[offset], size, format, precision, value);
    } else {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, width, value) :
          snprintf(&buffer_[offset], size, format, width, precision, value);
    }
    if (n >= 0 && offset + n < buffer_.capacity()) {
      GrowBuffer(n);
      return;
    }
    buffer_.reserve(n >= 0 ? offset + n + 1 : 2 * buffer_.capacity());
  }
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
unsigned Formatter::ParseUInt(const char *&s) const {
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

const Formatter::Arg &Formatter::ParseArgIndex(const char *&s) const {
  if (*s < '0' || *s > '9')
    ReportError(s, "missing argument index in format string");
  unsigned arg_index = ParseUInt(s);
  if (arg_index >= args_.size())
    ReportError(s, "argument index is out of range in format");
  return *args_[arg_index];
}

void Formatter::DoFormat() {
  const char *start = format_;
  format_ = 0;
  const char *s = start;
  while (*s) {
    char c = *s++;
    if (c != '{' && c != '}') continue;
    if (*s == c) {
      buffer_.append(start, s);
      start = ++s;
      continue;
    }
    if (c == '}')
      throw FormatError("unmatched '}' in format");
    num_open_braces_= 1;
    buffer_.append(start, s - 1);

    const Arg &arg = ParseArgIndex(s);

    unsigned flags = 0;
    int width = 0;
    int precision = -1;
    char type = 0;
    if (*s == ':') {
      ++s;
      if (*s == '+') {
        ++s;
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '+' requires numeric argument");
        if (arg.type == UINT || arg.type == ULONG) {
          ReportError(s,
              "format specifier '+' requires signed argument");
        }
        flags |= PLUS_FLAG;
      }
      if (*s == '0') {
        ++s;
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '0' requires numeric argument");
        flags |= ZERO_FLAG;
      }

      // Parse width.
      if ('0' <= *s && *s <= '9') {
        unsigned value = ParseUInt(s);
        if (value > INT_MAX)
          ReportError(s, "number is too big in format");
        width = value;
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
        type = *s++;
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case INT:
      FormatInt(arg.int_value, flags, width, type);
      break;
    case UINT:
      FormatInt(arg.uint_value, flags, width, type);
      break;
    case LONG:
      FormatInt(arg.long_value, flags, width, type);
      break;
    case ULONG:
      FormatInt(arg.ulong_value, flags, width, type);
      break;
    case DOUBLE:
      FormatDouble(arg.double_value, flags, width, precision, type);
      break;
    case LONG_DOUBLE:
      FormatDouble(arg.long_double_value, flags, width, precision, type);
      break;
    case CHAR: {
      if (type && type != 'c')
        ReportUnknownType(type, "char");
      char *out = GrowBuffer(std::max(width, 1));
      *out++ = arg.int_value;
      if (width > 1)
        std::fill_n(out, width - 1, ' ');
      break;
    }
    case STRING: {
      if (type && type != 's')
        ReportUnknownType(type, "string");
      const char *str = arg.string.value;
      size_t size = arg.string.size;
      if (size == 0) {
        if (!str)
          throw FormatError("string pointer is null");
        if (*str)
          size = std::strlen(str);
      }
      char *out = GrowBuffer(std::max<size_t>(width, size));
      out = std::copy(str, str + size, out);
      if (static_cast<unsigned>(width) > size)
        std::fill_n(out, width - size, ' ');
      break;
    }
    case POINTER:
      if (type && type != 'p')
        ReportUnknownType(type, "pointer");
      FormatInt(reinterpret_cast<uintptr_t>(
          arg.pointer_value), HEX_PREFIX_FLAG, width, 'x');
      break;
    case CUSTOM:
      if (type)
        ReportUnknownType(type, "object");
      (this->*arg.custom.format)(arg.custom.value, width);
      break;
    default:
      assert(false);
      break;
    }
  }
  buffer_.append(start, s + 1);
  buffer_.resize(buffer_.size() - 1);  // Don't count the terminating zero.
}

void Formatter::Write(const std::string &s, unsigned width) {
  char *out = GrowBuffer(std::max<std::size_t>(width, s.size()));
  std::copy(s.begin(), s.end(), out);
  if (width > s.size())
    std::fill_n(out + s.size(), width - s.size(), ' ');
}
