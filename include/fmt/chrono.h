// Formatting library for C++ - chrono support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_CHRONO_H_
#define FMT_CHRONO_H_

#include "format.h"
#include "locale.h"

#include <chrono>
#include <ctime>
#include <locale>
#include <sstream>

FMT_BEGIN_NAMESPACE

namespace internal{

enum class numeric_system {
  standard,
  // Alternative numeric system, e.g. 十二 instead of 12 in ja_JP locale.
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
      handler.on_dec0_weekday(numeric_system::standard);
      break;
    case 'u':
      handler.on_dec1_weekday(numeric_system::standard);
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
    // Other:
    case 'c':
      handler.on_std_datetime();
      break;
    case 'x':
      handler.on_loc_date();
      break;
    case 'X':
      handler.on_loc_time();
      break;
    case 'D':
      handler.on_us_date();
      break;
    case 'F':
      handler.on_iso_date();
      break;
    case 'r':
      handler.on_12_hour_time();
      break;
    case 'R':
      handler.on_24_hour_time();
      break;
    // Alternative numeric system:
    case 'O':
      if (ptr == end)
        throw format_error("invalid format");
      c = *ptr++;
      switch (c) {
      case 'w':
        handler.on_dec0_weekday(numeric_system::alternative);
        break;
      case 'u':
        handler.on_dec1_weekday(numeric_system::alternative);
        break;
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
  void on_dec0_weekday(numeric_system) {}
  void on_dec1_weekday(numeric_system) {}
  void on_abbr_month() {}
  void on_full_month() {}
  void on_24_hour(numeric_system) {}
  void on_12_hour(numeric_system) {}
  void on_minute(numeric_system) {}
  void on_second(numeric_system) {}
  void on_std_datetime() {}
  void on_loc_date() {}
  void on_loc_time() {}
  void on_us_date() {}
  void on_iso_date() {}
  void on_12_hour_time() {}
  void on_24_hour_time() {}
};

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

  typedef typename FormatContext::char_type char_type;

  explicit chrono_formatter(FormatContext &ctx)
    : context(ctx), out(ctx.out()) {}

  int hour() const { return to_int((s.count() / 3600) % 24); }

  int hour12() const {
    auto hour = to_int((s.count() / 3600) % 12);
    return hour > 0 ? hour : 12;
  }

  int minute() const { return to_int((s.count() / 60) % 60); }
  int second() const { return to_int(s.count() % 60); }

  std::tm time() const {
    auto time = std::tm();
    time.tm_hour = hour();
    time.tm_min = minute();
    time.tm_sec = second();
    return time;
  }

  std::tm datetime() const {
    auto t = time();
    t.tm_mday = 1;
    return t;
  }

  std::tm date() const {
    auto t = std::tm();
    t.tm_mday = 1;
    return t;
  }

  void write(int value, int width) {
    typedef typename int_traits<int>::main_type main_type;
    main_type n = to_unsigned(value);
    int num_digits = static_cast<int>(internal::count_digits(n));
    if (width > num_digits)
      out = std::fill_n(out, width - num_digits, '0');
    out = format_decimal<char_type>(out, n, num_digits);
  }

  void format_localized(const tm &time, const char *format) {
    auto locale = context.locale().template get<std::locale>();
    auto &facet = std::use_facet<std::time_put<char_type>>(locale);
    std::basic_ostringstream<char_type> os;
    os.imbue(locale);
    facet.put(os, os, ' ', &time, format, format + std::strlen(format));
    auto str = os.str();
    std::copy(str.begin(), str.end(), out);
  }

  void on_text(const char_type *begin, const char_type *end) {
    std::copy(begin, end, out);
  }

  void on_abbr_weekday() {}
  void on_full_weekday() {}
  void on_dec0_weekday(numeric_system) {}
  void on_dec1_weekday(numeric_system) {}
  void on_abbr_month() {}
  void on_full_month() {}

  void on_24_hour(numeric_system ns) {
    if (ns == numeric_system::standard)
      return write(hour(), 2);
    auto time = tm();
    time.tm_hour = hour();
    format_localized(time, "%OH");
  }

  void on_12_hour(numeric_system ns) {
    if (ns == numeric_system::standard)
      return write(hour12(), 2);
    auto time = tm();
    time.tm_hour = hour();
    format_localized(time, "%OI");
  }

  void on_minute(numeric_system ns) {
    if (ns == numeric_system::standard)
      return write(minute(), 2);
    auto time = tm();
    time.tm_min = minute();
    format_localized(time, "%OM");
  }

  void on_second(numeric_system ns) {
    if (ns == numeric_system::standard) {
      write(second(), 2);
      if (ms != std::chrono::milliseconds(0)) {
        *out++ = '.';
        write(to_int(ms.count()), 3);
      }
      return;
    }
    auto time = tm();
    time.tm_sec = second();
    format_localized(time, "%OS");
  }

  void on_std_datetime() { format_localized(datetime(), "%c"); }
  void on_loc_date() { format_localized(date(), "%x"); }
  void on_loc_time() { format_localized(datetime(), "%X"); }

  void on_us_date() {
    write(1, 2);
    *out++ = '/';
    write(0, 2);
    *out++ = '/';
    write(0, 2);
  }

  void on_iso_date() {
    write(1, 4);
    *out++ = '-';
    write(0, 2);
    *out++ = '-';
    write(0, 2);
  }

  void on_12_hour_time() { format_localized(time(), "%r"); }

  void on_24_hour_time() {
    write(hour(), 2);
    *out++ = ':';
    write(minute(), 2);
  }
};
}  // namespace internal

template <typename Rep, typename Period, typename Char>
struct formatter<std::chrono::duration<Rep, Period>, Char> {
  mutable basic_string_view<Char> format_str;
  typedef std::chrono::duration<Rep, Period> duration;

  FMT_CONSTEXPR auto parse(basic_parse_context<Char> &ctx)
      -> decltype(ctx.begin()) {
    auto begin = ctx.begin(), end = ctx.end();
    end = parse_chrono_format(begin, end, internal::chrono_format_checker());
    format_str = basic_string_view<Char>(&*begin, end - begin);
    return end;
  }

  template <typename FormatContext>
  auto format(const duration &d, FormatContext &ctx)
      -> decltype(ctx.out()) {
    internal::chrono_formatter<FormatContext> f(ctx);
    f.s = std::chrono::duration_cast<std::chrono::seconds>(d);
    f.ms = std::chrono::duration_cast<std::chrono::milliseconds>(d - f.s);
    parse_chrono_format(format_str.begin(), format_str.end(), f);
    return f.out;
  }
};

FMT_END_NAMESPACE

#endif  // FMT_CHRONO_H_
