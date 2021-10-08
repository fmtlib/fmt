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

  if (detail::const_check(!F::is_signed && T::is_signed && F::digits >= T::digits) &&
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

inline auto do_write(const std::tm& time, const std::locale& loc, char format,
                     char modifier) -> std::string {
  auto&& os = std::ostringstream();
  os.imbue(loc);
  using iterator = std::ostreambuf_iterator<char>;
  const auto& facet = std::use_facet<std::time_put<char, iterator>>(loc);
  auto end = facet.put(os, os, ' ', &time, format, modifier);
  if (end.failed()) FMT_THROW(format_error("failed to format time"));
  auto str = os.str();
  if (!detail::is_utf8() || loc == std::locale::classic()) return str;
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
  return str;
}

template <typename OutputIt>
auto write(OutputIt out, const std::tm& time, const std::locale& loc,
           char format, char modifier = 0) -> OutputIt {
  auto str = do_write(time, loc, format, modifier);
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

inline size_t strftime(char* str, size_t count, const char* format,
                       const std::tm* time) {
  // Assign to a pointer to suppress GCCs -Wformat-nonliteral
  // First assign the nullptr to suppress -Wsuggest-attribute=format
  std::size_t (*strftime)(char*, std::size_t, const char*, const std::tm*) =
      nullptr;
  strftime = std::strftime;
  return strftime(str, count, format, time);
}

inline size_t strftime(wchar_t* str, size_t count, const wchar_t* format,
                       const std::tm* time) {
  // See above
  std::size_t (*wcsftime)(wchar_t*, std::size_t, const wchar_t*,
                          const std::tm*) = nullptr;
  wcsftime = std::wcsftime;
  return wcsftime(str, count, format, time);
}

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

FMT_END_DETAIL_NAMESPACE

template <typename Char, typename Duration>
struct formatter<std::chrono::time_point<std::chrono::system_clock, Duration>,
                 Char> : formatter<std::tm, Char> {
  FMT_CONSTEXPR formatter() {
    this->specs = {default_specs, sizeof(default_specs) / sizeof(Char)};
  }

  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == ':') ++it;
    auto end = it;
    while (end != ctx.end() && *end != '}') ++end;
    if (end != it) this->specs = {it, detail::to_unsigned(end - it)};
    return end;
  }

  template <typename FormatContext>
  auto format(std::chrono::time_point<std::chrono::system_clock> val,
              FormatContext& ctx) -> decltype(ctx.out()) {
    std::tm time = localtime(val);
    return formatter<std::tm, Char>::format(time, ctx);
  }

  static constexpr Char default_specs[] = {'%', 'F', ' ', '%', 'T'};
};

template <typename Char, typename Duration>
constexpr Char
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

 public:
  basic_string_view<Char> specs;

  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == ':') ++it;
    auto end = it;
    while (end != ctx.end() && *end != '}') ++end;
    auto size = detail::to_unsigned(end - it);
    specs = {it, size};
    // basic_string_view<>::compare isn't constexpr before C++17
    if (specs.size() == 2 && specs[0] == Char('%')) {
      if (specs[1] == Char('F'))
        spec_ = spec::year_month_day;
      else if (specs[1] == Char('T'))
        spec_ = spec::hh_mm_ss;
    }
    return end;
  }

  template <typename FormatContext>
  auto format(const std::tm& tm, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto year = 1900 + tm.tm_year;
    if (spec_ == spec::year_month_day && year >= 0 && year < 10000) {
      char buf[10];
      detail::copy2(buf, detail::digits2(detail::to_unsigned(year / 100)));
      detail::write_digit2_separated(buf + 2, year % 100,
                                     detail::to_unsigned(tm.tm_mon + 1),
                                     detail::to_unsigned(tm.tm_mday), '-');
      return std::copy_n(buf, sizeof(buf), ctx.out());
    } else if (spec_ == spec::hh_mm_ss) {
      char buf[8];
      detail::write_digit2_separated(buf, detail::to_unsigned(tm.tm_hour),
                                     detail::to_unsigned(tm.tm_min),
                                     detail::to_unsigned(tm.tm_sec), ':');
      return std::copy_n(buf, sizeof(buf), ctx.out());
    }
    basic_memory_buffer<Char> tm_format;
    tm_format.append(specs.begin(), specs.end());
    // By appending an extra space we can distinguish an empty result that
    // indicates insufficient buffer size from a guaranteed non-empty result
    // https://github.com/fmtlib/fmt/issues/2238
    tm_format.push_back(' ');
    tm_format.push_back('\0');
    basic_memory_buffer<Char> buf;
    size_t start = buf.size();
    for (;;) {
      size_t size = buf.capacity() - start;
      size_t count = detail::strftime(&buf[start], size, &tm_format[0], &tm);
      if (count != 0) {
        buf.resize(start + count);
        break;
      }
      const size_t MIN_GROWTH = 10;
      buf.reserve(buf.capacity() + (size > MIN_GROWTH ? size : MIN_GROWTH));
    }
    // Remove the extra space.
    return std::copy(buf.begin(), buf.end() - 1, ctx.out());
  }
};

FMT_BEGIN_DETAIL_NAMESPACE

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
  FMT_CONSTEXPR void on_abbr_weekday() { unsupported(); }
  FMT_CONSTEXPR void on_full_weekday() { unsupported(); }
  FMT_CONSTEXPR void on_dec0_weekday(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_dec1_weekday(numeric_system) { unsupported(); }
  FMT_CONSTEXPR void on_abbr_month() { unsupported(); }
  FMT_CONSTEXPR void on_full_month() { unsupported(); }
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
  specs.type = precision > 0 ? presentation_type::fixed_lower
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

  explicit chrono_formatter(FormatContext& ctx, OutputIt o,
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

  void format_localized(const tm& time, char format, char modifier = 0) {
    if (isnan(val)) return write_nan();
    const auto& loc = localized ? context.locale().template get<std::locale>()
                                : std::locale::classic();
    out = detail::write(out, time, loc, format, modifier);
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

  void on_24_hour(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(hour(), 2);
    auto time = tm();
    time.tm_hour = to_nonnegative_int(hour(), 24);
    format_localized(time, 'H', 'O');
  }

  void on_12_hour(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(hour12(), 2);
    auto time = tm();
    time.tm_hour = to_nonnegative_int(hour12(), 12);
    format_localized(time, 'I', 'O');
  }

  void on_minute(numeric_system ns) {
    if (handle_nan_inf()) return;

    if (ns == numeric_system::standard) return write(minute(), 2);
    auto time = tm();
    time.tm_min = to_nonnegative_int(minute(), 60);
    format_localized(time, 'M', 'O');
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
    format_localized(time, 'S', 'O');
  }

  void on_12_hour_time() {
    if (handle_nan_inf()) return;
    format_localized(time(), 'r');
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
    format_localized(time(), 'p');
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
template <> struct formatter<weekday> {
 private:
  bool localized = false;

 public:
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    auto begin = ctx.begin(), end = ctx.end();
    if (begin != end && *begin == 'L') {
      ++begin;
      localized = true;
    }
    return begin;
  }

  auto format(weekday wd, format_context& ctx) -> decltype(ctx.out()) {
    auto time = std::tm();
    time.tm_wday = static_cast<int>(wd.c_encoding());
    const auto& loc = localized ? ctx.locale().template get<std::locale>()
                                : std::locale::classic();
    return detail::write(ctx.out(), time, loc, 'a');
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
    end = parse_chrono_format(begin, end, detail::chrono_format_checker());
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

FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_CHRONO_H_
