// Formatting library for C++ - time formatting
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TIME_H_
#define FMT_TIME_H_

#include "format.h"
#include <ctime>
#include <locale>

#if FMT_HAS_INCLUDE(<chrono>)
# include <chrono>
# include <sstream>
#endif

FMT_BEGIN_NAMESPACE

// Prevents expansion of a preceding token as a function-style macro.
// Usage: f FMT_NOMACRO()
#define FMT_NOMACRO

namespace internal{
inline null<> localtime_r FMT_NOMACRO(...) { return null<>(); }
inline null<> localtime_s(...) { return null<>(); }
inline null<> gmtime_r(...) { return null<>(); }
inline null<> gmtime_s(...) { return null<>(); }

enum class numeric_system {
  standard,
  alternative
};

// Parses a put_time-like format string and invokes handler actions.
template <typename Char, typename Handler>
FMT_CONSTEXPR const Char *parse_chrono_format(
    const Char *begin, const Char *end, Handler &&handler) {
  auto ptr = begin;
  while (ptr != end) {
    auto c = *ptr;
    if (c == '}') break;
    if (c != '%') {
      ++ptr;
      continue;
    }
    if (begin != ptr)
      handler.on_text(begin, ptr);
    ++ptr; // consume '%'
    if (ptr == end)
      throw format_error("invalid format");
    c = *ptr++;
    switch (c) {
    case '%':
      handler.on_text(ptr - 1, ptr);
      break;
    // Day of the week:
    case 'a':
      handler.on_abbr_weekday();
      break;
    case 'A':
      handler.on_full_weekday();
      break;
    case 'w':
      handler.on_dec0_weekday();
      break;
    case 'u':
      handler.on_dec1_weekday();
      break;
    // Month:
    case 'b':
      handler.on_abbr_month();
      break;
    case 'B':
      handler.on_full_month();
      break;
    // Hour, minute, second:
    case 'H':
      handler.on_24_hour(numeric_system::standard);
      break;
    case 'I':
      handler.on_12_hour(numeric_system::standard);
      break;
    case 'M':
      handler.on_minute(numeric_system::standard);
      break;
    case 'S':
      handler.on_second(numeric_system::standard);
      break;
    // Alternative numeric system:
    case 'O':
      if (ptr == end)
        throw format_error("invalid format");
      c = *ptr++;
      switch (c) {
      case 'H':
        handler.on_24_hour(numeric_system::alternative);
        break;
      case 'I':
        handler.on_12_hour(numeric_system::alternative);
        break;
      case 'M':
        handler.on_minute(numeric_system::alternative);
        break;
      case 'S':
        handler.on_second(numeric_system::alternative);
        break;
      }
      break;
      // TODO: parse more format specifiers
    }
    begin = ptr;
  }
  if (begin != ptr)
    handler.on_text(begin, ptr);
  return ptr;
}

struct chrono_format_checker {
  template <typename Char>
  void on_text(const Char *, const Char *) {}
  void on_abbr_weekday() {}
  void on_full_weekday() {}
  void on_dec0_weekday() {}
  void on_dec1_weekday() {}
  void on_abbr_month() {}
  void on_full_month() {}
  void on_24_hour(numeric_system) {}
  void on_12_hour(numeric_system) {}
  void on_minute(numeric_system) {}
  void on_second(numeric_system) {}
};
}  // namespace internal

#ifdef __cpp_lib_chrono
namespace internal {

template <typename Int>
inline int to_int(Int value) {
  FMT_ASSERT(value >= (std::numeric_limits<int>::min)() &&
             value <= (std::numeric_limits<int>::max)(), "invalid value");
  return static_cast<int>(value);
}

template <typename FormatContext>
struct chrono_formatter {
  FormatContext &context;
  typename FormatContext::iterator out;
  std::chrono::seconds s;
  std::chrono::milliseconds ms;

  using char_type = typename FormatContext::char_type;

  explicit chrono_formatter(FormatContext &ctx)
    : context(ctx), out(ctx.out()) {}

  void write(int value, int width) {
    typedef typename int_traits<int>::main_type main_type;
    main_type n = value;
    int num_digits = internal::count_digits(n);
    if (width > num_digits)
      out = std::fill_n(out, width - num_digits, '0');
    out = format_decimal<char_type>(out, n, num_digits);
  }

  void format_localized(const tm &time, char format) {
    auto locale = context.locale().template get<std::locale>();
    auto &facet = std::use_facet<std::time_put<char_type>>(locale);
    std::basic_ostringstream<char_type> os;
    os.imbue(locale);
    const char format_str[] = {'%', 'O', format};
    facet.put(os, os, ' ', &time, format_str, format_str + sizeof(format_str));
    auto str = os.str();
    std::copy(str.begin(), str.end(), out);
  }

  void on_text(const char_type *begin, const char_type *end) {
    std::copy(begin, end, out);
  }

  void on_abbr_weekday() {}
  void on_full_weekday() {}
  void on_dec0_weekday() {}
  void on_dec1_weekday() {}
  void on_abbr_month() {}
  void on_full_month() {}

  void on_24_hour(numeric_system ns) {
    auto hour = to_int((s.count() / 3600) % 24);
    if (ns == numeric_system::standard)
      return write(hour, 2);
    auto time = tm();
    time.tm_hour = hour;
    format_localized(time, 'H');
  }

  void on_12_hour(numeric_system ns) {
    auto hour = to_int((s.count() / 3600) % 12);
    hour = hour > 0 ? hour : 12;
    if (ns == numeric_system::standard)
      return write(hour, 2);
    auto time = tm();
    time.tm_hour = hour;
    format_localized(time, 'I');
  }

  void on_minute(numeric_system) {
    auto minute = to_int((s.count() / 60) % 60);
    write(minute, 2);
  }

  void on_second(numeric_system) {
    write(to_int(s.count() % 60), 2);
    if (ms != std::chrono::milliseconds()) {
      *out++ = '.';
      write(to_int(ms.count()), 3);
    }
  }
};
}  // namespace internal

template <typename Rep, typename Period, typename Char>
struct formatter<std::chrono::duration<Rep, Period>, Char> {
  mutable basic_string_view<Char> format_str;
  using Duration = std::chrono::duration<Rep, Period>;

  FMT_CONSTEXPR auto parse(basic_parse_context<Char> &ctx)
      -> decltype(ctx.begin()) {
    auto begin = ctx.begin(), end = ctx.end();
    end = parse_chrono_format(begin, end, internal::chrono_format_checker());
    format_str = basic_string_view<Char>(&*begin, end - begin);
    return end;
  }

  template <typename FormatContext>
  auto format(const Duration &d, FormatContext &ctx)
      -> decltype(ctx.out()) {
    internal::chrono_formatter<FormatContext> f(ctx);
    f.s = std::chrono::duration_cast<std::chrono::seconds>(d);
    f.ms = std::chrono::duration_cast<std::chrono::milliseconds>(d - f.s);
    parse_chrono_format(format_str.begin(), format_str.end(), f);
    return f.out;
  }
};
#endif  // __cpp_lib_chrono

// Thread-safe replacement for std::localtime
inline std::tm localtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t): time_(t) {}

    bool run() {
      using namespace fmt::internal;
      return handle(localtime_r(&time_, &tm_));
    }

    bool handle(std::tm *tm) { return tm != FMT_NULL; }

    bool handle(internal::null<>) {
      using namespace fmt::internal;
      return fallback(localtime_s(&tm_, &time_));
    }

    bool fallback(int res) { return res == 0; }

#if !FMT_MSC_VER
    bool fallback(internal::null<>) {
      using namespace fmt::internal;
      std::tm *tm = std::localtime(&time_);
      if (tm) tm_ = *tm;
      return tm != FMT_NULL;
    }
#endif
  };
  dispatcher lt(time);
  // Too big time values may be unsupported.
  if (!lt.run())
    FMT_THROW(format_error("time_t value out of range"));
  return lt.tm_;
}

// Thread-safe replacement for std::gmtime
inline std::tm gmtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t): time_(t) {}

    bool run() {
      using namespace fmt::internal;
      return handle(gmtime_r(&time_, &tm_));
    }

    bool handle(std::tm *tm) { return tm != FMT_NULL; }

    bool handle(internal::null<>) {
      using namespace fmt::internal;
      return fallback(gmtime_s(&tm_, &time_));
    }

    bool fallback(int res) { return res == 0; }

#if !FMT_MSC_VER
    bool fallback(internal::null<>) {
      std::tm *tm = std::gmtime(&time_);
      if (tm) tm_ = *tm;
      return tm != FMT_NULL;
    }
#endif
  };
  dispatcher gt(time);
  // Too big time values may be unsupported.
  if (!gt.run())
    FMT_THROW(format_error("time_t value out of range"));
  return gt.tm_;
}

namespace internal {
inline std::size_t strftime(char *str, std::size_t count, const char *format,
                            const std::tm *time) {
  return std::strftime(str, count, format, time);
}

inline std::size_t strftime(wchar_t *str, std::size_t count,
                            const wchar_t *format, const std::tm *time) {
  return std::wcsftime(str, count, format, time);
}
}

template <typename Char>
struct formatter<std::tm, Char> {
  template <typename ParseContext>
  auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    auto it = internal::null_terminating_iterator<Char>(ctx);
    if (*it == ':')
      ++it;
    auto end = it;
    while (*end && *end != '}')
      ++end;
    tm_format.reserve(end - it + 1);
    using internal::pointer_from;
    tm_format.append(pointer_from(it), pointer_from(end));
    tm_format.push_back('\0');
    return pointer_from(end);
  }

  template <typename FormatContext>
  auto format(const std::tm &tm, FormatContext &ctx) -> decltype(ctx.out()) {
    internal::basic_buffer<Char> &buf = internal::get_container(ctx.out());
    std::size_t start = buf.size();
    for (;;) {
      std::size_t size = buf.capacity() - start;
      std::size_t count =
        internal::strftime(&buf[start], size, &tm_format[0], &tm);
      if (count != 0) {
        buf.resize(start + count);
        break;
      }
      if (size >= tm_format.size() * 256) {
        // If the buffer is 256 times larger than the format string, assume
        // that `strftime` gives an empty result. There doesn't seem to be a
        // better way to distinguish the two cases:
        // https://github.com/fmtlib/fmt/issues/367
        break;
      }
      const std::size_t MIN_GROWTH = 10;
      buf.reserve(buf.capacity() + (size > MIN_GROWTH ? size : MIN_GROWTH));
    }
    return ctx.out();
  }

  basic_memory_buffer<Char> tm_format;
};
FMT_END_NAMESPACE

#endif  // FMT_TIME_H_
