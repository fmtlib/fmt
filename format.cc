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
    Arg &arg = args_[arg_index];

    enum { MAX_FORMAT_SIZE = 10}; // longest format: %+0-*.*ld
    char arg_format[MAX_FORMAT_SIZE];
    char *arg_format_ptr = arg_format;
    *arg_format_ptr++ = '%';

    int width = -1;
    int precision = -1;
    char type = 0;
    if (*s == ':') {
      ++s;
      if (*s == '+') {
        if (arg.type > LAST_NUMERIC_TYPE) {
          Throw<FormatError>(s,
              "format specifier '+' used with non-numeric type");
        }
        *arg_format_ptr++ = *s++;
      }
      if (*s == '0') {
        if (arg.type > LAST_NUMERIC_TYPE) {
          Throw<FormatError>(s,
              "format specifier '0' used with non-numeric type");
        }
        *arg_format_ptr++ = *s++;
      }

      // Parse width.
      if ('0' <= *s && *s <= '9') {
        if (arg.type > LAST_NUMERIC_TYPE)
          *arg_format_ptr++ = '-';
        *arg_format_ptr++ = '*';
        unsigned value = ParseUInt(s);
        if (value > INT_MAX)
          Throw<FormatError>(s, "number is too big in format");
        width = value;
      }

      // Parse precision.
      if (*s == '.') {
        *arg_format_ptr++ = '.';
        *arg_format_ptr++ = '*';
        ++s;
        precision = 0;
        if ('0' <= *s && *s <= '9') {
          unsigned value = ParseUInt(s);
          if (value > INT_MAX)
            Throw<FormatError>(s, "number is too big in format"); // TODO: test
          precision = value;
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
    switch (arg.type) {
    case CHAR:
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
      // TODO: align string left by default
      if (width == -1 && precision == -1) {
        const char *str = arg.string_value;
        std::size_t size = arg.size;
        if (size == 0 && *str)
          size = std::strlen(str);
        buffer_.reserve(buffer_.size() + size + 1);
        buffer_.insert(buffer_.end(), str, str + size);
        break;
      }
      *arg_format_ptr++ = 's';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.string_value, width, precision);
      break;
    case WSTRING:
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = 's';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.wstring_value, width, precision);
      break;
    case POINTER:
      *arg_format_ptr++ = 'p';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.pointer_value, width, precision);
      break;
    case CUSTOM:
      (this->*arg.format)(arg.custom_value, width);
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
