// Formatting library for C++ - chrono support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_CHRONO_H_
#define FMT_CHRONO_H_

#include <algorithm>
#include <chrono>
#include <ctime>
#include <locale>
#include <sstream>

#include "format.h"

FMT_BEGIN_NAMESPACE

// Enable safe chrono durations, unless explicitly disabled.
#ifndef FMT_SAFE_DURATION_CAST
#  define FMT_SAFE_DURATION_CAST 1
#endif
#if FMT_SAFE_DURATION_CAST

// For conversion between std::chrono::durations without undefined
// behaviour or erroneous results.
// This is a stripped down version of duration_cast, for inclusion in fmt.
// See https://github.com/pauldreik/safe_duration_cast
//
// Copyright Paul Dreik 2019
namespace safe_duration_cast {

template <typename To, typename From,
          FMT_ENABLE_IF(!std::is_same<From, To>::value &&
                        std::numeric_limits<From>::is_signed ==
                            std::numeric_limits<To>::is_signed)>
FMT_CONSTEXPR To lossless_integral_conversion(const From from, int& ec) {
  ec = 0;
  using F = std::numeric_limits<From>;
  using T = std::numeric_limits<To>;
  static_assert(F::is_integer, "From must be integral");
  static_assert(T::is_integer, "To must be integral");

  // A and B are both signed, or both unsigned.
  if (detail::const_check(F::digits <= T::digits)) {
    // From fits in To without any problem.
  } else {
    // From does not always fit in To, resort to a dynamic check.
    if (from < (T::min)() || from > (T::max)()) {
      // outside range.
      ec = 1;
      return {};
    }
  }
  return static_cast<To>(from);
}

/**
 * converts From to To, without loss. If the dynamic value of from
 * can't be converted to To without loss, ec is set.
 */
template <typename To, typename From,
          FMT_ENABLE_IF(!std::is_same<From, To>::value &&
                        std::numeric_limits<From>::is_signed !=
                            std::numeric_limits<To>::is_signed)>
FMT_CONSTEXPR To lossless_integral_conversion(const From from, int& ec) {
  ec = 0;
  using F = std::numeric_limits<From>;
  using T = std::numeric_limits<To>;
  static_assert(F::is_integer, "From must be integral");
  static_assert(T::is_integer, "To must be integral");

  if (detail::const_check(F::is_signed && !T::is_signed)) {
    // From may be negative, not allowed!
    if (fmt::detail::is_negative(from)) {
      ec = 1;
      return {};
    }
    // From is positive. Can it always fit in To?
    if (detail::const_check(F::digits > T::digits) &&
        from > static_cast<From>(detail::max_value<To>())) {
      ec = 1;
      return {};
    }
  }

  if (detail::const_check(!F::is_signed && T::is_signed &&
                          F::digits >= T::digits) &&
      from > static_cast<From>(detail::max_value<To>())) {
    ec = 1;
    return {};
  }
  return static_cast<To>(from);  // Lossless conversion.
}

template <typename To, typename From,
          FMT_ENABLE_IF(std::is_same<From, To>::value)>
FMT_CONSTEXPR To lossless_integral_conversion(const From from, int& ec) {
  ec = 0;
  return from;
}  // function

// clang-format off
/**
 * converts From to To if possible, otherwise ec is set.
 *
 * input                            |    output
 * ---------------------------------|---------------
 * NaN                              | NaN
 * Inf                              | Inf
 * normal, fits in output           | converted (possibly lossy)
 * normal, does not fit in output   | ec is set
 * subnormal                        | best effort
 * -Inf                             | -Inf
 */
// clang-format on
template <typename To, typename From,
          FMT_ENABLE_IF(!std::is_same<From, To>::value)>
FMT_CONSTEXPR To safe_float_conversion(const From from, int& ec) {
  ec = 0;
  using T = std::numeric_limits<To>;
  static_assert(std::is_floating_point<From>::value, "From must be floating");
  static_assert(std::is_floating_point<To>::value, "To must be floating");

  // catch the only happy case
  if (std::isfinite(from)) {
    if (from >= T::lowest() && from <= (T::max)()) {
      return static_cast<To>(from);
    }
    // not within range.
    ec = 1;
    return {};
  }

  // nan and inf will be preserved
  return static_cast<To>(from);
}  // function

template <typename To, typename From,
          FMT_ENABLE_IF(std::is_same<From, To>::value)>
FMT_CONSTEXPR To safe_float_conversion(const From from, int& ec) {
  ec = 0;
  static_assert(std::is_floating_point<From>::value, "From must be floating");
  return from;
}

/**
 * safe duration cast between integral durations
 */
template <typename To, typename FromRep, typename FromPeriod,
          FMT_ENABLE_IF(std::is_integral<FromRep>::value),
          FMT_ENABLE_IF(std::is_integral<typename To::rep>::value)>
To safe_duration_cast(std::chrono::duration<FromRep, FromPeriod> from,
                      int& ec) {
  using From = std::chrono::duration<FromRep, FromPeriod>;
  ec = 0;
  // the basic idea is that we need to convert from count() in the from type
  // to count() in the To type, by multiplying it with this:
  struct Factor
      : std::ratio_divide<typename From::period, typename To::period> {};

  static_assert(Factor::num > 0, "num must be positive");
  static_assert(Factor::den > 0, "den must be positive");

  // the conversion is like this: multiply from.count() with Factor::num
  // /Factor::den and convert it to To::rep, all this without
  // overflow/underflow. let's start by finding a suitable type that can hold
  // both To, From and Factor::num
  using IntermediateRep =
      typename std::common_type<typename From::rep, typename To::rep,
                                decltype(Factor::num)>::type;

  // safe conversion to IntermediateRep
  IntermediateRep count =
      lossless_integral_conversion<IntermediateRep>(from.count(), ec);
  if (ec) return {};
  // multiply with Factor::num without overflow or underflow
  if (detail::const_check(Factor::num != 1)) {
    const auto max1 = detail::max_value<IntermediateRep>() / Factor::num;
    if (count > max1) {
      ec = 1;
      return {};
    }
    const auto min1 =
        (std::numeric_limits<IntermediateRep>::min)() / Factor::num;
    if (count < min1) {
      ec = 1;
      return {};
    }
    count *= Factor::num;
  }

  if (detail::const_check(Factor::den != 1)) count /= Factor::den;
  auto tocount = lossless_integral_conversion<typename To::rep>(count, ec);
  return ec ? To() : To(tocount);
}

/**
 * safe duration_cast between floating point durations
 */
template <typename To, typename FromRep, typename FromPeriod,
          FMT_ENABLE_IF(std::is_floating_point<FromRep>::value),
          FMT_ENABLE_IF(std::is_floating_point<typename To::rep>::value)>
To safe_duration_cast(std::chrono::duration<FromRep, FromPeriod> from,
                      int& ec) {
  using From = std::chrono::duration<FromRep, FromPeriod>;
  ec = 0;
  if (std::isnan(from.count())) {
    // nan in, gives nan out. easy.
    return To{std::numeric_limits<typename To::rep>::quiet_NaN()};
  }
  // maybe we should also check if from is denormal, and decide what to do about
  // it.

  // +-inf should be preserved.
  if (std::isinf(from.count())) {
    return To{from.count()};
  }

  // the basic idea is that we need to convert from count() in the from type
  // to count() in the To type, by multiplying it with this:
  struct Factor
      : std::ratio_divide<typename From::period, typename To::period> {};

  static_assert(Factor::num > 0, "num must be positive");
  static_assert(Factor::den > 0, "den must be positive");

  // the conversion is like this: multiply from.count() with Factor::num
  // /Factor::den and convert it to To::rep, all this without
  // overflow/underflow. let's start by finding a suitable type that can hold
  // both To, From and Factor::num
  using IntermediateRep =
      typename std::common_type<typename From::rep, typename To::rep,
                                decltype(Factor::num)>::type;

  // force conversion of From::rep -> IntermediateRep to be safe,
  // even if it will never happen be narrowing in this context.
  IntermediateRep count =
      safe_float_conversion<IntermediateRep>(from.count(), ec);
  if (ec) {
    return {};
  }

  // multiply with Factor::num without overflow or underflow
  if (detail::const_check(Factor::num != 1)) {
    constexpr auto max1 = detail::max_value<IntermediateRep>() /
                          static_cast<IntermediateRep>(Factor::num);
    if (count > max1) {
      ec = 1;
      return {};
    }
    constexpr auto min1 = std::numeric_limits<IntermediateRep>::lowest() /
                          static_cast<IntermediateRep>(Factor::num);
    if (count < min1) {
      ec = 1;
      return {};
    }
    count *= static_cast<IntermediateRep>(Factor::num);
  }

  // this can't go wrong, right? den>0 is checked earlier.
  if (detail::const_check(Factor::den != 1)) {
    using common_t = typename std::common_type<IntermediateRep, intmax_t>::type;
    count /= static_cast<common_t>(Factor::den);
  }

  // convert to the to type, safely
  using ToRep = typename To::rep;

  const ToRep tocount = safe_float_conversion<ToRep>(count, ec);
  if (ec) {
    return {};
  }
  return To{tocount};
}
}  // namespace safe_duration_cast
#endif

// Prevents expansion of a preceding token as a function-style macro.
// Usage: f FMT_NOMACRO()
#define FMT_NOMACRO

namespace detail {
template <typename T = void> struct null {};
inline null<> localtime_r FMT_NOMACRO(...) { return null<>(); }
inline null<> localtime_s(...) { return null<>(); }
inline null<> gmtime_r(...) { return null<>(); }
inline null<> gmtime_s(...) { return null<>(); }

template <typename Char>
inline auto do_write(const std::tm& time, const std::locale& loc, char format,
                     char modifier) -> std::basic_string<Char> {
  auto&& os = std::basic_ostringstream<Char>();
  os.imbue(loc);
  using iterator = std::ostreambuf_iterator<Char>;
  const auto& facet = std::use_facet<std::time_put<Char, iterator>>(loc);
  auto end = facet.put(os, os, Char(' '), &time, format, modifier);
  if (end.failed()) FMT_THROW(format_error("failed to format time"));
  return std::move(os).str();
}

template <typename Char, typename OutputIt,
          FMT_ENABLE_IF(!std::is_same<Char, char>::value)>
auto write(OutputIt out, const std::tm& time, const std::locale& loc,
           char format, char modifier = 0) -> OutputIt {
  auto str = do_write<Char>(time, loc, format, modifier);
  return std::copy(str.begin(), str.end(), out);
}

inline const std::locale& get_classic_locale() {
  static const auto& locale = std::locale::classic();
  return locale;
}

template <typename Char, typename OutputIt,
          FMT_ENABLE_IF(std::is_same<Char, char>::value)>
auto write(OutputIt out, const std::tm& time, const std::locale& loc,
           char format, char modifier = 0) -> OutputIt {
  auto str = do_write<char>(time, loc, format, modifier);
  if (detail::is_utf8() && loc != get_classic_locale()) {
    // char16_t and char32_t codecvts are broken in MSVC (linkage errors) and
    // gcc-4.
#if FMT_MSC_VER != 0 || \
    (defined(__GLIBCXX__) && !defined(_GLIBCXX_USE_DUAL_ABI))
    // The _GLIBCXX_USE_DUAL_ABI macro is always defined in libstdc++ from gcc-5
    // and newer.
    using code_unit = wchar_t;
#else
    using code_unit = char32_t;
#endif

    using codecvt = std::codecvt<code_unit, char, std::mbstate_t>;
#if FMT_CLANG_VERSION
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wdeprecated"
    auto& f = std::use_facet<codecvt>(loc);
#  pragma clang diagnostic pop
#else
    auto& f = std::use_facet<codecvt>(loc);
#endif

    auto mb = std::mbstate_t();
    const char* from_next = nullptr;
    code_unit* to_next = nullptr;
    constexpr size_t buf_size = 32;
    code_unit buf[buf_size] = {};
    auto result = f.in(mb, str.data(), str.data() + str.size(), from_next, buf,
                       buf + buf_size, to_next);
    if (result != std::codecvt_base::ok)
      FMT_THROW(format_error("failed to format time"));
    str.clear();
    for (code_unit* p = buf; p != to_next; ++p) {
      uint32_t c = static_cast<uint32_t>(*p);
      if (sizeof(code_unit) == 2 && c >= 0xd800 && c <= 0xdfff) {
        // surrogate pair
        ++p;
        if (p == to_next || (c & 0xfc00) != 0xd800 || (*p & 0xfc00) != 0xdc00) {
          FMT_THROW(format_error("failed to format time"));
        }
        c = (c << 10) + static_cast<uint32_t>(*p) - 0x35fdc00;
      }
      if (c < 0x80) {
        str.push_back(static_cast<char>(c));
      } else if (c < 0x800) {
        str.push_back(static_cast<char>(0xc0 | (c >> 6)));
        str.push_back(static_cast<char>(0x80 | (c & 0x3f)));
      } else if ((c >= 0x800 && c <= 0xd7ff) || (c >= 0xe000 && c <= 0xffff)) {
        str.push_back(static_cast<char>(0xe0 | (c >> 12)));
        str.push_back(static_cast<char>(0x80 | ((c & 0xfff) >> 6)));
        str.push_back(static_cast<char>(0x80 | (c & 0x3f)));
      } else if (c >= 0x10000 && c <= 0x10ffff) {
        str.push_back(static_cast<char>(0xf0 | (c >> 18)));
        str.push_back(static_cast<char>(0x80 | ((c & 0x3ffff) >> 12)));
        str.push_back(static_cast<char>(0x80 | ((c & 0xfff) >> 6)));
        str.push_back(static_cast<char>(0x80 | (c & 0x3f)));
      } else {
        FMT_THROW(format_error("failed to format time"));
      }
    }
  }
  return std::copy(str.begin(), str.end(), out);
}

}  // namespace detail

FMT_MODULE_EXPORT_BEGIN

/**
  Converts given time since epoch as ``std::time_t`` value into calendar time,
  expressed in local time. Unlike ``std::localtime``, this function is
  thread-safe on most platforms.
 */
inline std::tm localtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t) : time_(t) {}

    bool run() {
      using namespace fmt::detail;
      return handle(localtime_r(&time_, &tm_));
    }

    bool handle(std::tm* tm) { return tm != nullptr; }

    bool handle(detail::null<>) {
      using namespace fmt::detail;
      return fallback(localtime_s(&tm_, &time_));
    }

    bool fallback(int res) { return res == 0; }

#if !FMT_MSC_VER
    bool fallback(detail::null<>) {
      using namespace fmt::detail;
      std::tm* tm = std::localtime(&time_);
      if (tm) tm_ = *tm;
      return tm != nullptr;
    }
#endif
  };
  dispatcher lt(time);
  // Too big time values may be unsupported.
  if (!lt.run()) FMT_THROW(format_error("time_t value out of range"));
  return lt.tm_;
}

inline std::tm localtime(
    std::chrono::time_point<std::chrono::system_clock> time_point) {
  return localtime(std::chrono::system_clock::to_time_t(time_point));
}

/**
  Converts given time since epoch as ``std::time_t`` value into calendar time,
  expressed in Coordinated Universal Time (UTC). Unlike ``std::gmtime``, this
  function is thread-safe on most platforms.
 */
inline std::tm gmtime(std::time_t time) {
  struct dispatcher {
    std::time_t time_;
    std::tm tm_;

    dispatcher(std::time_t t) : time_(t) {}

    bool run() {
      using namespace fmt::detail;
      return handle(gmtime_r(&time_, &tm_));
    }

    bool handle(std::tm* tm) { return tm != nullptr; }

    bool handle(detail::null<>) {
      using namespace fmt::detail;
      return fallback(gmtime_s(&tm_, &time_));
    }

    bool fallback(int res) { return res == 0; }

#if !FMT_MSC_VER
    bool fallback(detail::null<>) {
      std::tm* tm = std::gmtime(&time_);
      if (tm) tm_ = *tm;
      return tm != nullptr;
    }
#endif
  };
  dispatcher gt(time);
  // Too big time values may be unsupported.
  if (!gt.run()) FMT_THROW(format_error("time_t value out of range"));
  return gt.tm_;
}

inline std::tm gmtime(
    std::chrono::time_point<std::chrono::system_clock> time_point) {
  return gmtime(std::chrono::system_clock::to_time_t(time_point));
}

FMT_BEGIN_DETAIL_NAMESPACE

// Writes two-digit numbers a, b and c separated by sep to buf.
// The method by Pavel Novikov based on
// https://johnnylee-sde.github.io/Fast-unsigned-integer-to-time-string/.
inline void write_digit2_separated(char* buf, unsigned a, unsigned b,
                                   unsigned c, char sep) {
  unsigned long long digits =
      a | (b << 24) | (static_cast<unsigned long long>(c) << 48);
  // Convert each value to BCD.
  // We have x = a * 10 + b and we want to convert it to BCD y = a * 16 + b.
  // The difference is
  //   y - x = a * 6
  // a can be found from x:
  //   a = floor(x / 10)
  // then
  //   y = x + a * 6 = x + floor(x / 10) * 6
  // floor(x / 10) is (x * 205) >> 11 (needs 16 bits).
  digits += (((digits * 205) >> 11) & 0x000f00000f00000f) * 6;
  // Put low nibbles to high bytes and high nibbles to low bytes.
  digits = ((digits & 0x00f00000f00000f0) >> 4) |
           ((digits & 0x000f00000f00000f) << 8);
  auto usep = static_cast<unsigned long long>(sep);
  // Add ASCII '0' to each digit byte and insert separators.
  digits |= 0x3030003030003030 | (usep << 16) | (usep << 40);
  memcpy(buf, &digits, 8);
}

template <typename Period> FMT_CONSTEXPR inline const char* get_units() {
  if (std::is_same<Period, std::atto>::value) return "as";
  if (std::is_same<Period, std::femto>::value) return "fs";
  if (std::is_same<Period, std::pico>::value) return "ps";
  if (std::is_same<Period, std::nano>::value) return "ns";
  if (std::is_same<Period, std::micro>::value) return "µs";
  if (std::is_same<Period, std::milli>::value) return "ms";
  if (std::is_same<Period, std::centi>::value) return "cs";
  if (std::is_same<Period, std::deci>::value) return "ds";
  if (std::is_same<Period, std::ratio<1>>::value) return "s";
  if (std::is_same<Period, std::deca>::value) return "das";
  if (std::is_same<Period, std::hecto>::value) return "hs";
  if (std::is_same<Period, std::kilo>::value) return "ks";
  if (std::is_same<Period, std::mega>::value) return "Ms";
  if (std::is_same<Period, std::giga>::value) return "Gs";
  if (std::is_same<Period, std::tera>::value) return "Ts";
  if (std::is_same<Period, std::peta>::value) return "Ps";
  if (std::is_same<Period, std::exa>::value) return "Es";
  if (std::is_same<Period, std::ratio<60>>::value) return "m";
  if (std::is_same<Period, std::ratio<3600>>::value) return "h";
  return nullptr;
}

enum class numeric_system {
  standard,
  // Alternative numeric system, e.g. 十二 instead of 12 in ja_JP locale.
  alternative
};

// Parses a put_time-like format string and invokes handler actions.
template <typename Char, typename Handler>
FMT_CONSTEXPR const Char* parse_chrono_format(const Char* begin,
                                              const Char* end,
                                              Handler&& handler) {
  auto ptr = begin;
  while (ptr != end) {
    auto c = *ptr;
    if (c == '}') break;
    if (c != '%') {
      ++ptr;
      continue;
    }
    if (begin != ptr) handler.on_text(begin, ptr);
    ++ptr;  // consume '%'
    if (ptr == end) FMT_THROW(format_error("invalid format"));
    c = *ptr++;
    switch (c) {
    case '%':
      handler.on_text(ptr - 1, ptr);
      break;
    case 'n': {
      const Char newline[] = {'\n'};
      handler.on_text(newline, newline + 1);
      break;
    }
    case 't': {
      const Char tab[] = {'\t'};
      handler.on_text(tab, tab + 1);
      break;
    }
    // Year:
    case 'Y':
      handler.on_year(numeric_system::standard);
      break;
    case 'y':
      handler.on_short_year(numeric_system::standard);
      break;
    case 'C':
      handler.on_century(numeric_system::standard);
      break;
    case 'G':
      handler.on_iso_week_based_year();
      break;
    case 'g':
      handler.on_iso_week_based_short_year();
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
    case 'h':
      handler.on_abbr_month();
      break;
    case 'B':
      handler.on_full_month();
      break;
    case 'm':
      handler.on_dec_month(numeric_system::standard);
      break;
    // Day of the year/month:
    case 'U':
      handler.on_dec0_week_of_year(numeric_system::standard);
      break;
    case 'W':
      handler.on_dec1_week_of_year(numeric_system::standard);
      break;
    case 'V':
      handler.on_iso_week_of_year(numeric_system::standard);
      break;
    case 'j':
      handler.on_day_of_year();
      break;
    case 'd':
      handler.on_day_of_month(numeric_system::standard);
      break;
    case 'e':
      handler.on_day_of_month_space(numeric_system::standard);
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
      handler.on_datetime(numeric_system::standard);
      break;
    case 'x':
      handler.on_loc_date(numeric_system::standard);
      break;
    case 'X':
      handler.on_loc_time(numeric_system::standard);
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
    case 'T':
      handler.on_iso_time();
      break;
    case 'p':
      handler.on_am_pm();
      break;
    case 'Q':
      handler.on_duration_value();
      break;
    case 'q':
      handler.on_duration_unit();
      break;
    case 'z':
      handler.on_utc_offset();
      break;
    case 'Z':
      handler.on_tz_name();
      break;
    // Alternative representation:
    case 'E': {
      if (ptr == end) FMT_THROW(format_error("invalid format"));
      c = *ptr++;
      switch (c) {
      case 'Y':
        handler.on_year(numeric_system::alternative);
        break;
      case 'y':
        handler.on_offset_year();
        break;
      case 'C':
        handler.on_century(numeric_system::alternative);
        break;
      case 'c':
        handler.on_datetime(numeric_system::alternative);
        break;
      case 'x':
        handler.on_loc_date(numeric_system::alternative);
        break;
      case 'X':
        handler.on_loc_time(numeric_system::alternative);
        break;
      default:
        FMT_THROW(format_error("invalid format"));
      }
      break;
    }
    case 'O':
      if (ptr == end) FMT_THROW(format_error("invalid format"));
      c = *ptr++;
      switch (c) {
      case 'y':
        handler.on_short_year(numeric_system::alternative);
        break;
      case 'm':
        handler.on_dec_month(numeric_system::alternative);
        break;
      case 'U':
        handler.on_dec0_week_of_year(numeric_system::alternative);
        break;
      case 'W':
        handler.on_dec1_week_of_year(numeric_system::alternative);
        break;
      case 'V':
        handler.on_iso_week_of_year(numeric_system::alternative);
        break;
      case 'd':
        handler.on_day_of_month(numeric_system::alternative);
        break;
      case 'e':
        handler.on_day_of_month_space(numeric_system::alternative);
        break;
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
      default:
        FMT_THROW(format_error("invalid format"));
      }
      break;
    default:
      FMT_THROW(format_error("invalid format"));
    }
    begin = ptr;
  }
  if (begin != ptr) handler.on_text(begin, ptr);
  return ptr;
}

template <typename Derived> struct null_chrono_spec_handler {
  FMT_CONSTEXPR void unsupported() {
    static_cast<Derived*>(this)->unsupported();
  }
  FMT_CONSTEXPR void on_year(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_short_year(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_offset_year() { unsupported(); }
  FMT_CONSTEXPR void on_century(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_iso_week_based_year() { unsupported(); }
  FMT_CONSTEXPR void on_iso_week_based_short_year() { unsupported(); }
  FMT_CONSTEXPR void on_abbr_weekday() { unsupported(); }
  FMT_CONSTEXPR void on_full_weekday() { unsupported(); }
  FMT_CONSTEXPR void on_dec0_weekday(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_dec1_weekday(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_abbr_month() { unsupported(); }
  FMT_CONSTEXPR void on_full_month() { unsupported(); }
  FMT_CONSTEXPR void on_dec_month(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_dec0_week_of_year(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_dec1_week_of_year(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_iso_week_of_year(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_day_of_year() { unsupported(); }
  FMT_CONSTEXPR void on_day_of_month(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_day_of_month_space(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_24_hour(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_12_hour(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_minute(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_second(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_datetime(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_loc_date(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_loc_time(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_us_date() { unsupported(); }
  FMT_CONSTEXPR void on_iso_date() { unsupported(); }
  FMT_CONSTEXPR void on_12_hour_time() { unsupported(); }
  FMT_CONSTEXPR void on_24_hour_time() { unsupported(); }
  FMT_CONSTEXPR void on_iso_time() { unsupported(); }
  FMT_CONSTEXPR void on_am_pm() { unsupported(); }
  FMT_CONSTEXPR void on_duration_value() { unsupported(); }
  FMT_CONSTEXPR void on_duration_unit() { unsupported(); }
  FMT_CONSTEXPR void on_utc_offset() { unsupported(); }
  FMT_CONSTEXPR void on_tz_name() { unsupported(); }
};

struct tm_format_checker : null_chrono_spec_handler<tm_format_checker> {
  FMT_NORETURN void unsupported() { FMT_THROW(format_error("no format")); }

  template <typename Char>
  FMT_CONSTEXPR void on_text(const Char*, const Char*) {}
  FMT_CONSTEXPR void on_year(numeric_system) {}
  FMT_CONSTEXPR void on_short_year(numeric_system) {}
  FMT_CONSTEXPR void on_offset_year() {}
  FMT_CONSTEXPR void on_century(numeric_system) {}
  FMT_CONSTEXPR void on_iso_week_based_year() {}
  FMT_CONSTEXPR void on_iso_week_based_short_year() {}
  FMT_CONSTEXPR void on_abbr_weekday() {}
  FMT_CONSTEXPR void on_full_weekday() {}
  FMT_CONSTEXPR void on_dec0_weekday(numeric_system) {}
  FMT_CONSTEXPR void on_dec1_weekday(numeric_system) {}
  FMT_CONSTEXPR void on_abbr_month() {}
  FMT_CONSTEXPR void on_full_month() {}
  FMT_CONSTEXPR void on_dec_month(numeric_system) {}
  FMT_CONSTEXPR void on_dec0_week_of_year(numeric_system) {}
  FMT_CONSTEXPR void on_dec1_week_of_year(numeric_system) {}
  FMT_CONSTEXPR void on_iso_week_of_year(numeric_system) {}
  FMT_CONSTEXPR void on_day_of_year() {}
  FMT_CONSTEXPR void on_day_of_month(numeric_system) {}
  FMT_CONSTEXPR void on_day_of_month_space(numeric_system) {}
  FMT_CONSTEXPR void on_24_hour(numeric_system) {}
  FMT_CONSTEXPR void on_12_hour(numeric_system) {}
  FMT_CONSTEXPR void on_minute(numeric_system) {}
  FMT_CONSTEXPR void on_second(numeric_system) {}
  FMT_CONSTEXPR void on_datetime(numeric_system) {}
  FMT_CONSTEXPR void on_loc_date(numeric_system) {}
  FMT_CONSTEXPR void on_loc_time(numeric_system) {}
  FMT_CONSTEXPR void on_us_date() {}
  FMT_CONSTEXPR void on_iso_date() {}
  FMT_CONSTEXPR void on_12_hour_time() {}
  FMT_CONSTEXPR void on_24_hour_time() {}
  FMT_CONSTEXPR void on_iso_time() {}
  FMT_CONSTEXPR void on_am_pm() {}
  FMT_CONSTEXPR void on_utc_offset() {}
  FMT_CONSTEXPR void on_tz_name() {}
};

inline const char* tm_wday_full_name(int wday) {
  static constexpr const char* full_name_list[] = {
      "Sunday",   "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"};
  return wday >= 0 && wday <= 6 ? full_name_list[wday] : "?";
}
inline const char* tm_wday_short_name(int wday) {
  static constexpr const char* short_name_list[] = {"Sun", "Mon", "Tue", "Wed",
                                                    "Thu", "Fri", "Sat"};
  return wday >= 0 && wday <= 6 ? short_name_list[wday] : "???";
}

inline const char* tm_mon_full_name(int mon) {
  static constexpr const char* full_name_list[] = {
      "January", "February", "March",     "April",   "May",      "June",
      "July",    "August",   "September", "October", "November", "December"};
  return mon >= 0 && mon <= 11 ? full_name_list[mon] : "?";
}
inline const char* tm_mon_short_name(int mon) {
  static constexpr const char* short_name_list[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };
  return mon >= 0 && mon <= 11 ? short_name_list[mon] : "???";
}

template <typename OutputIt, typename Char> class tm_writer {
 private:
  static constexpr int days_per_week = 7;

  const std::locale& loc_;
  const bool is_classic_;
  OutputIt out_;
  const std::tm& tm_;

  auto tm_sec() const noexcept -> int {
    FMT_ASSERT(tm_.tm_sec >= 0 && tm_.tm_sec <= 61, "");
    return tm_.tm_sec;
  }
  auto tm_min() const noexcept -> int {
    FMT_ASSERT(tm_.tm_min >= 0 && tm_.tm_min <= 59, "");
    return tm_.tm_min;
  }
  auto tm_hour() const noexcept -> int {
    FMT_ASSERT(tm_.tm_hour >= 0 && tm_.tm_hour <= 23, "");
    return tm_.tm_hour;
  }
  auto tm_mday() const noexcept -> int {
    FMT_ASSERT(tm_.tm_mday >= 1 && tm_.tm_mday <= 31, "");
    return tm_.tm_mday;
  }
  auto tm_mon() const noexcept -> int {
    FMT_ASSERT(tm_.tm_mon >= 0 && tm_.tm_mon <= 11, "");
    return tm_.tm_mon;
  }
  auto tm_year() const noexcept -> long long { return 1900ll + tm_.tm_year; }
  auto tm_wday() const noexcept -> int {
    FMT_ASSERT(tm_.tm_wday >= 0 && tm_.tm_wday <= 6, "");
    return tm_.tm_wday;
  }
  auto tm_yday() const noexcept -> int {
    FMT_ASSERT(tm_.tm_yday >= 0 && tm_.tm_yday <= 365, "");
    return tm_.tm_yday;
  }

  auto tm_hour12() const noexcept -> int {
    const auto h = tm_hour();
    const auto z = h < 12 ? h : h - 12;
    return z == 0 ? 12 : z;
  }

  // POSIX and the C Standard are unclear or inconsistent about what %C and %y
  // do if the year is negative or exceeds 9999. Use the convention that %C
  // concatenated with %y yields the same output as %Y, and that %Y contains at
  // least 4 characters, with more only if necessary.
  auto split_year_lower(long long year) const noexcept -> int {
    auto l = year % 100;
    if (l < 0) l = -l;  // l in [0, 99]
    return static_cast<int>(l);
  }

  // Algorithm:
  // https://en.wikipedia.org/wiki/ISO_week_date#Calculating_the_week_number_from_a_month_and_day_of_the_month_or_ordinal_date
  auto iso_year_weeks(long long curr_year) const noexcept -> int {
    const auto prev_year = curr_year - 1;
    const auto curr_p =
        (curr_year + curr_year / 4 - curr_year / 100 + curr_year / 400) %
        days_per_week;
    const auto prev_p =
        (prev_year + prev_year / 4 - prev_year / 100 + prev_year / 400) %
        days_per_week;
    return 52 + ((curr_p == 4 || prev_p == 3) ? 1 : 0);
  }
  auto iso_week_num(int tm_yday, int tm_wday) const noexcept -> int {
    return (tm_yday + 11 - (tm_wday == 0 ? days_per_week : tm_wday)) /
           days_per_week;
  }
  auto tm_iso_week_year() const noexcept -> long long {
    const auto year = tm_year();
    const auto w = iso_week_num(tm_yday(), tm_wday());
    if (w < 1) return year - 1;
    if (w > iso_year_weeks(year)) return year + 1;
    return year;
  }
  auto tm_iso_week_of_year() const noexcept -> int {
    const auto year = tm_year();
    const auto w = iso_week_num(tm_yday(), tm_wday());
    if (w < 1) return iso_year_weeks(year - 1);
    if (w > iso_year_weeks(year)) return 1;
    return w;
  }

  void write1(int value) {
    *out_++ = static_cast<char>('0' + to_unsigned(value) % 10);
  }
  void write2(int value) {
    const char* d = digits2(to_unsigned(value) % 100);
    *out_++ = *d++;
    *out_++ = *d;
  }

  void write_year_extended(long long year) {
    // At least 4 characters.
    int width = 4;
    if (year < 0) {
      *out_++ = '-';
      year = 0 - year;
      --width;
    }
    uint32_or_64_or_128_t<long long> n = to_unsigned(year);
    const int num_digits = count_digits(n);
    if (width > num_digits) out_ = std::fill_n(out_, width - num_digits, '0');
    out_ = format_decimal<Char>(out_, n, num_digits).end;
  }
  void write_year(long long year) {
    if (year >= 0 && year < 10000) {
      write2(static_cast<int>(year / 100));
      write2(static_cast<int>(year % 100));
    } else {
      write_year_extended(year);
    }
  }

  void format_localized(char format, char modifier = 0) {
    out_ = write<Char>(out_, tm_, loc_, format, modifier);
  }

 public:
  tm_writer(const std::locale& loc, OutputIt out, const std::tm& tm)
      : loc_(loc),
        is_classic_(loc_ == get_classic_locale()),
        out_(out),
        tm_(tm) {}

  OutputIt out() const { return out_; }

  FMT_CONSTEXPR void on_text(const Char* begin, const Char* end) {
    out_ = copy_str<Char>(begin, end, out_);
  }

  void on_abbr_weekday() {
    if (is_classic_)
      out_ = write(out_, tm_wday_short_name(tm_wday()));
    else
      format_localized('a');
  }
  void on_full_weekday() {
    if (is_classic_)
      out_ = write(out_, tm_wday_full_name(tm_wday()));
    else
      format_localized('A');
  }
  void on_dec0_weekday(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('w', 'O');
    write1(tm_wday());
  }
  void on_dec1_weekday(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('u', 'O');
    auto wday = tm_wday();
    write1(wday == 0 ? days_per_week : wday);
  }

  void on_abbr_month() {
    if (is_classic_)
      out_ = write(out_, tm_mon_short_name(tm_mon()));
    else
      format_localized('b');
  }
  void on_full_month() {
    if (is_classic_)
      out_ = write(out_, tm_mon_full_name(tm_mon()));
    else
      format_localized('B');
  }

  void on_datetime(numeric_system ns) {
    if (is_classic_) {
      on_abbr_weekday();
      *out_++ = ' ';
      on_abbr_month();
      *out_++ = ' ';
      on_day_of_month_space(numeric_system::standard);
      *out_++ = ' ';
      on_iso_time();
      *out_++ = ' ';
      on_year(numeric_system::standard);
    } else {
      format_localized('c', ns == numeric_system::standard ? '\0' : 'E');
    }
  }
  void on_loc_date(numeric_system ns) {
    if (is_classic_)
      on_us_date();
    else
      format_localized('x', ns == numeric_system::standard ? '\0' : 'E');
  }
  void on_loc_time(numeric_system ns) {
    if (is_classic_)
      on_iso_time();
    else
      format_localized('X', ns == numeric_system::standard ? '\0' : 'E');
  }
  void on_us_date() {
    char buf[8];
    write_digit2_separated(buf, to_unsigned(tm_mon() + 1),
                           to_unsigned(tm_mday()),
                           to_unsigned(split_year_lower(tm_year())), '/');
    out_ = copy_str<Char>(std::begin(buf), std::end(buf), out_);
  }
  void on_iso_date() {
    auto year = tm_year();
    char buf[10];
    size_t offset = 0;
    if (year >= 0 && year < 10000) {
      copy2(buf, digits2(to_unsigned(year / 100)));
    } else {
      offset = 4;
      write_year_extended(year);
      year = 0;
    }
    write_digit2_separated(buf + 2, static_cast<unsigned>(year % 100),
                           to_unsigned(tm_mon() + 1), to_unsigned(tm_mday()),
                           '-');
    out_ = copy_str<Char>(std::begin(buf) + offset, std::end(buf), out_);
  }

  void on_utc_offset() { format_localized('z'); }
  void on_tz_name() { format_localized('Z'); }

  void on_year(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('Y', 'E');
    write_year(tm_year());
  }
  void on_short_year(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('y', 'O');
    write2(split_year_lower(tm_year()));
  }
  void on_offset_year() { format_localized('y', 'E'); }

  void on_century(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('C', 'E');
    auto year = tm_year();
    auto upper = year / 100;
    if (year >= -99 && year < 0) {
      // Zero upper on negative year.
      *out_++ = '-';
      *out_++ = '0';
    } else if (upper >= 0 && upper < 100) {
      write2(static_cast<int>(upper));
    } else {
      out_ = write<Char>(out_, upper);
    }
  }

  void on_dec_month(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('m', 'O');
    write2(tm_mon() + 1);
  }

  void on_dec0_week_of_year(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('U', 'O');
    write2((tm_yday() + days_per_week - tm_wday()) / days_per_week);
  }
  void on_dec1_week_of_year(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('W', 'O');
    auto wday = tm_wday();
    write2((tm_yday() + days_per_week -
            (wday == 0 ? (days_per_week - 1) : (wday - 1))) /
           days_per_week);
  }
  void on_iso_week_of_year(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('V', 'O');
    write2(tm_iso_week_of_year());
  }

  void on_iso_week_based_year() { write_year(tm_iso_week_year()); }
  void on_iso_week_based_short_year() {
    write2(split_year_lower(tm_iso_week_year()));
  }

  void on_day_of_year() {
    auto yday = tm_yday() + 1;
    write1(yday / 100);
    write2(yday % 100);
  }
  void on_day_of_month(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('d', 'O');
    write2(tm_mday());
  }
  void on_day_of_month_space(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('e', 'O');
    auto mday = to_unsigned(tm_mday()) % 100;
    const char* d2 = digits2(mday);
    *out_++ = mday < 10 ? ' ' : d2[0];
    *out_++ = d2[1];
  }

  void on_24_hour(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('H', 'O');
    write2(tm_hour());
  }
  void on_12_hour(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('I', 'O');
    write2(tm_hour12());
  }
  void on_minute(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('M', 'O');
    write2(tm_min());
  }
  void on_second(numeric_system ns) {
    if (ns != numeric_system::standard) return format_localized('S', 'O');
    write2(tm_sec());
  }

  void on_12_hour_time() {
    if (is_classic_) {
      char buf[8];
      write_digit2_separated(buf, to_unsigned(tm_hour12()),
                             to_unsigned(tm_min()), to_unsigned(tm_sec()), ':');
      out_ = copy_str<Char>(std::begin(buf), std::end(buf), out_);
      *out_++ = ' ';
      on_am_pm();
    } else {
      format_localized('r');
    }
  }
  void on_24_hour_time() {
    write2(tm_hour());
    *out_++ = ':';
    write2(tm_min());
  }
  void on_iso_time() {
    char buf[8];
    write_digit2_separated(buf, to_unsigned(tm_hour()), to_unsigned(tm_min()),
                           to_unsigned(tm_sec()), ':');
    out_ = copy_str<Char>(std::begin(buf), std::end(buf), out_);
  }

  void on_am_pm() {
    if (is_classic_) {
      *out_++ = tm_hour() < 12 ? 'A' : 'P';
      *out_++ = 'M';
    } else {
      format_localized('p');
    }
  }

  // These apply to chrono durations but not tm.
  void on_duration_value() {}
  void on_duration_unit() {}
};

struct chrono_format_checker : null_chrono_spec_handler<chrono_format_checker> {
  FMT_NORETURN void unsupported() { FMT_THROW(format_error("no date")); }

  template <typename Char>
  FMT_CONSTEXPR void on_text(const Char*, const Char*) {}
  FMT_CONSTEXPR void on_24_hour(numeric_system) {}
  FMT_CONSTEXPR void on_12_hour(numeric_system) {}
  FMT_CONSTEXPR void on_minute(numeric_system) {}
  FMT_CONSTEXPR void on_second(numeric_system) {}
  FMT_CONSTEXPR void on_12_hour_time() {}
  FMT_CONSTEXPR void on_24_hour_time() {}
  FMT_CONSTEXPR void on_iso_time() {}
  FMT_CONSTEXPR void on_am_pm() {}
  FMT_CONSTEXPR void on_duration_value() {}
  FMT_CONSTEXPR void on_duration_unit() {}
};

template <typename T, FMT_ENABLE_IF(std::is_integral<T>::value)>
inline bool isnan(T) {
  return false;
}
template <typename T, FMT_ENABLE_IF(std::is_floating_point<T>::value)>
inline bool isnan(T value) {
  return std::isnan(value);
}

template <typename T, FMT_ENABLE_IF(std::is_integral<T>::value)>
inline bool isfinite(T) {
  return true;
}

// Converts value to int and checks that it's in the range [0, upper).
template <typename T, FMT_ENABLE_IF(std::is_integral<T>::value)>
inline int to_nonnegative_int(T value, int upper) {
  FMT_ASSERT(value >= 0 && to_unsigned(value) <= to_unsigned(upper),
             "invalid value");
  (void)upper;
  return static_cast<int>(value);
}
template <typename T, FMT_ENABLE_IF(!std::is_integral<T>::value)>
inline int to_nonnegative_int(T value, int upper) {
  FMT_ASSERT(
      std::isnan(value) || (value >= 0 && value <= static_cast<T>(upper)),
      "invalid value");
  (void)upper;
  return static_cast<int>(value);
}

template <typename T, FMT_ENABLE_IF(std::is_integral<T>::value)>
inline T mod(T x, int y) {
  return x % static_cast<T>(y);
}
template <typename T, FMT_ENABLE_IF(std::is_floating_point<T>::value)>
inline T mod(T x, int y) {
  return std::fmod(x, static_cast<T>(y));
}

// If T is an integral type, maps T to its unsigned counterpart, otherwise
// leaves it unchanged (unlike std::make_unsigned).
template <typename T, bool INTEGRAL = std::is_integral<T>::value>
struct make_unsigned_or_unchanged {
  using type = T;
};

template <typename T> struct make_unsigned_or_unchanged<T, true> {
  using type = typename std::make_unsigned<T>::type;
};

#if FMT_SAFE_DURATION_CAST
// throwing version of safe_duration_cast
template <typename To, typename FromRep, typename FromPeriod>
To fmt_safe_duration_cast(std::chrono::duration<FromRep, FromPeriod> from) {
  int ec;
  To to = safe_duration_cast::safe_duration_cast<To>(from, ec);
  if (ec) FMT_THROW(format_error("cannot format duration"));
  return to;
}
#endif

template <typename Rep, typename Period,
          FMT_ENABLE_IF(std::is_integral<Rep>::value)>
inline std::chrono::duration<Rep, std::milli> get_milliseconds(
    std::chrono::duration<Rep, Period> d) {
  // this may overflow and/or the result may not fit in the
  // target type.
#if FMT_SAFE_DURATION_CAST
  using CommonSecondsType =
      typename std::common_type<decltype(d), std::chrono::seconds>::type;
  const auto d_as_common = fmt_safe_duration_cast<CommonSecondsType>(d);
  const auto d_as_whole_seconds =
      fmt_safe_duration_cast<std::chrono::seconds>(d_as_common);
  // this conversion should be nonproblematic
  const auto diff = d_as_common - d_as_whole_seconds;
  const auto ms =
      fmt_safe_duration_cast<std::chrono::duration<Rep, std::milli>>(diff);
  return ms;
#else
  auto s = std::chrono::duration_cast<std::chrono::seconds>(d);
  return std::chrono::duration_cast<std::chrono::milliseconds>(d - s);
#endif
}

template <typename Rep, typename Period,
          FMT_ENABLE_IF(std::is_floating_point<Rep>::value)>
inline std::chrono::duration<Rep, std::milli> get_milliseconds(
    std::chrono::duration<Rep, Period> d) {
  using common_type = typename std::common_type<Rep, std::intmax_t>::type;
  auto ms = mod(d.count() * static_cast<common_type>(Period::num) /
                    static_cast<common_type>(Period::den) * 1000,
                1000);
  return std::chrono::duration<Rep, std::milli>(static_cast<Rep>(ms));
}

template <typename Char, typename Rep, typename OutputIt,
          FMT_ENABLE_IF(std::is_integral<Rep>::value)>
OutputIt format_duration_value(OutputIt out, Rep val, int) {
  return write<Char>(out, val);
}

template <typename Char, typename Rep, typename OutputIt,
          FMT_ENABLE_IF(std::is_floating_point<Rep>::value)>
OutputIt format_duration_value(OutputIt out, Rep val, int precision) {
  auto specs = basic_format_specs<Char>();
  specs.precision = precision;
  specs.type = precision >= 0 ? presentation_type::fixed_lower
                              : presentation_type::general_lower;
  return write<Char>(out, val, specs);
}

template <typename Char, typename OutputIt>
OutputIt copy_unit(string_view unit, OutputIt out, Char) {
  return std::copy(unit.begin(), unit.end(), out);
}

template <typename OutputIt>
OutputIt copy_unit(string_view unit, OutputIt out, wchar_t) {
  // This works when wchar_t is UTF-32 because units only contain characters
  // that have the same representation in UTF-16 and UTF-32.
  utf8_to_utf16 u(unit);
  return std::copy(u.c_str(), u.c_str() + u.size(), out);
}

template <typename Char, typename Period, typename OutputIt>
OutputIt format_duration_unit(OutputIt out) {
  if (const char* unit = get_units<Period>())
    return copy_unit(string_view(unit), out, Char());
  *out++ = '[';
  out = write<Char>(out, Period::num);
  if (const_check(Period::den != 1)) {
    *out++ = '/';
    out = write<Char>(out, Period::den);
  }
  *out++ = ']';
  *out++ = 's';
  return out;
}

class get_locale {
 private:
  union {
    std::locale locale_;
  };
  bool has_locale_ = false;

 public:
  get_locale(bool localized, locale_ref loc) : has_locale_(localized) {
    if (localized)
      ::new (&locale_) std::locale(loc.template get<std::locale>());
  }
  ~get_locale() {
    if (has_locale_) locale_.~locale();
  }
  operator const std::locale&() const {
    return has_locale_ ? locale_ : get_classic_locale();
  }
};

template <typename FormatContext, typename OutputIt, typename Rep,
          typename Period>
struct chrono_formatter {
  FormatContext& context;
  OutputIt out;
  int precision;
  bool localized = false;
  // rep is unsigned to avoid overflow.
  using rep =
      conditional_t<std::is_integral<Rep>::value && sizeof(Rep) < sizeof(int),
                    unsigned, typename make_unsigned_or_unchanged<Rep>::type>;
  rep val;
  using seconds = std::chrono::duration<rep>;
  seconds s;
  using milliseconds = std::chrono::duration<rep, std::milli>;
  bool negative;

  using char_type = typename FormatContext::char_type;
  using tm_writer_type = tm_writer<OutputIt, char_type>;

  chrono_formatter(FormatContext& ctx, OutputIt o,
                   std::chrono::duration<Rep, Period> d)
      : context(ctx),
        out(o),
        val(static_cast<rep>(d.count())),
        negative(false) {
    if (d.count() < 0) {
      val = 0 - val;
      negative = true;
    }

    // this may overflow and/or the result may not fit in the
    // target type.
#if FMT_SAFE_DURATION_CAST
    // might need checked conversion (rep!=Rep)
    auto tmpval = std::chrono::duration<rep, Period>(val);
    s = fmt_safe_duration_cast<seconds>(tmpval);
#else
    s = std::chrono::duration_cast<seconds>(
        std::chrono::duration<rep, Period>(val));
#endif
  }

  // returns true if nan or inf, writes to out.
  bool handle_nan_inf() {
    if (isfinite(val)) {
      return false;
    }
    if (isnan(val)) {
      write_nan();
      return true;
    }
    // must be +-inf
    if (val > 0) {
      write_pinf();
    } else {
      write_ninf();
    }
    return true;
  }

  Rep hour() const { return static_cast<Rep>(mod((s.count() / 3600), 24)); }

  Rep hour12() const {
    Rep hour = static_cast<Rep>(mod((s.count() / 3600), 12));
    return hour <= 0 ? 12 : hour;
  }

  Rep minute() const { return static_cast<Rep>(mod((s.count() / 60), 60)); }
  Rep second() const { return static_cast<Rep>(mod(s.count(), 60)); }

  std::tm time() const {
    auto time = std::tm();
    time.tm_hour = to_nonnegative_int(hour(), 24);
    time.tm_min = to_nonnegative_int(minute(), 60);
    time.tm_sec = to_nonnegative_int(second(), 60);
    return time;
  }

  void write_sign() {
    if (negative) {
      *out++ = '-';
      negative = false;
    }
  }

  void write(Rep value, int width) {
    write_sign();
    if (isnan(value)) return write_nan();
    uint32_or_64_or_128_t<int> n =
        to_unsigned(to_nonnegative_int(value, max_value<int>()));
    int num_digits = detail::count_digits(n);
    if (width > num_digits) out = std::fill_n(out, width - num_digits, '0');
    out = format_decimal<char_type>(out, n, num_digits).end;
  }

  void write_nan() { std::copy_n("nan", 3, out); }
  void write_pinf() { std::copy_n("inf", 3, out); }
  void write_ninf() { std::copy_n("-inf", 4, out); }

  template <typename Callback, typename... Args>
  void format_tm(const tm& time, Callback cb, Args... args) {
    if (isnan(val)) return write_nan();
    get_locale loc(localized, context.locale());
    auto w = tm_writer_type(loc, out, time);
    (w.*cb)(args...);
    out = w.out();
  }

  void on_text(const char_type* begin, const char_type* end) {
    std::copy(begin, end, out);
  }

  // These are not implemented because durations don't have date information.
  void on_abbr_weekday() {}
  void on_full_weekday() {}
  void on_dec0_weekday(numeric_system) {}
  void on_dec1_weekday(numeric_system) {}
  void on_abbr_month() {}
  void on_full_month() {}
  void on_datetime(numeric_system) {}
  void on_loc_date(numeric_system) {}
  void on_loc_time(numeric_system) {}
  void on_us_date() {}
  void on_iso_date() {}
  void on_utc_offset() {}
  void on_tz_name() {}
  void on_year(numeric_system) {}
  void on_short_year(numeric_system) {}
  void on_offset_year() {}
  void on_century(numeric_system) {}
  void on_iso_week_based_year() {}
  void on_iso_week_based_short_year() {}
  void on_dec_month(numeric_system) {}
  void on_dec0_week_of_year(numeric_system) {}
  void on_dec1_week_of_year(numeric_system) {}
  void on_iso_week_of_year(numeric_system) {}
  void on_day_of_year() {}
  void on_day_of_month(numeric_system) {}
  void on_day_of_month_space(numeric_system) {}

  void on_24_hour(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(hour(), 2);
    auto time = tm();
    time.tm_hour = to_nonnegative_int(hour(), 24);
    format_tm(time, &tm_writer_type::on_24_hour, ns);
  }

  void on_12_hour(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(hour12(), 2);
    auto time = tm();
    time.tm_hour = to_nonnegative_int(hour12(), 12);
    format_tm(time, &tm_writer_type::on_12_hour, ns);
  }

  void on_minute(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(minute(), 2);
    auto time = tm();
    time.tm_min = to_nonnegative_int(minute(), 60);
    format_tm(time, &tm_writer_type::on_minute, ns);
  }

  void on_second(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) {
      write(second(), 2);
#if FMT_SAFE_DURATION_CAST
      // convert rep->Rep
      using duration_rep = std::chrono::duration<rep, Period>;
      using duration_Rep = std::chrono::duration<Rep, Period>;
      auto tmpval = fmt_safe_duration_cast<duration_Rep>(duration_rep{val});
#else
      auto tmpval = std::chrono::duration<Rep, Period>(val);
#endif
      auto ms = get_milliseconds(tmpval);
      if (ms != std::chrono::milliseconds(0)) {
        *out++ = '.';
        write(ms.count(), 3);
      }
      return;
    }
    auto time = tm();
    time.tm_sec = to_nonnegative_int(second(), 60);
    format_tm(time, &tm_writer_type::on_second, ns);
  }

  void on_12_hour_time() {
    if (handle_nan_inf()) return;
    format_tm(time(), &tm_writer_type::on_12_hour_time);
  }

  void on_24_hour_time() {
    if (handle_nan_inf()) {
      *out++ = ':';
      handle_nan_inf();
      return;
    }

    write(hour(), 2);
    *out++ = ':';
    write(minute(), 2);
  }

  void on_iso_time() {
    on_24_hour_time();
    *out++ = ':';
    if (handle_nan_inf()) return;
    write(second(), 2);
  }

  void on_am_pm() {
    if (handle_nan_inf()) return;
    format_tm(time(), &tm_writer_type::on_am_pm);
  }

  void on_duration_value() {
    if (handle_nan_inf()) return;
    write_sign();
    out = format_duration_value<char_type>(out, val, precision);
  }

  void on_duration_unit() {
    out = format_duration_unit<char_type, Period>(out);
  }
};

FMT_END_DETAIL_NAMESPACE

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907
using weekday = std::chrono::weekday;
#else
// A fallback version of weekday.
class weekday {
 private:
  unsigned char value;

 public:
  weekday() = default;
  explicit constexpr weekday(unsigned wd) noexcept
      : value(static_cast<unsigned char>(wd != 7 ? wd : 0)) {}
  constexpr unsigned c_encoding() const noexcept { return value; }
};

class year_month_day {};
#endif

// A rudimentary weekday formatter.
template <typename Char> struct formatter<weekday, Char> {
 private:
  bool localized = false;

 public:
  FMT_CONSTEXPR auto parse(basic_format_parse_context<Char>& ctx)
      -> decltype(ctx.begin()) {
    auto begin = ctx.begin(), end = ctx.end();
    if (begin != end && *begin == 'L') {
      ++begin;
      localized = true;
    }
    return begin;
  }

  template <typename FormatContext>
  auto format(weekday wd, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto time = std::tm();
    time.tm_wday = static_cast<int>(wd.c_encoding());
    detail::get_locale loc(localized, ctx.locale());
    auto w = detail::tm_writer<decltype(ctx.out()), Char>(loc, ctx.out(), time);
    w.on_abbr_weekday();
    return w.out();
  }
};

template <typename Rep, typename Period, typename Char>
struct formatter<std::chrono::duration<Rep, Period>, Char> {
 private:
  basic_format_specs<Char> specs;
  int precision = -1;
  using arg_ref_type = detail::arg_ref<Char>;
  arg_ref_type width_ref;
  arg_ref_type precision_ref;
  bool localized = false;
  basic_string_view<Char> format_str;
  using duration = std::chrono::duration<Rep, Period>;

  struct spec_handler {
    formatter& f;
    basic_format_parse_context<Char>& context;
    basic_string_view<Char> format_str;

    template <typename Id> FMT_CONSTEXPR arg_ref_type make_arg_ref(Id arg_id) {
      context.check_arg_id(arg_id);
      return arg_ref_type(arg_id);
    }

    FMT_CONSTEXPR arg_ref_type make_arg_ref(basic_string_view<Char> arg_id) {
      context.check_arg_id(arg_id);
      return arg_ref_type(arg_id);
    }

    FMT_CONSTEXPR arg_ref_type make_arg_ref(detail::auto_id) {
      return arg_ref_type(context.next_arg_id());
    }

    void on_error(const char* msg) { FMT_THROW(format_error(msg)); }
    FMT_CONSTEXPR void on_fill(basic_string_view<Char> fill) {
      f.specs.fill = fill;
    }
    FMT_CONSTEXPR void on_align(align_t align) { f.specs.align = align; }
    FMT_CONSTEXPR void on_width(int width) { f.specs.width = width; }
    FMT_CONSTEXPR void on_precision(int _precision) {
      f.precision = _precision;
    }
    FMT_CONSTEXPR void end_precision() {}

    template <typename Id> FMT_CONSTEXPR void on_dynamic_width(Id arg_id) {
      f.width_ref = make_arg_ref(arg_id);
    }

    template <typename Id> FMT_CONSTEXPR void on_dynamic_precision(Id arg_id) {
      f.precision_ref = make_arg_ref(arg_id);
    }
  };

  using iterator = typename basic_format_parse_context<Char>::iterator;
  struct parse_range {
    iterator begin;
    iterator end;
  };

  FMT_CONSTEXPR parse_range do_parse(basic_format_parse_context<Char>& ctx) {
    auto begin = ctx.begin(), end = ctx.end();
    if (begin == end || *begin == '}') return {begin, begin};
    spec_handler handler{*this, ctx, format_str};
    begin = detail::parse_align(begin, end, handler);
    if (begin == end) return {begin, begin};
    begin = detail::parse_width(begin, end, handler);
    if (begin == end) return {begin, begin};
    if (*begin == '.') {
      if (std::is_floating_point<Rep>::value)
        begin = detail::parse_precision(begin, end, handler);
      else
        handler.on_error("precision not allowed for this argument type");
    }
    if (begin != end && *begin == 'L') {
      ++begin;
      localized = true;
    }
    end = detail::parse_chrono_format(begin, end,
                                      detail::chrono_format_checker());
    return {begin, end};
  }

 public:
  FMT_CONSTEXPR auto parse(basic_format_parse_context<Char>& ctx)
      -> decltype(ctx.begin()) {
    auto range = do_parse(ctx);
    format_str = basic_string_view<Char>(
        &*range.begin, detail::to_unsigned(range.end - range.begin));
    return range.end;
  }

  template <typename FormatContext>
  auto format(const duration& d, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto specs_copy = specs;
    auto precision_copy = precision;
    auto begin = format_str.begin(), end = format_str.end();
    // As a possible future optimization, we could avoid extra copying if width
    // is not specified.
    basic_memory_buffer<Char> buf;
    auto out = std::back_inserter(buf);
    detail::handle_dynamic_spec<detail::width_checker>(specs_copy.width,
                                                       width_ref, ctx);
    detail::handle_dynamic_spec<detail::precision_checker>(precision_copy,
                                                           precision_ref, ctx);
    if (begin == end || *begin == '}') {
      out = detail::format_duration_value<Char>(out, d.count(), precision_copy);
      detail::format_duration_unit<Char, Period>(out);
    } else {
      detail::chrono_formatter<FormatContext, decltype(out), Rep, Period> f(
          ctx, out, d);
      f.precision = precision_copy;
      f.localized = localized;
      detail::parse_chrono_format(begin, end, f);
    }
    return detail::write(
        ctx.out(), basic_string_view<Char>(buf.data(), buf.size()), specs_copy);
  }
};

template <typename Char, typename Duration>
struct formatter<std::chrono::time_point<std::chrono::system_clock, Duration>,
                 Char> : formatter<std::tm, Char> {
  FMT_CONSTEXPR formatter() {
    this->do_parse(default_specs,
                   default_specs + sizeof(default_specs) / sizeof(Char));
  }

  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return this->do_parse(ctx.begin(), ctx.end(), true);
  }

  template <typename FormatContext>
  auto format(std::chrono::time_point<std::chrono::system_clock> val,
              FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<std::tm, Char>::format(localtime(val), ctx);
  }

  static constexpr const Char default_specs[] = {'%', 'F', ' ', '%', 'T'};
};

template <typename Char, typename Duration>
constexpr const Char
    formatter<std::chrono::time_point<std::chrono::system_clock, Duration>,
              Char>::default_specs[];

template <typename Char> struct formatter<std::tm, Char> {
 private:
  enum class spec {
    unknown,
    year_month_day,
    hh_mm_ss,
  };
  spec spec_ = spec::unknown;
  basic_string_view<Char> specs;

 protected:
  template <typename It>
  FMT_CONSTEXPR auto do_parse(It begin, It end, bool with_default = false)
      -> It {
    if (begin != end && *begin == ':') ++begin;
    end = detail::parse_chrono_format(begin, end, detail::tm_format_checker());
    if (!with_default || end != begin)
      specs = {begin, detail::to_unsigned(end - begin)};
    // basic_string_view<>::compare isn't constexpr before C++17.
    if (specs.size() == 2 && specs[0] == Char('%')) {
      if (specs[1] == Char('F'))
        spec_ = spec::year_month_day;
      else if (specs[1] == Char('T'))
        spec_ = spec::hh_mm_ss;
    }
    return end;
  }

 public:
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return this->do_parse(ctx.begin(), ctx.end());
  }

  template <typename FormatContext>
  auto format(const std::tm& tm, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    const auto loc_ref = ctx.locale();
    detail::get_locale loc(static_cast<bool>(loc_ref), loc_ref);
    auto w = detail::tm_writer<decltype(ctx.out()), Char>(loc, ctx.out(), tm);
    if (spec_ == spec::year_month_day)
      w.on_iso_date();
    else if (spec_ == spec::hh_mm_ss)
      w.on_iso_time();
    else
      detail::parse_chrono_format(specs.begin(), specs.end(), w);
    return w.out();
  }
};

FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_CHRONO_H_
