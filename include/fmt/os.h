// Formatting library for C++ - optional OS-specific functionality
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OS_H_
#define FMT_OS_H_

#include <cerrno>
#include <clocale>  // locale_t
#include <cstddef>
#include <cstdio>
#include <cstdlib>       // strtod_l
#include <system_error>  // std::system_error

#if defined __APPLE__ || defined(__FreeBSD__)
#  include <xlocale.h>  // for LC_NUMERIC_MASK on OS X
#endif

#include "format.h"

#ifndef FMT_USE_FCNTL
// UWP doesn't provide _pipe.
#  if FMT_HAS_INCLUDE("winapifamily.h")
#    include <winapifamily.h>
#  endif
#  if (FMT_HAS_INCLUDE(<fcntl.h>) || defined(__APPLE__) || \
       defined(__linux__)) &&                              \
      (!defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
#  include <fcntl.h>  // for O_RDONLY
#    define FMT_USE_FCNTL 1
#  else
#    define FMT_USE_FCNTL 0
#  endif
#endif

#ifndef FMT_POSIX
#  if defined(_WIN32) && !defined(__MINGW32__)
// Fix warnings about deprecated symbols.
#    define FMT_POSIX(call) _##call
#  else
#    define FMT_POSIX(call) call
#  endif
#endif

// Calls to system functions are wrapped in FMT_SYSTEM for testability.
#ifdef FMT_SYSTEM
#  define FMT_POSIX_CALL(call) FMT_SYSTEM(call)
#else
#  define FMT_SYSTEM(call) ::call
#  ifdef _WIN32
// Fix warnings about deprecated symbols.
#    define FMT_POSIX_CALL(call) ::_##call
#  else
#    define FMT_POSIX_CALL(call) ::call
#  endif
#endif

// Retries the expression while it evaluates to error_result and errno
// equals to EINTR.
#ifndef _WIN32
#  define FMT_RETRY_VAL(result, expression, error_result) \
    do {                                                  \
      (result) = (expression);                            \
    } while ((result) == (error_result) && errno == EINTR)
#else
#  define FMT_RETRY_VAL(result, expression, error_result) result = (expression)
#endif

#define FMT_RETRY(result, expression) FMT_RETRY_VAL(result, expression, -1)

FMT_BEGIN_NAMESPACE
FMT_MODULE_EXPORT_BEGIN

/**
  \rst
  A reference to a null-terminated string. It can be constructed from a C
  string or ``std::string``.

  You can use one of the following type aliases for common character types:

  +---------------+-----------------------------+
  | Type          | Definition                  |
  +===============+=============================+
  | cstring_view  | basic_cstring_view<char>    |
  +---------------+-----------------------------+
  | wcstring_view | basic_cstring_view<wchar_t> |
  +---------------+-----------------------------+

  This class is most useful as a parameter type to allow passing
  different types of strings to a function, for example::

    template <typename... Args>
    std::string format(cstring_view format_str, const Args & ... args);

    format("{}", 42);
    format(std::string("{}"), 42);
  \endrst
 */
template <typename Char> class basic_cstring_view {
 private:
  const Char* data_;

 public:
  /** Constructs a string reference object from a C string. */
  basic_cstring_view(const Char* s) : data_(s) {}

  /**
    \rst
    Constructs a string reference from an ``std::string`` object.
    \endrst
   */
  basic_cstring_view(const std::basic_string<Char>& s) : data_(s.c_str()) {}

  /** Returns the pointer to a C string. */
  const Char* c_str() const { return data_; }
};

using cstring_view = basic_cstring_view<char>;
using wcstring_view = basic_cstring_view<wchar_t>;

template <typename Char> struct formatter<std::error_code, Char> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  FMT_CONSTEXPR auto format(const std::error_code& ec, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    out = detail::write_bytes(out, ec.category().name(),
                              basic_format_specs<Char>());
    out = detail::write<Char>(out, Char(':'));
    out = detail::write<Char>(out, ec.value());
    return out;
  }
};

#ifdef _WIN32
FMT_API const std::error_category& system_category() FMT_NOEXCEPT;

FMT_BEGIN_DETAIL_NAMESPACE
// A converter from UTF-16 to UTF-8.
// It is only provided for Windows since other systems support UTF-8 natively.
class utf16_to_utf8 {
 private:
  memory_buffer buffer_;

 public:
  utf16_to_utf8() {}
  FMT_API explicit utf16_to_utf8(basic_string_view<wchar_t> s);
  operator string_view() const { return string_view(&buffer_[0], size()); }
  size_t size() const { return buffer_.size() - 1; }
  const char* c_str() const { return &buffer_[0]; }
  std::string str() const { return std::string(&buffer_[0], size()); }

  // Performs conversion returning a system error code instead of
  // throwing exception on conversion error. This method may still throw
  // in case of memory allocation error.
  FMT_API int convert(basic_string_view<wchar_t> s);
};

FMT_API void format_windows_error(buffer<char>& out, int error_code,
                                  const char* message) FMT_NOEXCEPT;
FMT_END_DETAIL_NAMESPACE

FMT_API std::system_error vwindows_error(int error_code, string_view format_str,
                                         format_args args);

/**
 \rst
 Constructs a :class:`std::system_error` object with the description
 of the form

 .. parsed-literal::
   *<message>*: *<system-message>*

 where *<message>* is the formatted message and *<system-message>* is the
 system message corresponding to the error code.
 *error_code* is a Windows error code as given by ``GetLastError``.
 If *error_code* is not a valid error code such as -1, the system message
 will look like "error -1".

 **Example**::

   // This throws a system_error with the description
   //   cannot open file 'madeup': The system cannot find the file specified.
   // or similar (system message may vary).
   const char *filename = "madeup";
   LPOFSTRUCT of = LPOFSTRUCT();
   HFILE file = OpenFile(filename, &of, OF_READ);
   if (file == HFILE_ERROR) {
     throw fmt::windows_error(GetLastError(),
                              "cannot open file '{}'", filename);
   }
 \endrst
*/
template <typename... Args>
std::system_error windows_error(int error_code, string_view message,
                                const Args&... args) {
  return vwindows_error(error_code, message, fmt::make_format_args(args...));
}

// Reports a Windows error without throwing an exception.
// Can be used to report errors from destructors.
FMT_API void report_windows_error(int error_code,
                                  const char* message) FMT_NOEXCEPT;
#else
inline const std::error_category& system_category() FMT_NOEXCEPT {
  return std::system_category();
}
#endif  // _WIN32

// std::system is not available on some platforms such as iOS (#2248).
#ifdef __OSX__
template <typename S, typename... Args, typename Char = char_t<S>>
void say(const S& format_str, Args&&... args) {
  std::system(format("say \"{}\"", format(format_str, args...)).c_str());
}
#endif

// A buffered file.
class buffered_file {
 private:
  FILE* file_;

  friend class file;

  explicit buffered_file(FILE* f) : file_(f) {}

 public:
  buffered_file(const buffered_file&) = delete;
  void operator=(const buffered_file&) = delete;

  // Constructs a buffered_file object which doesn't represent any file.
  buffered_file() FMT_NOEXCEPT : file_(nullptr) {}

  // Destroys the object closing the file it represents if any.
  FMT_API ~buffered_file() FMT_NOEXCEPT;

 public:
  buffered_file(buffered_file&& other) FMT_NOEXCEPT : file_(other.file_) {
    other.file_ = nullptr;
  }

  buffered_file& operator=(buffered_file&& other) {
    close();
    file_ = other.file_;
    other.file_ = nullptr;
    return *this;
  }

  // Opens a file.
  FMT_API buffered_file(cstring_view filename, cstring_view mode);

  // Closes the file.
  FMT_API void close();

  // Returns the pointer to a FILE object representing this file.
  FILE* get() const FMT_NOEXCEPT { return file_; }

  // We place parentheses around fileno to workaround a bug in some versions
  // of MinGW that define fileno as a macro.
  FMT_API int(fileno)() const;

  void vprint(string_view format_str, format_args args) {
    fmt::vprint(file_, format_str, args);
  }

  template <typename... Args>
  inline void print(string_view format_str, const Args&... args) {
    vprint(format_str, fmt::make_format_args(args...));
  }
};

#if FMT_USE_FCNTL
// A file. Closed file is represented by a file object with descriptor -1.
// Methods that are not declared with FMT_NOEXCEPT may throw
// fmt::system_error in case of failure. Note that some errors such as
// closing the file multiple times will cause a crash on Windows rather
// than an exception. You can get standard behavior by overriding the
// invalid parameter handler with _set_invalid_parameter_handler.
class file {
 private:
  int fd_;  // File descriptor.

  // Constructs a file object with a given descriptor.
  explicit file(int fd) : fd_(fd) {}

 public:
  // Possible values for the oflag argument to the constructor.
  enum {
    RDONLY = FMT_POSIX(O_RDONLY),  // Open for reading only.
    WRONLY = FMT_POSIX(O_WRONLY),  // Open for writing only.
    RDWR = FMT_POSIX(O_RDWR),      // Open for reading and writing.
    CREATE = FMT_POSIX(O_CREAT),   // Create if the file doesn't exist.
    APPEND = FMT_POSIX(O_APPEND),  // Open in append mode.
    TRUNC = FMT_POSIX(O_TRUNC)     // Truncate the content of the file.
  };

  // Constructs a file object which doesn't represent any file.
  file() FMT_NOEXCEPT : fd_(-1) {}

  // Opens a file and constructs a file object representing this file.
  FMT_API file(cstring_view path, int oflag);

 public:
  file(const file&) = delete;
  void operator=(const file&) = delete;

  file(file&& other) FMT_NOEXCEPT : fd_(other.fd_) { other.fd_ = -1; }

  // Move assignment is not noexcept because close may throw.
  file& operator=(file&& other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  // Destroys the object closing the file it represents if any.
  FMT_API ~file() FMT_NOEXCEPT;

  // Returns the file descriptor.
  int descriptor() const FMT_NOEXCEPT { return fd_; }

  // Closes the file.
  FMT_API void close();

  // Returns the file size. The size has signed type for consistency with
  // stat::st_size.
  FMT_API long long size() const;

  // Attempts to read count bytes from the file into the specified buffer.
  FMT_API size_t read(void* buffer, size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  FMT_API size_t write(const void* buffer, size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object.
  FMT_API static file dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  FMT_API void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  FMT_API void dup2(int fd, std::error_code& ec) FMT_NOEXCEPT;

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively.
  FMT_API static void pipe(file& read_end, file& write_end);

  // Creates a buffered_file object associated with this file and detaches
  // this file object from the file.
  FMT_API buffered_file fdopen(const char* mode);
};

// Returns the memory page size.
long getpagesize();

FMT_BEGIN_DETAIL_NAMESPACE

struct buffer_size {
  buffer_size() = default;
  size_t value = 0;
  buffer_size operator=(size_t val) const {
    auto bs = buffer_size();
    bs.value = val;
    return bs;
  }
};

struct ostream_params {
  int oflag = file::WRONLY | file::CREATE | file::TRUNC;
  size_t buffer_size = BUFSIZ > 32768 ? BUFSIZ : 32768;

  ostream_params() {}

  template <typename... T>
  ostream_params(T... params, int new_oflag) : ostream_params(params...) {
    oflag = new_oflag;
  }

  template <typename... T>
  ostream_params(T... params, detail::buffer_size bs)
      : ostream_params(params...) {
    this->buffer_size = bs.value;
  }

// Intel has a bug that results in failure to deduce a constructor
// for empty parameter packs.
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 2000
  ostream_params(int new_oflag) : oflag(new_oflag) {}
  ostream_params(detail::buffer_size bs) : buffer_size(bs.value) {}
#endif
};

FMT_END_DETAIL_NAMESPACE

// Added {} below to work around default constructor error known to
// occur in Xcode versions 7.2.1 and 8.2.1.
constexpr detail::buffer_size buffer_size{};

/** A fast output stream which is not thread-safe. */
class FMT_API ostream final : private detail::buffer<char> {
 private:
  file file_;

  void grow(size_t) override;

  ostream(cstring_view path, const detail::ostream_params& params)
      : file_(path, params.oflag) {
    set(new char[params.buffer_size], params.buffer_size);
  }

 public:
  ostream(ostream&& other)
      : detail::buffer<char>(other.data(), other.size(), other.capacity()),
        file_(std::move(other.file_)) {
    other.clear();
    other.set(nullptr, 0);
  }
  ~ostream() {
    flush();
    delete[] data();
  }

  void flush() {
    if (size() == 0) return;
    file_.write(data(), size());
    clear();
  }

  template <typename... T>
  friend ostream output_file(cstring_view path, T... params);

  void close() {
    flush();
    file_.close();
  }

  /**
    Formats ``args`` according to specifications in ``fmt`` and writes the
    output to the file.
   */
  template <typename... T> void print(format_string<T...> fmt, T&&... args) {
    vformat_to(detail::buffer_appender<char>(*this), fmt,
               fmt::make_format_args(args...));
  }
};

/**
  \rst
  Opens a file for writing. Supported parameters passed in *params*:

  * ``<integer>``: Flags passed to `open
    <https://pubs.opengroup.org/onlinepubs/007904875/functions/open.html>`_
    (``file::WRONLY | file::CREATE`` by default)
  * ``buffer_size=<integer>``: Output buffer size

  **Example**::

    auto out = fmt::output_file("guide.txt");
    out.print("Don't {}", "Panic");
  \endrst
 */
template <typename... T>
inline ostream output_file(cstring_view path, T... params) {
  return {path, detail::ostream_params(params...)};
}
#endif  // FMT_USE_FCNTL

#ifdef FMT_LOCALE
// A "C" numeric locale.
class locale {
 private:
#  ifdef _WIN32
  using locale_t = _locale_t;

  static void freelocale(locale_t loc) { _free_locale(loc); }

  static double strtod_l(const char* nptr, char** endptr, _locale_t loc) {
    return _strtod_l(nptr, endptr, loc);
  }
#  endif

  locale_t locale_;

 public:
  using type = locale_t;
  locale(const locale&) = delete;
  void operator=(const locale&) = delete;

  locale() {
#  ifndef _WIN32
    locale_ = FMT_SYSTEM(newlocale(LC_NUMERIC_MASK, "C", nullptr));
#  else
    locale_ = _create_locale(LC_NUMERIC, "C");
#  endif
    if (!locale_) FMT_THROW(system_error(errno, "cannot create locale"));
  }
  ~locale() { freelocale(locale_); }

  type get() const { return locale_; }

  // Converts string to floating-point number and advances str past the end
  // of the parsed input.
  FMT_DEPRECATED double strtod(const char*& str) const {
    char* end = nullptr;
    double result = strtod_l(str, &end, locale_);
    str = end;
    return result;
  }
};
using Locale FMT_DEPRECATED_ALIAS = locale;
#endif  // FMT_LOCALE
FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_OS_H_
