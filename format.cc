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
#undef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#include "format.h"

#include <stdint.h>

#include <cassert>
#include <cctype>
#include <climits>
#include <cstring>
#include <algorithm>

using std::size_t;
using fmt::Formatter;
using fmt::FormatSpec;

#if _MSC_VER
# undef snprintf
# define snprintf _snprintf
#endif

namespace {

// Flags.
enum { PLUS_FLAG = 1, ZERO_FLAG = 2, HEX_PREFIX_FLAG = 4 };

// Alignment.
enum Alignment {
  ALIGN_DEFAULT, ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER, ALIGN_NUMERIC
};

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
void Formatter::FormatInt(T value, FormatSpec spec) {
  unsigned size = 0;
  char sign = 0;
  typedef typename IntTraits<T>::UnsignedType UnsignedType;
  UnsignedType abs_value = value;
  if (IntTraits<T>::IsNegative(value)) {
    sign = '-';
    ++size;
    abs_value = 0 - abs_value;
  } else if ((spec.flags & PLUS_FLAG) != 0) {
    sign = '+';
    ++size;
  }
  size_t start = buffer_.size();
  char *p = 0;
  switch (spec.type) {
  case 0: case 'd': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n /= 10) != 0);
    unsigned width = std::max(spec.width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n % 10);
    } while ((n /= 10) != 0);
    break;
  }
  case 'x': case 'X': {
    UnsignedType n = abs_value;
    bool print_prefix = (spec.flags & HEX_PREFIX_FLAG) != 0;
    if (print_prefix) size += 2;
    do {
      ++size;
    } while ((n >>= 4) != 0);
    unsigned width = std::max(spec.width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    const char *digits = spec.type == 'x' ?
        "0123456789abcdef" : "0123456789ABCDEF";
    do {
      *p-- = digits[n & 0xf];
    } while ((n >>= 4) != 0);
    if (print_prefix) {
      *p-- = spec.type;
      *p-- = '0';
    }
    break;
  }
  case 'o': {
    UnsignedType n = abs_value;
    do {
      ++size;
    } while ((n >>= 3) != 0);
    unsigned width = std::max(spec.width, size);
    p = GrowBuffer(width) + width - 1;
    n = abs_value;
    do {
      *p-- = '0' + (n & 7);
    } while ((n >>= 3) != 0);
    break;
  }
  default:
    ReportUnknownType(spec.type, "integer");
    break;
  }
  if (sign) {
    if ((spec.flags & ZERO_FLAG) != 0)
      buffer_[start++] = sign;
    else
      *p-- = sign;
  }
  std::fill(&buffer_[start], p + 1, spec.fill);
}

template <typename T>
void Formatter::FormatDouble(T value, const FormatSpec &spec, int precision) {
  // Check type.
  char type = spec.type;
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
  unsigned width = spec.width;
  *format_ptr++ = '%';
  if ((spec.flags & PLUS_FLAG) != 0)
    *format_ptr++ = '+';
  if ((spec.flags & ZERO_FLAG) != 0)
    *format_ptr++ = '0';
  if (width != 0)
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
    char *start = &buffer_[offset];
    if (width == 0) {
      n = precision < 0 ?
          snprintf(start, size, format, value) :
          snprintf(start, size, format, precision, value);
    } else {
      n = precision < 0 ?
          snprintf(start, size, format, width, value) :
          snprintf(start, size, format, width, precision, value);
    }
    if (n >= 0 && offset + n < buffer_.capacity()) {
      if (spec.fill != ' ') {
        while (*start == ' ')
          *start++ = spec.fill;
      }
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

    FormatSpec spec;
    int precision = -1;
    if (*s == ':') {
      ++s;

      // Parse fill and alignment.
      if (char c = *s) {
        const char *p = s + 1;
        Alignment align = ALIGN_DEFAULT;
        do {
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
            if (p != s && c != '{' && c != '}') {
              s += 2;
              spec.fill = c;
            } else ++s;
            break;
          }
        } while (--p >= s);
      }

      // Parse sign.
      if (*s == '+') {
        ++s;
        if (arg.type > LAST_NUMERIC_TYPE)
          ReportError(s, "format specifier '+' requires numeric argument");
        if (arg.type == UINT || arg.type == ULONG) {
          ReportError(s,
              "format specifier '+' requires signed argument");
        }
        spec.flags |= PLUS_FLAG;
      }

      // Parse width and zero flag.
      if ('0' <= *s && *s <= '9') {
        if (*s == '0') {
          if (arg.type > LAST_NUMERIC_TYPE)
            ReportError(s, "format specifier '0' requires numeric argument");
          spec.flags |= ZERO_FLAG;
          spec.fill = '0';
        }
        // Zero may be parsed again as a part of the width, but it is simpler
        // and more efficient than checking if the next char is a digit.
        unsigned value = ParseUInt(s);
        if (value > INT_MAX)
          ReportError(s, "number is too big in format");
        spec.width = value;
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
        spec.type = *s++;
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case INT:
      FormatInt(arg.int_value, spec);
      break;
    case UINT:
      FormatInt(arg.uint_value, spec);
      break;
    case LONG:
      FormatInt(arg.long_value, spec);
      break;
    case ULONG:
      FormatInt(arg.ulong_value, spec);
      break;
    case DOUBLE:
      FormatDouble(arg.double_value, spec, precision);
      break;
    case LONG_DOUBLE:
      FormatDouble(arg.long_double_value, spec, precision);
      break;
    case CHAR: {
      if (spec.type && spec.type != 'c')
        ReportUnknownType(spec.type, "char");
      char *out = GrowBuffer(std::max(spec.width, 1u));
      *out++ = arg.int_value;
      if (spec.width > 1)
        std::fill_n(out, spec.width - 1, spec.fill);
      break;
    }
    case STRING: {
      if (spec.type && spec.type != 's')
        ReportUnknownType(spec.type, "string");
      const char *str = arg.string.value;
      size_t size = arg.string.size;
      if (size == 0) {
        if (!str)
          throw FormatError("string pointer is null");
        if (*str)
          size = std::strlen(str);
      }
      char *out = GrowBuffer(std::max<size_t>(spec.width, size));
      out = std::copy(str, str + size, out);
      if (spec.width > size)
        std::fill_n(out, spec.width - size, spec.fill);
      break;
    }
    case POINTER:
      if (spec.type && spec.type != 'p')
        ReportUnknownType(spec.type, "pointer");
      spec.flags = HEX_PREFIX_FLAG;
      spec.type = 'x';
      FormatInt(reinterpret_cast<uintptr_t>(arg.pointer_value), spec);
      break;
    case CUSTOM:
      if (spec.type)
        ReportUnknownType(spec.type, "object");
      (this->*arg.custom.format)(arg.custom.value, spec);
      break;
    default:
      assert(false);
      break;
    }
  }
  buffer_.append(start, s + 1);
  buffer_.resize(buffer_.size() - 1);  // Don't count the terminating zero.
}

void Formatter::Write(const std::string &s, const FormatSpec &spec) {
  char *out = GrowBuffer(std::max<std::size_t>(spec.width, s.size()));
  std::copy(s.begin(), s.end(), out);
  if (spec.width > s.size())
    std::fill_n(out + s.size(), spec.width - s.size(), spec.fill);
}
