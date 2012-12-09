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
    // TODO: handle escape sequence
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
    bool is_floating_point = false;
    if (*s == ':') {
      ++s;
      if (*s == '+') {
        if (arg.type > LAST_NUMERIC_TYPE) {
          Throw<FormatError>(s,
              "format specifier '+' requires numeric argument");
        }
        if (arg.type == UINT || arg.type == ULONG) {
          Throw<FormatError>(s,
              "format specifier '+' requires signed argument");
        }
        *arg_format_ptr++ = *s++;
      }
      if (*s == '0') {
        if (arg.type > LAST_NUMERIC_TYPE) {
          Throw<FormatError>(s,
              "format specifier '0' requires numeric argument");
        }
        *arg_format_ptr++ = *s++;
      }

      // Parse width.
      if ('0' <= *s && *s <= '9') {
        if (arg.type > LAST_NUMERIC_TYPE && arg.type != POINTER)
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
            Throw<FormatError>(s, "number is too big in format");
          precision = value;
        } else {
          Throw<FormatError>(s, "missing precision in format");
        }
        if (arg.type > LAST_NUMERIC_TYPE ||
            (*s == '}' && arg.type != DOUBLE && arg.type != LONG_DOUBLE)) {
          Throw<FormatError>(s,
              "precision specifier requires floating-point type");
        }
      }

      // Parse type.
      if (*s != '}' && *s) {
        type = *s++;
        if (arg.type <= LAST_NUMERIC_TYPE) {
          switch (type) {
          case 'd': case 'o': case 'x': case 'X':
            // TODO: check that argument is integer
            break;
          case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
            is_floating_point = true;
            break;
          default:
            // TODO: error
            break;
          }
        }
        // TODO: check non-numeric types (string, character)
      }
    }

    if (*s++ != '}')
      throw FormatError("unmatched '{' in format");
    start = s;

    // Format argument.
    switch (arg.type) {
    case INT:
      *arg_format_ptr++ = type ? type : 'd';
      *arg_format_ptr = '\0';
      if (is_floating_point)
        FormatBuiltinArg<double>(arg_format, arg.int_value, width, precision);
      else
        FormatBuiltinArg(arg_format, arg.int_value, width, precision);
      break;
    case UINT:
      *arg_format_ptr++ = type ? type : 'u';
      *arg_format_ptr = '\0';
      if (is_floating_point)
        FormatBuiltinArg<double>(arg_format, arg.uint_value, width, precision);
      else
        FormatBuiltinArg(arg_format, arg.uint_value, width, precision);
      break;
    case LONG:
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = type ? type : 'd';
      *arg_format_ptr = '\0';
      if (is_floating_point)
        FormatBuiltinArg<double>(arg_format, arg.long_value, width, precision);
      else
        FormatBuiltinArg(arg_format, arg.long_value, width, precision);
      break;
    case ULONG:
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = type ? type : 'u';
      *arg_format_ptr = '\0';
      if (is_floating_point)
        FormatBuiltinArg<double>(arg_format, arg.ulong_value, width, precision);
      else
        FormatBuiltinArg(arg_format, arg.ulong_value, width, precision);
      break;
    case DOUBLE:
      *arg_format_ptr++ = type ? type : 'g';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.double_value, width, precision);
      break;
    case LONG_DOUBLE:
      *arg_format_ptr++ = 'L';
      *arg_format_ptr++ = type ? type : 'g';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.long_double_value, width, precision);
      break;
    case POINTER:
      // TODO: don't allow any type specifiers other than 'p'
      *arg_format_ptr++ = 'p';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.pointer_value, width, precision);
      break;
    case CHAR:
      // TODO: check if type is 'c' or none
      if (width <= 1) {
        buffer_.push_back(arg.int_value);
        break;
      }
      *arg_format_ptr++ = 'c';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.int_value, width, precision);
      break;
    case STRING:
      // TODO: check if type is 's' or none
      if (width == -1 || width <= arg.size) {
        const char *str = arg.string_value;
        size_t size = arg.size;
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
      // TODO: check if type is 's' or none
      *arg_format_ptr++ = 'l';
      *arg_format_ptr++ = 's';
      *arg_format_ptr = '\0';
      FormatBuiltinArg(arg_format, arg.wstring_value, width, precision);
      break;
    case CUSTOM:
      // TODO: check if type is 's' or none
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
