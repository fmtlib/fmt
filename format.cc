/*
 Small, safe and fast printf-like formatting library for C++
 Author: Victor Zverovich
 */

#include "format.h"

#include <cassert>
#include <climits>
#include <cstring>
#include <algorithm>

using std::size_t;

namespace {

// Throws Exception(message) if format contains '}', otherwise throws
// FormatError reporting unmatched '{'. The idea is that unmatched '{'
// should override other errors.
template <typename Exception>
void Throw(const char *s, const char *message) {
  while (*s && *s != '}')
    ++s;
  if (!*s)
    throw fmt::FormatError("unmatched '{' in format");
  throw Exception(message);
}

// Parses an unsigned integer advancing s to the end of the parsed input.
// This function assumes that the first character of s is a digit.
unsigned ParseUInt(const char *&s) {
  assert('0' <= *s && *s <= '9');
  unsigned value = 0;
  do {
    unsigned new_value = value * 10 + (*s++ - '0');
    if (new_value < value)  // Check if value wrapped around.
      Throw<fmt::FormatError>(s, "number is too big in format");
    value = new_value;
  } while ('0' <= *s && *s <= '9');
  return value;
}

// Flags.
enum {
  PLUS_FLAG = 1,
  ZERO_FLAG = 2
};

void CheckFlags(unsigned flags) {
  if (flags == 0) return;
  if ((flags & PLUS_FLAG) != 0)
    throw fmt::FormatError("format specifier '+' used with non-numeric type");
  if ((flags & ZERO_FLAG) != 0)
    throw fmt::FormatError("format specifier '0' used with non-numeric type");
}
}

template <typename T>
void fmt::Formatter::FormatBuiltinArg(
    const char *format, const T &arg, int width, int precision) {
  size_t offset = buffer_.size();
  buffer_.resize(buffer_.capacity());
  for (;;) {
    size_t size = buffer_.size() - offset;
    int n = 0;
    if (width < 0) {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, arg) :
          snprintf(&buffer_[offset], size, format, precision, arg);
    } else {
      n = precision < 0 ?
          snprintf(&buffer_[offset], size, format, width, arg) :
          snprintf(&buffer_[offset], size, format, width, precision, arg);
    }
    if (n >= 0 && offset + n < buffer_.size()) {
      buffer_.resize(offset + n);
      return;
    }
    buffer_.resize(n >= 0 ? offset + n + 1 : 2 * buffer_.size());
  }
}

void fmt::Formatter::Format() {
  buffer_.reserve(500);
  const char *start = format_;
  const char *s = start;
  while (*s) {
    if (*s++ != '{') continue;
    buffer_.insert(buffer_.end(), start, s - 1);

    // Parse argument index.
    if (*s < '0' || *s > '9')
      Throw<fmt::FormatError>(s, "missing argument index in format string");
    unsigned arg_index = ParseUInt(s);
    if (arg_index >= args_.size())
      Throw<std::out_of_range>(s, "argument index is out of range in format");

    enum { MAX_FORMAT_SIZE = 9}; // longest format: %+0*.*ld
    char arg_format[MAX_FORMAT_SIZE];
    char *arg_format_ptr = arg_format;
    *arg_format_ptr++ = '%';

    unsigned flags = 0;
    int width = -1;
    int precision = -1;
    char type = 0;
    if (*s == ':') {
      ++s;
      if (*s == '+') {
        flags |= PLUS_FLAG;
        *arg_format_ptr++ = *s++;
      }
      if (*s == '0') {
        flags |= ZERO_FLAG;
        *arg_format_ptr++ = *s++;
      }

      // Parse width.
      if ('0' <= *s && *s <= '9') {
        *arg_format_ptr++ = '*';
        unsigned number = ParseUInt(s);
        if (number > INT_MAX) ; // TODO: error
        width = number;
      }

      // Parse precision.
      if (*s == '.') {
        *arg_format_ptr++ = '.';
        *arg_format_ptr++ = '*';
        ++s;
        precision = 0;
        if ('0' <= *s && *s <= '9') {
          do {
            // TODO: check overflow
            precision = precision * 10 + (*s++ - '0');
          } while ('0' <= *s && *s <= '9');
        } else {
          // TODO: error
        }
      }

      // Parse type.
      if (*s == 'f' || *s == 'g')
        type = *s++; // TODO: check if the type matches
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    Arg &arg = args_[arg_index];
    switch (arg.type) {
    case CHAR:
      CheckFlags(flags);
      if (width == -1 && precision == -1) {
        buffer_.push_back(arg.int_value);
        break;
      }
      *arg_format_ptr++ = 'c';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.int_value, width, precision);
      break;
    case INT:
      *arg_format_ptr++ = 'd';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.int_value, width, precision);
      break;
    case UINT:
      *arg_format_ptr++ = 'd';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.uint_value, width, precision);
      break;
    case LONG:
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = 'd';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.long_value, width, precision);
      break;
    case ULONG:
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = 'd';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.ulong_value, width, precision);
      break;
    case DOUBLE:
      *arg_format_ptr++ = type ? type : 'g';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.double_value, width, precision);
      break;
    case LONG_DOUBLE:
      *arg_format_ptr++ = 'L';
      *arg_format_ptr++ = 'g';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.long_double_value, width, precision);
      break;
    case STRING:
      CheckFlags(flags);
      if (width == -1 && precision == -1) {
        const char *str = arg.string_value;
        std::size_t size = arg.size;
        if (size == 0 && *str)
          size = std::strlen(str);
        buffer_.insert(buffer_.end(), str, str + size);
        break;
      }
      *arg_format_ptr++ = 's';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.string_value, width, precision);
      break;
    case WSTRING:
      CheckFlags(flags);
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = 's';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.wstring_value, width, precision);
      break;
    case POINTER:
      CheckFlags(flags);
      *arg_format_ptr++ = 'p';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.pointer_value, width, precision);
      break;
    case CUSTOM:
      CheckFlags(flags);
      (this->*arg.format)(arg.custom_value);
      break;
    default:
      assert(false);
      break;
    }
  }
  buffer_.insert(buffer_.end(), start, s + 1);
}

fmt::ArgFormatter::~ArgFormatter() {
  if (!formatter_) return;
  FinishFormatting();
}
