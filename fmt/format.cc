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

#include "fmt/format.h"
#include "fmt/printf.h"

#include <string.h>

#include <cctype>

#include <cmath>

#include <cstddef>  // for std::ptrdiff_t


namespace fmt {
namespace internal {

// This method is used to preserve binary compatibility with fmt 3.0.
// It can be removed in 4.0.
FMT_FUNC void format_system_error(
  Writer &out, int error_code, StringRef message) FMT_NOEXCEPT {
  fmt::format_system_error(out, error_code, message);
}

}  // namespace internal


}  // namespace fmt

FMT_FUNC void fmt::SystemError::init(
    int err_code, CStringRef format_str, ArgList args) {
  error_code_ = err_code;
  MemoryWriter w;
  format_system_error(w, err_code, format(format_str, args));
  std::runtime_error &base = *this;
  base = std::runtime_error(w.str());
}

template <typename T>
int fmt::internal::CharTraits<char>::format_float(
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
int fmt::internal::CharTraits<wchar_t>::format_float(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    unsigned width, int precision, T value) {
  if (width == 0) {
    return precision < 0 ?
        FMT_SWPRINTF(buffer, size, format, value) :
        FMT_SWPRINTF(buffer, size, format, precision, value);
  }
  return precision < 0 ?
      FMT_SWPRINTF(buffer, size, format, width, value) :
      FMT_SWPRINTF(buffer, size, format, width, precision, value);
}

template <typename T>
const char fmt::internal::BasicData<T>::DIGITS[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";


FMT_FUNC void fmt::internal::report_unknown_type(char code, const char *type) {
  (void)type;
  if (std::isprint(static_cast<unsigned char>(code))) {
    FMT_THROW(fmt::FormatError(
        fmt::format("unknown format code '{}' for {}", code, type)));
  }
  FMT_THROW(fmt::FormatError(
      fmt::format("unknown format code '\\x{:02x}' for {}",
        static_cast<unsigned>(code), type)));
}

#if FMT_USE_WINDOWS_H

FMT_FUNC fmt::internal::UTF8ToUTF16::UTF8ToUTF16(fmt::StringRef s) {
  static const char ERROR_MSG[] = "cannot convert string from UTF-8 to UTF-16";
  if (s.size() > INT_MAX)
    FMT_THROW(WindowsError(ERROR_INVALID_PARAMETER, ERROR_MSG));
  int s_size = static_cast<int>(s.size());
  int length = MultiByteToWideChar(
      CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), s_size, 0, 0);
  if (length == 0)
    FMT_THROW(WindowsError(GetLastError(), ERROR_MSG));
  buffer_.resize(length + 1);
  length = MultiByteToWideChar(
    CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), s_size, &buffer_[0], length);
  if (length == 0)
    FMT_THROW(WindowsError(GetLastError(), ERROR_MSG));
  buffer_[length] = 0;
}

FMT_FUNC fmt::internal::UTF16ToUTF8::UTF16ToUTF8(fmt::WStringRef s) {
  if (int error_code = convert(s)) {
    FMT_THROW(WindowsError(error_code,
        "cannot convert string from UTF-16 to UTF-8"));
  }
}

FMT_FUNC int fmt::internal::UTF16ToUTF8::convert(fmt::WStringRef s) {
  if (s.size() > INT_MAX)
    return ERROR_INVALID_PARAMETER;
  int s_size = static_cast<int>(s.size());
  int length = WideCharToMultiByte(CP_UTF8, 0, s.data(), s_size, 0, 0, 0, 0);
  if (length == 0)
    return GetLastError();
  buffer_.resize(length + 1);
  length = WideCharToMultiByte(
    CP_UTF8, 0, s.data(), s_size, &buffer_[0], length, 0, 0);
  if (length == 0)
    return GetLastError();
  buffer_[length] = 0;
  return 0;
}

FMT_FUNC void fmt::WindowsError::init(
    int err_code, CStringRef format_str, ArgList args) {
  error_code_ = err_code;
  MemoryWriter w;
  internal::format_windows_error(w, err_code, format(format_str, args));
  std::runtime_error &base = *this;
  base = std::runtime_error(w.str());
}

FMT_FUNC void fmt::internal::format_windows_error(
    fmt::Writer &out, int error_code,
    fmt::StringRef message) FMT_NOEXCEPT {
  FMT_TRY {
    MemoryBuffer<wchar_t, INLINE_BUFFER_SIZE> buffer;
    buffer.resize(INLINE_BUFFER_SIZE);
    for (;;) {
      wchar_t *system_message = &buffer[0];
      int result = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  0, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  system_message, static_cast<uint32_t>(buffer.size()), 0);
      if (result != 0) {
        UTF16ToUTF8 utf8_message;
        if (utf8_message.convert(system_message) == ERROR_SUCCESS) {
          out << message << ": " << utf8_message;
          return;
        }
        break;
      }
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        break;  // Can't get error message, report error code instead.
      buffer.resize(buffer.size() * 2);
    }
  } FMT_CATCH(...) {}
  fmt::format_error_code(out, error_code, message);  // 'fmt::' is for bcc32.
}

#endif  // FMT_USE_WINDOWS_H

FMT_FUNC void fmt::format_system_error(
    fmt::Writer &out, int error_code,
    fmt::StringRef message) FMT_NOEXCEPT {
  FMT_TRY {
    internal::MemoryBuffer<char, internal::INLINE_BUFFER_SIZE> buffer;
    buffer.resize(internal::INLINE_BUFFER_SIZE);
    for (;;) {
      char *system_message = &buffer[0];
      int result = safe_strerror(error_code, system_message, buffer.size());
      if (result == 0) {
        out << message << ": " << system_message;
        return;
      }
      if (result != ERANGE)
        break;  // Can't get error message, report error code instead.
      buffer.resize(buffer.size() * 2);
    }
  } FMT_CATCH(...) {}
  fmt::format_error_code(out, error_code, message);  // 'fmt::' is for bcc32.
}

template <typename Char>
void fmt::internal::ArgMap<Char>::init(const ArgList &args) {
  if (!map_.empty())
    return;
  typedef internal::NamedArg<Char> NamedArg;
  const NamedArg *named_arg = 0;
  bool use_values =
      args.type(ArgList::MAX_PACKED_ARGS - 1) == internal::Arg::NONE;
  if (use_values) {
    for (unsigned i = 0;/*nothing*/; ++i) {
      internal::Arg::Type arg_type = args.type(i);
      switch (arg_type) {
      case internal::Arg::NONE:
        return;
      case internal::Arg::NAMED_ARG:
        named_arg = static_cast<const NamedArg*>(args.values_[i].pointer);
        map_.push_back(Pair(named_arg->name, *named_arg));
        break;
      default:
        /*nothing*/;
      }
    }
    return;
  }
  for (unsigned i = 0; i != ArgList::MAX_PACKED_ARGS; ++i) {
    internal::Arg::Type arg_type = args.type(i);
    if (arg_type == internal::Arg::NAMED_ARG) {
      named_arg = static_cast<const NamedArg*>(args.args_[i].pointer);
      map_.push_back(Pair(named_arg->name, *named_arg));
    }
  }
  for (unsigned i = ArgList::MAX_PACKED_ARGS;/*nothing*/; ++i) {
    switch (args.args_[i].type) {
    case internal::Arg::NONE:
      return;
    case internal::Arg::NAMED_ARG:
      named_arg = static_cast<const NamedArg*>(args.args_[i].pointer);
      map_.push_back(Pair(named_arg->name, *named_arg));
      break;
    default:
      /*nothing*/;
    }
  }
}

template <typename Char>
void fmt::internal::FixedBuffer<Char>::grow(std::size_t) {
  FMT_THROW(std::runtime_error("buffer overflow"));
}

FMT_FUNC Arg fmt::internal::FormatterBase::do_get_arg(
    unsigned arg_index, const char *&error) {
  Arg arg = args_[arg_index];
  switch (arg.type) {
  case Arg::NONE:
    error = "argument index out of range";
    break;
  case Arg::NAMED_ARG:
    arg = *static_cast<const internal::Arg*>(arg.pointer);
    break;
  default:
    /*nothing*/;
  }
  return arg;
}



FMT_FUNC void fmt::report_system_error(
    int error_code, fmt::StringRef message) FMT_NOEXCEPT {
  // 'fmt::' is for bcc32.
  fmt::report_error(format_system_error, error_code, message);
}

#if FMT_USE_WINDOWS_H
FMT_FUNC void fmt::report_windows_error(
    int error_code, fmt::StringRef message) FMT_NOEXCEPT {
  // 'fmt::' is for bcc32.
  fmt::report_error(internal::format_windows_error, error_code, message);
}
#endif

FMT_FUNC void fmt::print(std::FILE *f, CStringRef format_str, ArgList args) {
  MemoryWriter w;
  w.write(format_str, args);
  std::fwrite(w.data(), 1, w.size(), f);
}

FMT_FUNC void fmt::print(CStringRef format_str, ArgList args) {
  print(stdout, format_str, args);
}

FMT_FUNC void fmt::print_colored(Color c, CStringRef format, ArgList args) {
  char escape[] = "\x1b[30m";
  escape[3] = static_cast<char>('0' + c);
  std::fputs(escape, stdout);
  print(format, args);
  std::fputs(RESET_COLOR, stdout);
}


#ifndef FMT_HEADER_ONLY

template struct fmt::internal::BasicData<void>;

// Explicit instantiations for char.

template void fmt::internal::FixedBuffer<char>::grow(std::size_t);

template void fmt::internal::ArgMap<char>::init(const fmt::ArgList &args);

template int fmt::internal::CharTraits<char>::format_float(
    char *buffer, std::size_t size, const char *format,
    unsigned width, int precision, double value);

template int fmt::internal::CharTraits<char>::format_float(
    char *buffer, std::size_t size, const char *format,
    unsigned width, int precision, long double value);

// Explicit instantiations for wchar_t.

template void fmt::internal::FixedBuffer<wchar_t>::grow(std::size_t);

template void fmt::internal::ArgMap<wchar_t>::init(const fmt::ArgList &args);

template int fmt::internal::CharTraits<wchar_t>::format_float(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    unsigned width, int precision, double value);

template int fmt::internal::CharTraits<wchar_t>::format_float(
    wchar_t *buffer, std::size_t size, const wchar_t *format,
    unsigned width, int precision, long double value);

#endif  // FMT_HEADER_ONLY

#ifdef _MSC_VER
# pragma warning(pop)
#endif
