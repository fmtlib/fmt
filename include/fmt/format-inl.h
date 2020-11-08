// Formatting library for C++ - implementation
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_FORMAT_INL_H_
#define FMT_FORMAT_INL_H_

#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstring>  // std::memmove
#include <cwchar>
#include <exception>

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
#  include <locale>
#endif

#ifdef _WIN32
#  include <io.h>  // _isatty
#endif

#include "format.h"

// Dummy implementations of strerror_r and strerror_s called if corresponding
// system functions are not available.
inline fmt::detail::null<> strerror_r(int, char*, ...) { return {}; }
inline fmt::detail::null<> strerror_s(char*, size_t, ...) { return {}; }

FMT_BEGIN_NAMESPACE
namespace detail {

FMT_FUNC void assert_fail(const char* file, int line, const char* message) {
  // Use unchecked std::fprintf to avoid triggering another assertion when
  // writing to stderr fails
  std::fprintf(stderr, "%s:%d: assertion failed: %s", file, line, message);
  // Chosen instead of std::abort to satisfy Clang in CUDA mode during device
  // code pass.
  std::terminate();
}

#ifndef _MSC_VER
#  define FMT_SNPRINTF snprintf
#else  // _MSC_VER
inline int fmt_snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(buffer, size, _TRUNCATE, format, args);
  va_end(args);
  return result;
}
#  define FMT_SNPRINTF fmt_snprintf
#endif  // _MSC_VER

// A portable thread-safe version of strerror.
// Sets buffer to point to a string describing the error code.
// This can be either a pointer to a string stored in buffer,
// or a pointer to some static immutable string.
// Returns one of the following values:
//   0      - success
//   ERANGE - buffer is not large enough to store the error message
//   other  - failure
// Buffer should be at least of size 1.
inline int safe_strerror(int error_code, char*& buffer,
                         size_t buffer_size) FMT_NOEXCEPT {
  FMT_ASSERT(buffer != nullptr && buffer_size != 0, "invalid buffer");

  class dispatcher {
   private:
    int error_code_;
    char*& buffer_;
    size_t buffer_size_;

    // A noop assignment operator to avoid bogus warnings.
    void operator=(const dispatcher&) {}

    // Handle the result of XSI-compliant version of strerror_r.
    int handle(int result) {
      // glibc versions before 2.13 return result in errno.
      return result == -1 ? errno : result;
    }

    // Handle the result of GNU-specific version of strerror_r.
    FMT_MAYBE_UNUSED
    int handle(char* message) {
      // If the buffer is full then the message is probably truncated.
      if (message == buffer_ && strlen(buffer_) == buffer_size_ - 1)
        return ERANGE;
      buffer_ = message;
      return 0;
    }

    // Handle the case when strerror_r is not available.
    FMT_MAYBE_UNUSED
    int handle(detail::null<>) {
      return fallback(strerror_s(buffer_, buffer_size_, error_code_));
    }

    // Fallback to strerror_s when strerror_r is not available.
    FMT_MAYBE_UNUSED
    int fallback(int result) {
      // If the buffer is full then the message is probably truncated.
      return result == 0 && strlen(buffer_) == buffer_size_ - 1 ? ERANGE
                                                                : result;
    }

#if !FMT_MSC_VER
    // Fallback to strerror if strerror_r and strerror_s are not available.
    int fallback(detail::null<>) {
      errno = 0;
      buffer_ = strerror(error_code_);
      return errno;
    }
#endif

   public:
    dispatcher(int err_code, char*& buf, size_t buf_size)
        : error_code_(err_code), buffer_(buf), buffer_size_(buf_size) {}

    int run() { return handle(strerror_r(error_code_, buffer_, buffer_size_)); }
  };
  return dispatcher(error_code, buffer, buffer_size).run();
}

FMT_FUNC void format_error_code(detail::buffer<char>& out, int error_code,
                                string_view message) FMT_NOEXCEPT {
  // Report error code making sure that the output fits into
  // inline_buffer_size to avoid dynamic memory allocation and potential
  // bad_alloc.
  out.try_resize(0);
  static const char SEP[] = ": ";
  static const char ERROR_STR[] = "error ";
  // Subtract 2 to account for terminating null characters in SEP and ERROR_STR.
  size_t error_code_size = sizeof(SEP) + sizeof(ERROR_STR) - 2;
  auto abs_value = static_cast<uint32_or_64_or_128_t<int>>(error_code);
  if (detail::is_negative(error_code)) {
    abs_value = 0 - abs_value;
    ++error_code_size;
  }
  error_code_size += detail::to_unsigned(detail::count_digits(abs_value));
  auto it = buffer_appender<char>(out);
  if (message.size() <= inline_buffer_size - error_code_size)
    format_to(it, "{}{}", message, SEP);
  format_to(it, "{}{}", ERROR_STR, error_code);
  assert(out.size() <= inline_buffer_size);
}

FMT_FUNC void report_error(format_func func, int error_code,
                           string_view message) FMT_NOEXCEPT {
  memory_buffer full_message;
  func(full_message, error_code, message);
  // Don't use fwrite_fully because the latter may throw.
  (void)std::fwrite(full_message.data(), full_message.size(), 1, stderr);
  std::fputc('\n', stderr);
}

// A wrapper around fwrite that throws on error.
inline void fwrite_fully(const void* ptr, size_t size, size_t count,
                         FILE* stream) {
  size_t written = std::fwrite(ptr, size, count, stream);
  if (written < count) FMT_THROW(system_error(errno, "cannot write to file"));
}
}  // namespace detail

#if !defined(FMT_STATIC_THOUSANDS_SEPARATOR)
namespace detail {

template <typename Locale>
FMT_CTF_CONSTEXPR locale_ref::locale_ref(const Locale& loc) : locale_(&loc) {
  static_assert(std::is_same<Locale, std::locale>::value, "");
}

template <typename Locale> FMT_CTF_CONSTEXPR Locale locale_ref::get() const {
  static_assert(std::is_same<Locale, std::locale>::value, "");
  return locale_ ? *static_cast<const std::locale*>(locale_) : std::locale();
}

template <typename Char> FMT_FUNC std::string grouping_impl(locale_ref loc) {
  return std::use_facet<std::numpunct<Char>>(loc.get<std::locale>()).grouping();
}
template <typename Char> FMT_FUNC Char thousands_sep_impl(locale_ref loc) {
  return std::use_facet<std::numpunct<Char>>(loc.get<std::locale>())
      .thousands_sep();
}
template <typename Char> FMT_FUNC Char decimal_point_impl(locale_ref loc) {
  return std::use_facet<std::numpunct<Char>>(loc.get<std::locale>())
      .decimal_point();
}
}  // namespace detail
#else
template <typename Char>
FMT_FUNC std::string detail::grouping_impl(locale_ref) {
  return "\03";
}
template <typename Char> FMT_FUNC Char detail::thousands_sep_impl(locale_ref) {
  return FMT_STATIC_THOUSANDS_SEPARATOR;
}
template <typename Char> FMT_FUNC Char detail::decimal_point_impl(locale_ref) {
  return '.';
}
#endif

FMT_API FMT_FUNC format_error::~format_error() FMT_NOEXCEPT = default;
FMT_API FMT_FUNC system_error::~system_error() FMT_NOEXCEPT = default;

FMT_FUNC void system_error::init(int err_code, string_view format_str,
                                 format_args args) {
  error_code_ = err_code;
  memory_buffer buffer;
  format_system_error(buffer, err_code, vformat(format_str, args));
  std::runtime_error& base = *this;
  base = std::runtime_error(to_string(buffer));
}

namespace detail {

template <> FMT_FUNC int count_digits<4>(detail::fallback_uintptr n) {
  // fallback_uintptr is always stored in little endian.
  int i = static_cast<int>(sizeof(void*)) - 1;
  while (i > 0 && n.value[i] == 0) --i;
  auto char_digits = std::numeric_limits<unsigned char>::digits / 4;
  return i >= 0 ? i * char_digits + count_digits<4, unsigned>(n.value[i]) : 1;
}

template <typename T>
const data_digit_pair basic_data<T>::digits[] =
    FMT_DATA_DIGITS;

template <typename T>
const char basic_data<T>::hex_digits[] = FMT_DATA_HEX_DIGITS;

template <typename T>
const uint64_t basic_data<T>::powers_of_10_64[] = FMT_DATA_POWERS_OF_10_64;

template <typename T>
const uint32_t basic_data<T>::zero_or_powers_of_10_32[] = {0,
                                                           FMT_POWERS_OF_10(1)};
template <typename T>
const uint64_t basic_data<T>::zero_or_powers_of_10_64[] = {
    0, FMT_POWERS_OF_10(1), FMT_POWERS_OF_10(1000000000ULL),
    10000000000000000000ULL};

template <typename T>
const uint32_t basic_data<T>::zero_or_powers_of_10_32_new[] =
    FMT_DATA_ZERO_OR_POWERS_OF_10_32_NEW;

template <typename T>
const uint64_t basic_data<T>::zero_or_powers_of_10_64_new[] =
    FMT_DATA_ZERO_OR_POWERS_OF_10_64_NEW;

template <typename T>
const uint64_t basic_data<T>::grisu_pow10_significands[] =
    FMT_DATA_GRISU_POW10_SIGNIFICANDS;

template <typename T>
const int16_t basic_data<T>::grisu_pow10_exponents[] =
    FMT_DATA_GRISU_POW10_EXPONENTS;

template <typename T>
const divtest_table_entry<uint32_t> basic_data<T>::divtest_table_for_pow5_32[] =
    FMT_DATA_DIVTEST_TABLE_FOR_POW5_32;

template <typename T>
const divtest_table_entry<uint64_t> basic_data<T>::divtest_table_for_pow5_64[] =
    FMT_DATA_DIVTEST_TABLE_FOR_POW5_64;

template <typename T>
const uint64_t basic_data<T>::dragonbox_pow10_significands_64[] =
    FMT_DATA_DRAGONBOX_POW10_SIGNIFICANDS_64;

template <typename T>
const uint128_wrapper basic_data<T>::dragonbox_pow10_significands_128[] =
    FMT_DATA_DRAGONBOX_POW10_SIGNIFICANDS_128;

#if !FMT_USE_FULL_CACHE_DRAGONBOX
template <typename T>
const uint64_t basic_data<T>::powers_of_5_64[] = FMT_DATA_POWERS_OF_5_64;

template <typename T>
const uint32_t basic_data<T>::dragonbox_pow10_recovery_errors[] =
    FMT_DATA_DRAGONBOX_POW10_RECOVERY_ERRORS;
#endif

template <typename T>
const char basic_data<T>::foreground_color[] = FMT_DATA_FOREGROUND_COLOR;
template <typename T>
const char basic_data<T>::background_color[] = FMT_DATA_BACKGROUND_COLOR;
template <typename T>
const char basic_data<T>::reset_color[] = FMT_DATA_RESET_COLOR;
template <typename T>
const wchar_t basic_data<T>::wreset_color[] = FMT_DATA_WRESET_COLOR;
template <typename T> const char basic_data<T>::signs[] = FMT_DATA_SIGNS;
template <typename T>
const char basic_data<T>::left_padding_shifts[] = FMT_DATA_LEFT_PADDING_SHIFTS;
template <typename T>
const char basic_data<T>::right_padding_shifts[] =
    FMT_DATA_RIGHT_PADDING_SHIFTS;

template <typename T> struct bits {
  static FMT_CONSTEXPR_DECL const int value =
      static_cast<int>(sizeof(T) * std::numeric_limits<unsigned char>::digits);
};

class fp;
template <int SHIFT = 0> fp normalize(fp value);

// Lower (upper) boundary is a value half way between a floating-point value
// and its predecessor (successor). Boundaries have the same exponent as the
// value so only significands are stored.
struct boundaries {
  uint64_t lower;
  uint64_t upper;
};

// A handmade floating-point number f * pow(2, e).
class fp {
 private:
  using significand_type = uint64_t;

  template <typename Float>
  using is_supported_float = bool_constant<sizeof(Float) == sizeof(uint64_t) ||
                                           sizeof(Float) == sizeof(uint32_t)>;

 public:
  significand_type f;
  int e;

  // All sizes are in bits.
  // Subtract 1 to account for an implicit most significant bit in the
  // normalized form.
  static FMT_CONSTEXPR_DECL const int double_significand_size =
      std::numeric_limits<double>::digits - 1;
  static FMT_CONSTEXPR_DECL const uint64_t implicit_bit =
      1ULL << double_significand_size;
  static FMT_CONSTEXPR_DECL const int significand_size =
      bits<significand_type>::value;

  fp() : f(0), e(0) {}
  fp(uint64_t f_val, int e_val) : f(f_val), e(e_val) {}

  // Constructs fp from an IEEE754 double. It is a template to prevent compile
  // errors on platforms where double is not IEEE754.
  template <typename Double> explicit fp(Double d) { assign(d); }

  // Assigns d to this and return true iff predecessor is closer than successor.
  template <typename Float, FMT_ENABLE_IF(is_supported_float<Float>::value)>
  bool assign(Float d) {
    // Assume float is in the format [sign][exponent][significand].
    using limits = std::numeric_limits<Float>;
    const int float_significand_size = limits::digits - 1;
    const int exponent_size =
        bits<Float>::value - float_significand_size - 1;  // -1 for sign
    const uint64_t float_implicit_bit = 1ULL << float_significand_size;
    const uint64_t significand_mask = float_implicit_bit - 1;
    const uint64_t exponent_mask = (~0ULL >> 1) & ~significand_mask;
    const int exponent_bias = (1 << exponent_size) - limits::max_exponent - 1;
    constexpr bool is_double = sizeof(Float) == sizeof(uint64_t);
    auto u = bit_cast<conditional_t<is_double, uint64_t, uint32_t>>(d);
    f = u & significand_mask;
    int biased_e =
        static_cast<int>((u & exponent_mask) >> float_significand_size);
    // Predecessor is closer if d is a normalized power of 2 (f == 0) other than
    // the smallest normalized number (biased_e > 1).
    bool is_predecessor_closer = f == 0 && biased_e > 1;
    if (biased_e != 0)
      f += float_implicit_bit;
    else
      biased_e = 1;  // Subnormals use biased exponent 1 (min exponent).
    e = biased_e - exponent_bias - float_significand_size;
    return is_predecessor_closer;
  }

  template <typename Float, FMT_ENABLE_IF(!is_supported_float<Float>::value)>
  bool assign(Float) {
    *this = fp();
    return false;
  }
};

// Normalizes the value converted from double and multiplied by (1 << SHIFT).
template <int SHIFT> fp normalize(fp value) {
  // Handle subnormals.
  const auto shifted_implicit_bit = fp::implicit_bit << SHIFT;
  while ((value.f & shifted_implicit_bit) == 0) {
    value.f <<= 1;
    --value.e;
  }
  // Subtract 1 to account for hidden bit.
  const auto offset =
      fp::significand_size - fp::double_significand_size - SHIFT - 1;
  value.f <<= offset;
  value.e -= offset;
  return value;
}

inline bool operator==(fp x, fp y) { return x.f == y.f && x.e == y.e; }

// Computes lhs * rhs / pow(2, 64) rounded to nearest with half-up tie breaking.
inline uint64_t multiply(uint64_t lhs, uint64_t rhs) {
#if FMT_USE_INT128
  auto product = static_cast<__uint128_t>(lhs) * rhs;
  auto f = static_cast<uint64_t>(product >> 64);
  return (static_cast<uint64_t>(product) & (1ULL << 63)) != 0 ? f + 1 : f;
#else
  // Multiply 32-bit parts of significands.
  uint64_t mask = (1ULL << 32) - 1;
  uint64_t a = lhs >> 32, b = lhs & mask;
  uint64_t c = rhs >> 32, d = rhs & mask;
  uint64_t ac = a * c, bc = b * c, ad = a * d, bd = b * d;
  // Compute mid 64-bit of result and round.
  uint64_t mid = (bd >> 32) + (ad & mask) + (bc & mask) + (1U << 31);
  return ac + (ad >> 32) + (bc >> 32) + (mid >> 32);
#endif
}

inline fp operator*(fp x, fp y) { return {multiply(x.f, y.f), x.e + y.e + 64}; }

// Returns a cached power of 10 `c_k = c_k.f * pow(2, c_k.e)` such that its
// (binary) exponent satisfies `min_exponent <= c_k.e <= min_exponent + 28`.
inline fp get_cached_power(int min_exponent, int& pow10_exponent) {
  const int shift = 32;
  const auto significand = static_cast<int64_t>(data::log10_2_significand);
  int index = static_cast<int>(
      ((min_exponent + fp::significand_size - 1) * (significand >> shift) +
       ((int64_t(1) << shift) - 1))  // ceil
      >> 32                          // arithmetic shift
  );
  // Decimal exponent of the first (smallest) cached power of 10.
  const int first_dec_exp = -348;
  // Difference between 2 consecutive decimal exponents in cached powers of 10.
  const int dec_exp_step = 8;
  index = (index - first_dec_exp - 1) / dec_exp_step + 1;
  pow10_exponent = first_dec_exp + index * dec_exp_step;
  return {data::grisu_pow10_significands[index],
          data::grisu_pow10_exponents[index]};
}

// A simple accumulator to hold the sums of terms in bigint::square if uint128_t
// is not available.
struct accumulator {
  uint64_t lower;
  uint64_t upper;

  accumulator() : lower(0), upper(0) {}
  explicit operator uint32_t() const { return static_cast<uint32_t>(lower); }

  void operator+=(uint64_t n) {
    lower += n;
    if (lower < n) ++upper;
  }
  void operator>>=(int shift) {
    assert(shift == 32);
    (void)shift;
    lower = (upper << 32) | (lower >> 32);
    upper >>= 32;
  }
};

class bigint {
 private:
  // A bigint is stored as an array of bigits (big digits), with bigit at index
  // 0 being the least significant one.
  using bigit = uint32_t;
  using double_bigit = uint64_t;
  enum { bigits_capacity = 32 };
  basic_memory_buffer<bigit, bigits_capacity> bigits_;
  int exp_;

  bigit operator[](int index) const { return bigits_[to_unsigned(index)]; }
  bigit& operator[](int index) { return bigits_[to_unsigned(index)]; }

  static FMT_CONSTEXPR_DECL const int bigit_bits = bits<bigit>::value;

  friend struct formatter<bigint>;

  void subtract_bigits(int index, bigit other, bigit& borrow) {
    auto result = static_cast<double_bigit>((*this)[index]) - other - borrow;
    (*this)[index] = static_cast<bigit>(result);
    borrow = static_cast<bigit>(result >> (bigit_bits * 2 - 1));
  }

  void remove_leading_zeros() {
    int num_bigits = static_cast<int>(bigits_.size()) - 1;
    while (num_bigits > 0 && (*this)[num_bigits] == 0) --num_bigits;
    bigits_.resize(to_unsigned(num_bigits + 1));
  }

  // Computes *this -= other assuming aligned bigints and *this >= other.
  void subtract_aligned(const bigint& other) {
    FMT_ASSERT(other.exp_ >= exp_, "unaligned bigints");
    FMT_ASSERT(compare(*this, other) >= 0, "");
    bigit borrow = 0;
    int i = other.exp_ - exp_;
    for (size_t j = 0, n = other.bigits_.size(); j != n; ++i, ++j)
      subtract_bigits(i, other.bigits_[j], borrow);
    while (borrow > 0) subtract_bigits(i, 0, borrow);
    remove_leading_zeros();
  }

  void multiply(uint32_t value) {
    const double_bigit wide_value = value;
    bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      double_bigit result = bigits_[i] * wide_value + carry;
      bigits_[i] = static_cast<bigit>(result);
      carry = static_cast<bigit>(result >> bigit_bits);
    }
    if (carry != 0) bigits_.push_back(carry);
  }

  void multiply(uint64_t value) {
    const bigit mask = ~bigit(0);
    const double_bigit lower = value & mask;
    const double_bigit upper = value >> bigit_bits;
    double_bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      double_bigit result = bigits_[i] * lower + (carry & mask);
      carry =
          bigits_[i] * upper + (result >> bigit_bits) + (carry >> bigit_bits);
      bigits_[i] = static_cast<bigit>(result);
    }
    while (carry != 0) {
      bigits_.push_back(carry & mask);
      carry >>= bigit_bits;
    }
  }

 public:
  bigint() : exp_(0) {}
  explicit bigint(uint64_t n) { assign(n); }
  ~bigint() { assert(bigits_.capacity() <= bigits_capacity); }

  bigint(const bigint&) = delete;
  void operator=(const bigint&) = delete;

  void assign(const bigint& other) {
    auto size = other.bigits_.size();
    bigits_.resize(size);
    auto data = other.bigits_.data();
    std::copy(data, data + size, make_checked(bigits_.data(), size));
    exp_ = other.exp_;
  }

  void assign(uint64_t n) {
    size_t num_bigits = 0;
    do {
      bigits_[num_bigits++] = n & ~bigit(0);
      n >>= bigit_bits;
    } while (n != 0);
    bigits_.resize(num_bigits);
    exp_ = 0;
  }

  int num_bigits() const { return static_cast<int>(bigits_.size()) + exp_; }

  FMT_NOINLINE bigint& operator<<=(int shift) {
    assert(shift >= 0);
    exp_ += shift / bigit_bits;
    shift %= bigit_bits;
    if (shift == 0) return *this;
    bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      bigit c = bigits_[i] >> (bigit_bits - shift);
      bigits_[i] = (bigits_[i] << shift) + carry;
      carry = c;
    }
    if (carry != 0) bigits_.push_back(carry);
    return *this;
  }

  template <typename Int> bigint& operator*=(Int value) {
    FMT_ASSERT(value > 0, "");
    multiply(uint32_or_64_or_128_t<Int>(value));
    return *this;
  }

  friend int compare(const bigint& lhs, const bigint& rhs) {
    int num_lhs_bigits = lhs.num_bigits(), num_rhs_bigits = rhs.num_bigits();
    if (num_lhs_bigits != num_rhs_bigits)
      return num_lhs_bigits > num_rhs_bigits ? 1 : -1;
    int i = static_cast<int>(lhs.bigits_.size()) - 1;
    int j = static_cast<int>(rhs.bigits_.size()) - 1;
    int end = i - j;
    if (end < 0) end = 0;
    for (; i >= end; --i, --j) {
      bigit lhs_bigit = lhs[i], rhs_bigit = rhs[j];
      if (lhs_bigit != rhs_bigit) return lhs_bigit > rhs_bigit ? 1 : -1;
    }
    if (i != j) return i > j ? 1 : -1;
    return 0;
  }

  // Returns compare(lhs1 + lhs2, rhs).
  friend int add_compare(const bigint& lhs1, const bigint& lhs2,
                         const bigint& rhs) {
    int max_lhs_bigits = (std::max)(lhs1.num_bigits(), lhs2.num_bigits());
    int num_rhs_bigits = rhs.num_bigits();
    if (max_lhs_bigits + 1 < num_rhs_bigits) return -1;
    if (max_lhs_bigits > num_rhs_bigits) return 1;
    auto get_bigit = [](const bigint& n, int i) -> bigit {
      return i >= n.exp_ && i < n.num_bigits() ? n[i - n.exp_] : 0;
    };
    double_bigit borrow = 0;
    int min_exp = (std::min)((std::min)(lhs1.exp_, lhs2.exp_), rhs.exp_);
    for (int i = num_rhs_bigits - 1; i >= min_exp; --i) {
      double_bigit sum =
          static_cast<double_bigit>(get_bigit(lhs1, i)) + get_bigit(lhs2, i);
      bigit rhs_bigit = get_bigit(rhs, i);
      if (sum > rhs_bigit + borrow) return 1;
      borrow = rhs_bigit + borrow - sum;
      if (borrow > 1) return -1;
      borrow <<= bigit_bits;
    }
    return borrow != 0 ? -1 : 0;
  }

  // Assigns pow(10, exp) to this bigint.
  void assign_pow10(int exp) {
    assert(exp >= 0);
    if (exp == 0) return assign(1);
    // Find the top bit.
    int bitmask = 1;
    while (exp >= bitmask) bitmask <<= 1;
    bitmask >>= 1;
    // pow(10, exp) = pow(5, exp) * pow(2, exp). First compute pow(5, exp) by
    // repeated squaring and multiplication.
    assign(5);
    bitmask >>= 1;
    while (bitmask != 0) {
      square();
      if ((exp & bitmask) != 0) *this *= 5;
      bitmask >>= 1;
    }
    *this <<= exp;  // Multiply by pow(2, exp) by shifting.
  }

  void square() {
    basic_memory_buffer<bigit, bigits_capacity> n(std::move(bigits_));
    int num_bigits = static_cast<int>(bigits_.size());
    int num_result_bigits = 2 * num_bigits;
    bigits_.resize(to_unsigned(num_result_bigits));
    using accumulator_t = conditional_t<FMT_USE_INT128, uint128_t, accumulator>;
    auto sum = accumulator_t();
    for (int bigit_index = 0; bigit_index < num_bigits; ++bigit_index) {
      // Compute bigit at position bigit_index of the result by adding
      // cross-product terms n[i] * n[j] such that i + j == bigit_index.
      for (int i = 0, j = bigit_index; j >= 0; ++i, --j) {
        // Most terms are multiplied twice which can be optimized in the future.
        sum += static_cast<double_bigit>(n[i]) * n[j];
      }
      (*this)[bigit_index] = static_cast<bigit>(sum);
      sum >>= bits<bigit>::value;  // Compute the carry.
    }
    // Do the same for the top half.
    for (int bigit_index = num_bigits; bigit_index < num_result_bigits;
         ++bigit_index) {
      for (int j = num_bigits - 1, i = bigit_index - j; i < num_bigits;)
        sum += static_cast<double_bigit>(n[i++]) * n[j--];
      (*this)[bigit_index] = static_cast<bigit>(sum);
      sum >>= bits<bigit>::value;
    }
    --num_result_bigits;
    remove_leading_zeros();
    exp_ *= 2;
  }

  // If this bigint has a bigger exponent than other, adds trailing zero to make
  // exponents equal. This simplifies some operations such as subtraction.
  void align(const bigint& other) {
    int exp_difference = exp_ - other.exp_;
    if (exp_difference <= 0) return;
    int num_bigits = static_cast<int>(bigits_.size());
    bigits_.resize(to_unsigned(num_bigits + exp_difference));
    for (int i = num_bigits - 1, j = i + exp_difference; i >= 0; --i, --j)
      bigits_[j] = bigits_[i];
    std::uninitialized_fill_n(bigits_.data(), exp_difference, 0);
    exp_ -= exp_difference;
  }

  // Divides this bignum by divisor, assigning the remainder to this and
  // returning the quotient.
  int divmod_assign(const bigint& divisor) {
    FMT_ASSERT(this != &divisor, "");
    if (compare(*this, divisor) < 0) return 0;
    FMT_ASSERT(divisor.bigits_[divisor.bigits_.size() - 1u] != 0, "");
    align(divisor);
    int quotient = 0;
    do {
      subtract_aligned(divisor);
      ++quotient;
    } while (compare(*this, divisor) >= 0);
    return quotient;
  }
};

enum class round_direction { unknown, up, down };

// Given the divisor (normally a power of 10), the remainder = v % divisor for
// some number v and the error, returns whether v should be rounded up, down, or
// whether the rounding direction can't be determined due to error.
// error should be less than divisor / 2.
inline round_direction get_round_direction(uint64_t divisor, uint64_t remainder,
                                           uint64_t error) {
  FMT_ASSERT(remainder < divisor, "");  // divisor - remainder won't overflow.
  FMT_ASSERT(error < divisor, "");      // divisor - error won't overflow.
  FMT_ASSERT(error < divisor - error, "");  // error * 2 won't overflow.
  // Round down if (remainder + error) * 2 <= divisor.
  if (remainder <= divisor - remainder && error * 2 <= divisor - remainder * 2)
    return round_direction::down;
  // Round up if (remainder - error) * 2 >= divisor.
  if (remainder >= error &&
      remainder - error >= divisor - (remainder - error)) {
    return round_direction::up;
  }
  return round_direction::unknown;
}

namespace digits {
enum result {
  more,  // Generate more digits.
  done,  // Done generating digits.
  error  // Digit generation cancelled due to an error.
};
}

// Generates output using the Grisu digit-gen algorithm.
// error: the size of the region (lower, upper) outside of which numbers
// definitely do not round to value (Delta in Grisu3).
template <typename Handler>
FMT_ALWAYS_INLINE digits::result grisu_gen_digits(fp value, uint64_t error,
                                                  int& exp, Handler& handler) {
  const fp one(1ULL << -value.e, value.e);
  // The integral part of scaled value (p1 in Grisu) = value / one. It cannot be
  // zero because it contains a product of two 64-bit numbers with MSB set (due
  // to normalization) - 1, shifted right by at most 60 bits.
  auto integral = static_cast<uint32_t>(value.f >> -one.e);
  FMT_ASSERT(integral != 0, "");
  FMT_ASSERT(integral == value.f >> -one.e, "");
  // The fractional part of scaled value (p2 in Grisu) c = value % one.
  uint64_t fractional = value.f & (one.f - 1);
  exp = count_digits(integral);  // kappa in Grisu.
  // Divide by 10 to prevent overflow.
  auto result = handler.on_start(data::powers_of_10_64[exp - 1] << -one.e,
                                 value.f / 10, error * 10, exp);
  if (result != digits::more) return result;
  // Generate digits for the integral part. This can produce up to 10 digits.
  do {
    uint32_t digit = 0;
    auto divmod_integral = [&](uint32_t divisor) {
      digit = integral / divisor;
      integral %= divisor;
    };
    // This optimization by Milo Yip reduces the number of integer divisions by
    // one per iteration.
    switch (exp) {
    case 10:
      divmod_integral(1000000000);
      break;
    case 9:
      divmod_integral(100000000);
      break;
    case 8:
      divmod_integral(10000000);
      break;
    case 7:
      divmod_integral(1000000);
      break;
    case 6:
      divmod_integral(100000);
      break;
    case 5:
      divmod_integral(10000);
      break;
    case 4:
      divmod_integral(1000);
      break;
    case 3:
      divmod_integral(100);
      break;
    case 2:
      divmod_integral(10);
      break;
    case 1:
      digit = integral;
      integral = 0;
      break;
    default:
      FMT_ASSERT(false, "invalid number of digits");
    }
    --exp;
    auto remainder = (static_cast<uint64_t>(integral) << -one.e) + fractional;
    result = handler.on_digit(static_cast<char>('0' + digit),
                              data::powers_of_10_64[exp] << -one.e, remainder,
                              error, exp, true);
    if (result != digits::more) return result;
  } while (exp > 0);
  // Generate digits for the fractional part.
  for (;;) {
    fractional *= 10;
    error *= 10;
    char digit = static_cast<char>('0' + (fractional >> -one.e));
    fractional &= one.f - 1;
    --exp;
    result = handler.on_digit(digit, one.f, fractional, error, exp, false);
    if (result != digits::more) return result;
  }
}

// The fixed precision digit handler.
struct fixed_handler {
  char* buf;
  int size;
  int precision;
  int exp10;
  bool fixed;

  digits::result on_start(uint64_t divisor, uint64_t remainder, uint64_t error,
                          int& exp) {
    // Non-fixed formats require at least one digit and no precision adjustment.
    if (!fixed) return digits::more;
    // Adjust fixed precision by exponent because it is relative to decimal
    // point.
    precision += exp + exp10;
    // Check if precision is satisfied just by leading zeros, e.g.
    // format("{:.2f}", 0.001) gives "0.00" without generating any digits.
    if (precision > 0) return digits::more;
    if (precision < 0) return digits::done;
    auto dir = get_round_direction(divisor, remainder, error);
    if (dir == round_direction::unknown) return digits::error;
    buf[size++] = dir == round_direction::up ? '1' : '0';
    return digits::done;
  }

  digits::result on_digit(char digit, uint64_t divisor, uint64_t remainder,
                          uint64_t error, int, bool integral) {
    FMT_ASSERT(remainder < divisor, "");
    buf[size++] = digit;
    if (!integral && error >= remainder) return digits::error;
    if (size < precision) return digits::more;
    if (!integral) {
      // Check if error * 2 < divisor with overflow prevention.
      // The check is not needed for the integral part because error = 1
      // and divisor > (1 << 32) there.
      if (error >= divisor || error >= divisor - error) return digits::error;
    } else {
      FMT_ASSERT(error == 1 && divisor > 2, "");
    }
    auto dir = get_round_direction(divisor, remainder, error);
    if (dir != round_direction::up)
      return dir == round_direction::down ? digits::done : digits::error;
    ++buf[size - 1];
    for (int i = size - 1; i > 0 && buf[i] > '9'; --i) {
      buf[i] = '0';
      ++buf[i - 1];
    }
    if (buf[0] > '9') {
      buf[0] = '1';
      if (fixed)
        buf[size++] = '0';
      else
        ++exp10;
    }
    return digits::done;
  }
};

// Implementation of Dragonbox algorithm: https://github.com/jk-jeon/dragonbox.
namespace dragonbox {
// Computes 128-bit result of multiplication of two 64-bit unsigned integers.
inline uint128_wrapper umul128(uint64_t x, uint64_t y) FMT_NOEXCEPT {
#if FMT_USE_INT128
  return static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
#elif defined(_MSC_VER) && defined(_M_X64)
  uint128_wrapper result;
  result.low_ = _umul128(x, y, &result.high_);
  return result;
#else
  const uint64_t mask = (uint64_t(1) << 32) - uint64_t(1);

  uint64_t a = x >> 32;
  uint64_t b = x & mask;
  uint64_t c = y >> 32;
  uint64_t d = y & mask;

  uint64_t ac = a * c;
  uint64_t bc = b * c;
  uint64_t ad = a * d;
  uint64_t bd = b * d;

  uint64_t intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

  return {ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
          (intermediate << 32) + (bd & mask)};
#endif
}

// Computes upper 64 bits of multiplication of two 64-bit unsigned integers.
inline uint64_t umul128_upper64(uint64_t x, uint64_t y) FMT_NOEXCEPT {
#if FMT_USE_INT128
  auto p = static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
  return static_cast<uint64_t>(p >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
  return __umulh(x, y);
#else
  return umul128(x, y).high();
#endif
}

// Computes upper 64 bits of multiplication of a 64-bit unsigned integer and a
// 128-bit unsigned integer.
inline uint64_t umul192_upper64(uint64_t x, uint128_wrapper y) FMT_NOEXCEPT {
  uint128_wrapper g0 = umul128(x, y.high());
  g0 += umul128_upper64(x, y.low());
  return g0.high();
}

// Computes upper 32 bits of multiplication of a 32-bit unsigned integer and a
// 64-bit unsigned integer.
inline uint32_t umul96_upper32(uint32_t x, uint64_t y) FMT_NOEXCEPT {
  return static_cast<uint32_t>(umul128_upper64(x, y));
}

// Computes middle 64 bits of multiplication of a 64-bit unsigned integer and a
// 128-bit unsigned integer.
inline uint64_t umul192_middle64(uint64_t x, uint128_wrapper y) FMT_NOEXCEPT {
  uint64_t g01 = x * y.high();
  uint64_t g10 = umul128_upper64(x, y.low());
  return g01 + g10;
}

// Computes lower 64 bits of multiplication of a 32-bit unsigned integer and a
// 64-bit unsigned integer.
inline uint64_t umul96_lower64(uint32_t x, uint64_t y) FMT_NOEXCEPT {
  return x * y;
}

// Computes floor(log10(pow(2, e))) for e in [-1700, 1700] using the method from
// https://fmt.dev/papers/Grisu-Exact.pdf#page=5, section 3.4.
inline int floor_log10_pow2(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1700 && e >= -1700, "too large exponent");
  const int shift = 22;
  return (e * static_cast<int>(data::log10_2_significand >> (64 - shift))) >>
         shift;
}

// Various fast log computations.
inline int floor_log2_pow10(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1233 && e >= -1233, "too large exponent");
  const uint64_t log2_10_integer_part = 3;
  const uint64_t log2_10_fractional_digits = 0x5269e12f346e2bf9;
  const int shift_amount = 19;
  return (e * static_cast<int>(
                  (log2_10_integer_part << shift_amount) |
                  (log2_10_fractional_digits >> (64 - shift_amount)))) >>
         shift_amount;
}
inline int floor_log10_pow2_minus_log10_4_over_3(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1700 && e >= -1700, "too large exponent");
  const uint64_t log10_4_over_3_fractional_digits = 0x1ffbfc2bbc780375;
  const int shift_amount = 22;
  return (e * static_cast<int>(data::log10_2_significand >>
                               (64 - shift_amount)) -
          static_cast<int>(log10_4_over_3_fractional_digits >>
                           (64 - shift_amount))) >>
         shift_amount;
}

// Returns true iff x is divisible by pow(2, exp).
inline bool divisible_by_power_of_2(uint32_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp >= 1, "");
  FMT_ASSERT(x != 0, "");
#ifdef FMT_BUILTIN_CTZ
  return FMT_BUILTIN_CTZ(x) >= exp;
#else
  return exp < num_bits<uint32_t>() && x == ((x >> exp) << exp);
#endif
}
inline bool divisible_by_power_of_2(uint64_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp >= 1, "");
  FMT_ASSERT(x != 0, "");
#ifdef FMT_BUILTIN_CTZLL
  return FMT_BUILTIN_CTZLL(x) >= exp;
#else
  return exp < num_bits<uint64_t>() && x == ((x >> exp) << exp);
#endif
}

// Returns true iff x is divisible by pow(5, exp).
inline bool divisible_by_power_of_5(uint32_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp <= 10, "too large exponent");
  return x * data::divtest_table_for_pow5_32[exp].mod_inv <=
         data::divtest_table_for_pow5_32[exp].max_quotient;
}
inline bool divisible_by_power_of_5(uint64_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp <= 23, "too large exponent");
  return x * data::divtest_table_for_pow5_64[exp].mod_inv <=
         data::divtest_table_for_pow5_64[exp].max_quotient;
}

// Replaces n by floor(n / pow(5, N)) returning true if and only if n is
// divisible by pow(5, N).
// Precondition: n <= 2 * pow(5, N + 1).
template <int N>
bool check_divisibility_and_divide_by_pow5(uint32_t& n) FMT_NOEXCEPT {
  static constexpr struct {
    uint32_t magic_number;
    int bits_for_comparison;
    uint32_t threshold;
    int shift_amount;
  } infos[] = {{0xcccd, 16, 0x3333, 18}, {0xa429, 8, 0x0a, 20}};
  constexpr auto info = infos[N - 1];
  n *= info.magic_number;
  const uint32_t comparison_mask = (1u << info.bits_for_comparison) - 1;
  bool result = (n & comparison_mask) <= info.threshold;
  n >>= info.shift_amount;
  return result;
}

// Computes floor(n / pow(10, N)) for small n and N.
// Precondition: n <= pow(10, N + 1).
template <int N> uint32_t small_division_by_pow10(uint32_t n) FMT_NOEXCEPT {
  static constexpr struct {
    uint32_t magic_number;
    int shift_amount;
    uint32_t divisor_times_10;
  } infos[] = {{0xcccd, 19, 100}, {0xa3d8, 22, 1000}};
  constexpr auto info = infos[N - 1];
  FMT_ASSERT(n <= info.divisor_times_10, "n is too large");
  return n * info.magic_number >> info.shift_amount;
}

// Computes floor(n / 10^(kappa + 1)) (float)
inline uint32_t divide_by_10_to_kappa_plus_1(uint32_t n) FMT_NOEXCEPT {
  return n / float_info<float>::big_divisor;
}
// Computes floor(n / 10^(kappa + 1)) (double)
inline uint64_t divide_by_10_to_kappa_plus_1(uint64_t n) FMT_NOEXCEPT {
  return umul128_upper64(n, 0x83126e978d4fdf3c) >> 9;
}

// Various subroutines using pow10 cache
template <class T> struct cache_accessor;

template <> struct cache_accessor<float> {
  using carrier_uint = float_info<float>::carrier_uint;
  using cache_entry_type = uint64_t;

  static uint64_t get_cached_power(int k) FMT_NOEXCEPT {
    FMT_ASSERT(k >= float_info<float>::min_k && k <= float_info<float>::max_k,
               "k is out of range");
    return data::dragonbox_pow10_significands_64[k - float_info<float>::min_k];
  }

  static carrier_uint compute_mul(carrier_uint u,
                                  const cache_entry_type& cache) FMT_NOEXCEPT {
    return umul96_upper32(u, cache);
  }

  static uint32_t compute_delta(const cache_entry_type& cache,
                                int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<uint32_t>(cache >> (64 - 1 - beta_minus_1));
  }

  static bool compute_mul_parity(carrier_uint two_f,
                                 const cache_entry_type& cache,
                                 int beta_minus_1) FMT_NOEXCEPT {
    FMT_ASSERT(beta_minus_1 >= 1, "");
    FMT_ASSERT(beta_minus_1 < 64, "");

    return ((umul96_lower64(two_f, cache) >> (64 - beta_minus_1)) & 1) != 0;
  }

  static carrier_uint compute_left_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<carrier_uint>(
        (cache - (cache >> (float_info<float>::significand_bits + 2))) >>
        (64 - float_info<float>::significand_bits - 1 - beta_minus_1));
  }

  static carrier_uint compute_right_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<carrier_uint>(
        (cache + (cache >> (float_info<float>::significand_bits + 1))) >>
        (64 - float_info<float>::significand_bits - 1 - beta_minus_1));
  }

  static carrier_uint compute_round_up_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (static_cast<carrier_uint>(
                cache >>
                (64 - float_info<float>::significand_bits - 2 - beta_minus_1)) +
            1) /
           2;
  }
};

template <> struct cache_accessor<double> {
  using carrier_uint = float_info<double>::carrier_uint;
  using cache_entry_type = uint128_wrapper;

  static uint128_wrapper get_cached_power(int k) FMT_NOEXCEPT {
    FMT_ASSERT(k >= float_info<double>::min_k && k <= float_info<double>::max_k,
               "k is out of range");

#if FMT_USE_FULL_CACHE_DRAGONBOX
    return data::dragonbox_pow10_significands_128[k -
                                                  float_info<double>::min_k];
#else
    static const int compression_ratio = 27;

    // Compute base index.
    int cache_index = (k - float_info<double>::min_k) / compression_ratio;
    int kb = cache_index * compression_ratio + float_info<double>::min_k;
    int offset = k - kb;

    // Get base cache.
    uint128_wrapper base_cache =
        data::dragonbox_pow10_significands_128[cache_index];
    if (offset == 0) return base_cache;

    // Compute the required amount of bit-shift.
    int alpha = floor_log2_pow10(kb + offset) - floor_log2_pow10(kb) - offset;
    FMT_ASSERT(alpha > 0 && alpha < 64, "shifting error detected");

    // Try to recover the real cache.
    uint64_t pow5 = data::powers_of_5_64[offset];
    uint128_wrapper recovered_cache = umul128(base_cache.high(), pow5);
    uint128_wrapper middle_low =
        umul128(base_cache.low() - (kb < 0 ? 1u : 0u), pow5);

    recovered_cache += middle_low.high();

    uint64_t high_to_middle = recovered_cache.high() << (64 - alpha);
    uint64_t middle_to_low = recovered_cache.low() << (64 - alpha);

    recovered_cache =
        uint128_wrapper{(recovered_cache.low() >> alpha) | high_to_middle,
                        ((middle_low.low() >> alpha) | middle_to_low)};

    if (kb < 0) recovered_cache += 1;

    // Get error.
    int error_idx = (k - float_info<double>::min_k) / 16;
    uint32_t error = (data::dragonbox_pow10_recovery_errors[error_idx] >>
                      ((k - float_info<double>::min_k) % 16) * 2) &
                     0x3;

    // Add the error back.
    FMT_ASSERT(recovered_cache.low() + error >= recovered_cache.low(), "");
    return {recovered_cache.high(), recovered_cache.low() + error};
#endif
  }

  static carrier_uint compute_mul(carrier_uint u,
                                  const cache_entry_type& cache) FMT_NOEXCEPT {
    return umul192_upper64(u, cache);
  }

  static uint32_t compute_delta(cache_entry_type const& cache,
                                int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<uint32_t>(cache.high() >> (64 - 1 - beta_minus_1));
  }

  static bool compute_mul_parity(carrier_uint two_f,
                                 const cache_entry_type& cache,
                                 int beta_minus_1) FMT_NOEXCEPT {
    FMT_ASSERT(beta_minus_1 >= 1, "");
    FMT_ASSERT(beta_minus_1 < 64, "");

    return ((umul192_middle64(two_f, cache) >> (64 - beta_minus_1)) & 1) != 0;
  }

  static carrier_uint compute_left_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (cache.high() -
            (cache.high() >> (float_info<double>::significand_bits + 2))) >>
           (64 - float_info<double>::significand_bits - 1 - beta_minus_1);
  }

  static carrier_uint compute_right_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (cache.high() +
            (cache.high() >> (float_info<double>::significand_bits + 1))) >>
           (64 - float_info<double>::significand_bits - 1 - beta_minus_1);
  }

  static carrier_uint compute_round_up_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return ((cache.high() >>
             (64 - float_info<double>::significand_bits - 2 - beta_minus_1)) +
            1) /
           2;
  }
};

// Various integer checks
template <class T>
bool is_left_endpoint_integer_shorter_interval(int exponent) FMT_NOEXCEPT {
  return exponent >=
             float_info<
                 T>::case_shorter_interval_left_endpoint_lower_threshold &&
         exponent <=
             float_info<T>::case_shorter_interval_left_endpoint_upper_threshold;
}
template <class T>
bool is_endpoint_integer(typename float_info<T>::carrier_uint two_f,
                         int exponent, int minus_k) FMT_NOEXCEPT {
  if (exponent < float_info<T>::case_fc_pm_half_lower_threshold) return false;
  // For k >= 0.
  if (exponent <= float_info<T>::case_fc_pm_half_upper_threshold) return true;
  // For k < 0.
  if (exponent > float_info<T>::divisibility_check_by_5_threshold) return false;
  return divisible_by_power_of_5(two_f, minus_k);
}

template <class T>
bool is_center_integer(typename float_info<T>::carrier_uint two_f, int exponent,
                       int minus_k) FMT_NOEXCEPT {
  // Exponent for 5 is negative.
  if (exponent > float_info<T>::divisibility_check_by_5_threshold) return false;
  if (exponent > float_info<T>::case_fc_upper_threshold)
    return divisible_by_power_of_5(two_f, minus_k);
  // Both exponents are nonnegative.
  if (exponent >= float_info<T>::case_fc_lower_threshold) return true;
  // Exponent for 2 is negative.
  return divisible_by_power_of_2(two_f, minus_k - exponent + 1);
}

// Remove trailing zeros from n and return the number of zeros removed (float)
FMT_ALWAYS_INLINE int remove_trailing_zeros(uint32_t& n) FMT_NOEXCEPT {
#ifdef FMT_BUILTIN_CTZ
  int t = FMT_BUILTIN_CTZ(n);
#else
  int t = ctz(n);
#endif
  if (t > float_info<float>::max_trailing_zeros)
    t = float_info<float>::max_trailing_zeros;

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint32_t mod_inv2 = 0xc28f5c29;
  const uint32_t max_quotient2 = 0x0a3d70a3;

  int s = 0;
  for (; s < t - 1; s += 2) {
    if (n * mod_inv2 > max_quotient2) break;
    n *= mod_inv2;
  }
  if (s < t && n * mod_inv1 <= max_quotient1) {
    n *= mod_inv1;
    ++s;
  }
  n >>= s;
  return s;
}

// Removes trailing zeros and returns the number of zeros removed (double)
FMT_ALWAYS_INLINE int remove_trailing_zeros(uint64_t& n) FMT_NOEXCEPT {
#ifdef FMT_BUILTIN_CTZLL
  int t = FMT_BUILTIN_CTZLL(n);
#else
  int t = ctzll(n);
#endif
  if (t > float_info<double>::max_trailing_zeros)
    t = float_info<double>::max_trailing_zeros;
  // Divide by 10^8 and reduce to 32-bits
  // Since ret_value.significand <= (2^64 - 1) / 1000 < 10^17,
  // both of the quotient and the r should fit in 32-bits

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint64_t mod_inv8 = 0xc767074b22e90e21;
  const uint64_t max_quotient8 = 0x00002af31dc46118;

  // If the number is divisible by 1'0000'0000, work with the quotient
  if (t >= 8) {
    auto quotient_candidate = n * mod_inv8;

    if (quotient_candidate <= max_quotient8) {
      auto quotient = static_cast<uint32_t>(quotient_candidate >> 8);

      int s = 8;
      for (; s < t; ++s) {
        if (quotient * mod_inv1 > max_quotient1) break;
        quotient *= mod_inv1;
      }
      quotient >>= (s - 8);
      n = quotient;
      return s;
    }
  }

  // Otherwise, work with the remainder
  auto quotient = static_cast<uint32_t>(n / 100000000);
  auto remainder = static_cast<uint32_t>(n - 100000000 * quotient);

  if (t == 0 || remainder * mod_inv1 > max_quotient1) {
    return 0;
  }
  remainder *= mod_inv1;

  if (t == 1 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 1) + quotient * 10000000ull;
    return 1;
  }
  remainder *= mod_inv1;

  if (t == 2 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 2) + quotient * 1000000ull;
    return 2;
  }
  remainder *= mod_inv1;

  if (t == 3 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 3) + quotient * 100000ull;
    return 3;
  }
  remainder *= mod_inv1;

  if (t == 4 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 4) + quotient * 10000ull;
    return 4;
  }
  remainder *= mod_inv1;

  if (t == 5 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 5) + quotient * 1000ull;
    return 5;
  }
  remainder *= mod_inv1;

  if (t == 6 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 6) + quotient * 100ull;
    return 6;
  }
  remainder *= mod_inv1;

  n = (remainder >> 7) + quotient * 10ull;
  return 7;
}

// The main algorithm for shorter interval case
template <class T>
FMT_ALWAYS_INLINE decimal_fp<T> shorter_interval_case(int exponent)
    FMT_NOEXCEPT {
  decimal_fp<T> ret_value;
  // Compute k and beta
  const int minus_k = floor_log10_pow2_minus_log10_4_over_3(exponent);
  const int beta_minus_1 = exponent + floor_log2_pow10(-minus_k);

  // Compute xi and zi
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);

  auto xi = cache_accessor<T>::compute_left_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);
  auto zi = cache_accessor<T>::compute_right_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);

  // If the left endpoint is not an integer, increase it
  if (!is_left_endpoint_integer_shorter_interval<T>(exponent)) ++xi;

  // Try bigger divisor
  ret_value.significand = zi / 10;

  // If succeed, remove trailing zeros if necessary and return
  if (ret_value.significand * 10 >= xi) {
    ret_value.exponent = minus_k + 1;
    ret_value.exponent += remove_trailing_zeros(ret_value.significand);
    return ret_value;
  }

  // Otherwise, compute the round-up of y
  ret_value.significand =
      cache_accessor<T>::compute_round_up_for_shorter_interval_case(
          cache, beta_minus_1);
  ret_value.exponent = minus_k;

  // When tie occurs, choose one of them according to the rule
  if (exponent >= float_info<T>::shorter_interval_tie_lower_threshold &&
      exponent <= float_info<T>::shorter_interval_tie_upper_threshold) {
    ret_value.significand = ret_value.significand % 2 == 0
                                ? ret_value.significand
                                : ret_value.significand - 1;
  } else if (ret_value.significand < xi) {
    ++ret_value.significand;
  }
  return ret_value;
}

template <typename T> decimal_fp<T> to_decimal(T x) FMT_NOEXCEPT {
  // Step 1: integer promotion & Schubfach multiplier calculation.

  using carrier_uint = typename float_info<T>::carrier_uint;
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  auto br = bit_cast<carrier_uint>(x);

  // Extract significand bits and exponent bits.
  const carrier_uint significand_mask =
      (static_cast<carrier_uint>(1) << float_info<T>::significand_bits) - 1;
  carrier_uint significand = (br & significand_mask);
  int exponent = static_cast<int>((br & exponent_mask<T>()) >>
                                  float_info<T>::significand_bits);

  if (exponent != 0) {  // Check if normal.
    exponent += float_info<T>::exponent_bias - float_info<T>::significand_bits;

    // Shorter interval case; proceed like Schubfach.
    if (significand == 0) return shorter_interval_case<T>(exponent);

    significand |=
        (static_cast<carrier_uint>(1) << float_info<T>::significand_bits);
  } else {
    // Subnormal case; the interval is always regular.
    if (significand == 0) return {0, 0};
    exponent = float_info<T>::min_exponent - float_info<T>::significand_bits;
  }

  const bool include_left_endpoint = (significand % 2 == 0);
  const bool include_right_endpoint = include_left_endpoint;

  // Compute k and beta.
  const int minus_k = floor_log10_pow2(exponent) - float_info<T>::kappa;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);
  const int beta_minus_1 = exponent + floor_log2_pow10(-minus_k);

  // Compute zi and deltai
  // 10^kappa <= deltai < 10^(kappa + 1)
  const uint32_t deltai = cache_accessor<T>::compute_delta(cache, beta_minus_1);
  const carrier_uint two_fc = significand << 1;
  const carrier_uint two_fr = two_fc | 1;
  const carrier_uint zi =
      cache_accessor<T>::compute_mul(two_fr << beta_minus_1, cache);

  // Step 2: Try larger divisor; remove trailing zeros if necessary

  // Using an upper bound on zi, we might be able to optimize the division
  // better than the compiler; we are computing zi / big_divisor here
  decimal_fp<T> ret_value;
  ret_value.significand = divide_by_10_to_kappa_plus_1(zi);
  uint32_t r = static_cast<uint32_t>(zi - float_info<T>::big_divisor *
                                              ret_value.significand);

  if (r > deltai) {
    goto small_divisor_case_label;
  } else if (r < deltai) {
    // Exclude the right endpoint if necessary
    if (r == 0 && !include_right_endpoint &&
        is_endpoint_integer<T>(two_fr, exponent, minus_k)) {
      --ret_value.significand;
      r = float_info<T>::big_divisor;
      goto small_divisor_case_label;
    }
  } else {
    // r == deltai; compare fractional parts
    // Check conditions in the order different from the paper
    // to take advantage of short-circuiting
    const carrier_uint two_fl = two_fc - 1;
    if ((!include_left_endpoint ||
         !is_endpoint_integer<T>(two_fl, exponent, minus_k)) &&
        !cache_accessor<T>::compute_mul_parity(two_fl, cache, beta_minus_1)) {
      goto small_divisor_case_label;
    }
  }
  ret_value.exponent = minus_k + float_info<T>::kappa + 1;

  // We may need to remove trailing zeros
  ret_value.exponent += remove_trailing_zeros(ret_value.significand);
  return ret_value;

  // Step 3: Find the significand with the smaller divisor

small_divisor_case_label:
  ret_value.significand *= 10;
  ret_value.exponent = minus_k + float_info<T>::kappa;

  const uint32_t mask = (1u << float_info<T>::kappa) - 1;
  auto dist = r - (deltai / 2) + (float_info<T>::small_divisor / 2);

  // Is dist divisible by 2^kappa?
  if ((dist & mask) == 0) {
    const bool approx_y_parity =
        ((dist ^ (float_info<T>::small_divisor / 2)) & 1) != 0;
    dist >>= float_info<T>::kappa;

    // Is dist divisible by 5^kappa?
    if (check_divisibility_and_divide_by_pow5<float_info<T>::kappa>(dist)) {
      ret_value.significand += dist;

      // Check z^(f) >= epsilon^(f)
      // We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
      // where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f)
      // Since there are only 2 possibilities, we only need to care about the
      // parity. Also, zi and r should have the same parity since the divisor
      // is an even number
      if (cache_accessor<T>::compute_mul_parity(two_fc, cache, beta_minus_1) !=
          approx_y_parity) {
        --ret_value.significand;
      } else {
        // If z^(f) >= epsilon^(f), we might have a tie
        // when z^(f) == epsilon^(f), or equivalently, when y is an integer
        if (is_center_integer<T>(two_fc, exponent, minus_k)) {
          ret_value.significand = ret_value.significand % 2 == 0
                                      ? ret_value.significand
                                      : ret_value.significand - 1;
        }
      }
    }
    // Is dist not divisible by 5^kappa?
    else {
      ret_value.significand += dist;
    }
  }
  // Is dist not divisible by 2^kappa?
  else {
    // Since we know dist is small, we might be able to optimize the division
    // better than the compiler; we are computing dist / small_divisor here
    ret_value.significand +=
        small_division_by_pow10<float_info<T>::kappa>(dist);
  }
  return ret_value;
}
}  // namespace dragonbox

// Formats value using a variation of the Fixed-Precision Positive
// Floating-Point Printout ((FPP)^2) algorithm by Steele & White:
// https://fmt.dev/p372-steele.pdf.
template <typename Double>
void fallback_format(Double d, int num_digits, bool binary32, buffer<char>& buf,
                     int& exp10) {
  bigint numerator;    // 2 * R in (FPP)^2.
  bigint denominator;  // 2 * S in (FPP)^2.
  // lower and upper are differences between value and corresponding boundaries.
  bigint lower;             // (M^- in (FPP)^2).
  bigint upper_store;       // upper's value if different from lower.
  bigint* upper = nullptr;  // (M^+ in (FPP)^2).
  fp value;
  // Shift numerator and denominator by an extra bit or two (if lower boundary
  // is closer) to make lower and upper integers. This eliminates multiplication
  // by 2 during later computations.
  const bool is_predecessor_closer =
      binary32 ? value.assign(static_cast<float>(d)) : value.assign(d);
  int shift = is_predecessor_closer ? 2 : 1;
  uint64_t significand = value.f << shift;
  if (value.e >= 0) {
    numerator.assign(significand);
    numerator <<= value.e;
    lower.assign(1);
    lower <<= value.e;
    if (shift != 1) {
      upper_store.assign(1);
      upper_store <<= value.e + 1;
      upper = &upper_store;
    }
    denominator.assign_pow10(exp10);
    denominator <<= shift;
  } else if (exp10 < 0) {
    numerator.assign_pow10(-exp10);
    lower.assign(numerator);
    if (shift != 1) {
      upper_store.assign(numerator);
      upper_store <<= 1;
      upper = &upper_store;
    }
    numerator *= significand;
    denominator.assign(1);
    denominator <<= shift - value.e;
  } else {
    numerator.assign(significand);
    denominator.assign_pow10(exp10);
    denominator <<= shift - value.e;
    lower.assign(1);
    if (shift != 1) {
      upper_store.assign(1ULL << 1);
      upper = &upper_store;
    }
  }
  // Invariant: value == (numerator / denominator) * pow(10, exp10).
  if (num_digits < 0) {
    // Generate the shortest representation.
    if (!upper) upper = &lower;
    bool even = (value.f & 1) == 0;
    num_digits = 0;
    char* data = buf.data();
    for (;;) {
      int digit = numerator.divmod_assign(denominator);
      bool low = compare(numerator, lower) - even < 0;  // numerator <[=] lower.
      // numerator + upper >[=] pow10:
      bool high = add_compare(numerator, *upper, denominator) + even > 0;
      data[num_digits++] = static_cast<char>('0' + digit);
      if (low || high) {
        if (!low) {
          ++data[num_digits - 1];
        } else if (high) {
          int result = add_compare(numerator, numerator, denominator);
          // Round half to even.
          if (result > 0 || (result == 0 && (digit % 2) != 0))
            ++data[num_digits - 1];
        }
        buf.try_resize(to_unsigned(num_digits));
        exp10 -= num_digits - 1;
        return;
      }
      numerator *= 10;
      lower *= 10;
      if (upper != &lower) *upper *= 10;
    }
  }
  // Generate the given number of digits.
  exp10 -= num_digits - 1;
  if (num_digits == 0) {
    buf.try_resize(1);
    denominator *= 10;
    buf[0] = add_compare(numerator, numerator, denominator) > 0 ? '1' : '0';
    return;
  }
  buf.try_resize(to_unsigned(num_digits));
  for (int i = 0; i < num_digits - 1; ++i) {
    int digit = numerator.divmod_assign(denominator);
    buf[i] = static_cast<char>('0' + digit);
    numerator *= 10;
  }
  int digit = numerator.divmod_assign(denominator);
  auto result = add_compare(numerator, numerator, denominator);
  if (result > 0 || (result == 0 && (digit % 2) != 0)) {
    if (digit == 9) {
      const auto overflow = '0' + 10;
      buf[num_digits - 1] = overflow;
      // Propagate the carry.
      for (int i = num_digits - 1; i > 0 && buf[i] == overflow; --i) {
        buf[i] = '0';
        ++buf[i - 1];
      }
      if (buf[0] == overflow) {
        buf[0] = '1';
        ++exp10;
      }
      return;
    }
    ++digit;
  }
  buf[num_digits - 1] = static_cast<char>('0' + digit);
}

template <typename T>
int format_float(T value, int precision, float_specs specs, buffer<char>& buf) {
  static_assert(!std::is_same<T, float>::value, "");
  FMT_ASSERT(value >= 0, "value is negative");

  const bool fixed = specs.format == float_format::fixed;
  if (value <= 0) {  // <= instead of == to silence a warning.
    if (precision <= 0 || !fixed) {
      buf.push_back('0');
      return 0;
    }
    buf.try_resize(to_unsigned(precision));
    std::uninitialized_fill_n(buf.data(), precision, '0');
    return -precision;
  }

  if (!specs.use_grisu) return snprintf_float(value, precision, specs, buf);

  if (precision < 0) {
    // Use Dragonbox for the shortest format.
    if (specs.binary32) {
      auto dec = dragonbox::to_decimal(static_cast<float>(value));
      write<char>(buffer_appender<char>(buf), dec.significand);
      return dec.exponent;
    }
    auto dec = dragonbox::to_decimal(static_cast<double>(value));
    write<char>(buffer_appender<char>(buf), dec.significand);
    return dec.exponent;
  }

  // Use Grisu + Dragon4 for the given precision:
  // https://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf.
  int exp = 0;
  const int min_exp = -60;  // alpha in Grisu.
  int cached_exp10 = 0;     // K in Grisu.
  fp normalized = normalize(fp(value));
  const auto cached_pow = get_cached_power(
      min_exp - (normalized.e + fp::significand_size), cached_exp10);
  normalized = normalized * cached_pow;
  // Limit precision to the maximum possible number of significant digits in an
  // IEEE754 double because we don't need to generate zeros.
  const int max_double_digits = 767;
  if (precision > max_double_digits) precision = max_double_digits;
  fixed_handler handler{buf.data(), 0, precision, -cached_exp10, fixed};
  if (grisu_gen_digits(normalized, 1, exp, handler) == digits::error) {
    exp += handler.size - cached_exp10 - 1;
    fallback_format(value, handler.precision, specs.binary32, buf, exp);
  } else {
    exp += handler.exp10;
    buf.try_resize(to_unsigned(handler.size));
  }
  if (!fixed && !specs.showpoint) {
    // Remove trailing zeros.
    auto num_digits = buf.size();
    while (num_digits > 0 && buf[num_digits - 1] == '0') {
      --num_digits;
      ++exp;
    }
    buf.try_resize(num_digits);
  }
  return exp;
}  // namespace detail

template <typename T>
int snprintf_float(T value, int precision, float_specs specs,
                   buffer<char>& buf) {
  // Buffer capacity must be non-zero, otherwise MSVC's vsnprintf_s will fail.
  FMT_ASSERT(buf.capacity() > buf.size(), "empty buffer");
  static_assert(!std::is_same<T, float>::value, "");

  // Subtract 1 to account for the difference in precision since we use %e for
  // both general and exponent format.
  if (specs.format == float_format::general ||
      specs.format == float_format::exp)
    precision = (precision >= 0 ? precision : 6) - 1;

  // Build the format string.
  enum { max_format_size = 7 };  // The longest format is "%#.*Le".
  char format[max_format_size];
  char* format_ptr = format;
  *format_ptr++ = '%';
  if (specs.showpoint && specs.format == float_format::hex) *format_ptr++ = '#';
  if (precision >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (std::is_same<T, long double>()) *format_ptr++ = 'L';
  *format_ptr++ = specs.format != float_format::hex
                      ? (specs.format == float_format::fixed ? 'f' : 'e')
                      : (specs.upper ? 'A' : 'a');
  *format_ptr = '\0';

  // Format using snprintf.
  auto offset = buf.size();
  for (;;) {
    auto begin = buf.data() + offset;
    auto capacity = buf.capacity() - offset;
#ifdef FMT_FUZZ
    if (precision > 100000)
      throw std::runtime_error(
          "fuzz mode - avoid large allocation inside snprintf");
#endif
    // Suppress the warning about a nonliteral format string.
    // Cannot use auto because of a bug in MinGW (#1532).
    int (*snprintf_ptr)(char*, size_t, const char*, ...) = FMT_SNPRINTF;
    int result = precision >= 0
                     ? snprintf_ptr(begin, capacity, format, precision, value)
                     : snprintf_ptr(begin, capacity, format, value);
    if (result < 0) {
      // The buffer will grow exponentially.
      buf.try_reserve(buf.capacity() + 1);
      continue;
    }
    auto size = to_unsigned(result);
    // Size equal to capacity means that the last character was truncated.
    if (size >= capacity) {
      buf.try_reserve(size + offset + 1);  // Add 1 for the terminating '\0'.
      continue;
    }
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    if (specs.format == float_format::fixed) {
      if (precision == 0) {
        buf.try_resize(size);
        return 0;
      }
      // Find and remove the decimal point.
      auto end = begin + size, p = end;
      do {
        --p;
      } while (is_digit(*p));
      int fraction_size = static_cast<int>(end - p - 1);
      std::memmove(p, p + 1, to_unsigned(fraction_size));
      buf.try_resize(size - 1);
      return -fraction_size;
    }
    if (specs.format == float_format::hex) {
      buf.try_resize(size + offset);
      return 0;
    }
    // Find and parse the exponent.
    auto end = begin + size, exp_pos = end;
    do {
      --exp_pos;
    } while (*exp_pos != 'e');
    char sign = exp_pos[1];
    assert(sign == '+' || sign == '-');
    int exp = 0;
    auto p = exp_pos + 2;  // Skip 'e' and sign.
    do {
      assert(is_digit(*p));
      exp = exp * 10 + (*p++ - '0');
    } while (p != end);
    if (sign == '-') exp = -exp;
    int fraction_size = 0;
    if (exp_pos != begin + 1) {
      // Remove trailing zeros.
      auto fraction_end = exp_pos - 1;
      while (*fraction_end == '0') --fraction_end;
      // Move the fractional part left to get rid of the decimal point.
      fraction_size = static_cast<int>(fraction_end - begin - 1);
      std::memmove(begin + 1, begin + 2, to_unsigned(fraction_size));
    }
    buf.try_resize(to_unsigned(fraction_size) + offset + 1);
    return exp - fraction_size;
  }
}

// A public domain branchless UTF-8 decoder by Christopher Wellons:
// https://github.com/skeeto/branchless-utf8
/* Decode the next character, c, from buf, reporting errors in e.
 *
 * Since this is a branchless decoder, four bytes will be read from the
 * buffer regardless of the actual length of the next character. This
 * means the buffer _must_ have at least three bytes of zero padding
 * following the end of the data stream.
 *
 * Errors are reported in e, which will be non-zero if the parsed
 * character was somehow invalid: invalid byte sequence, non-canonical
 * encoding, or a surrogate half.
 *
 * The function returns a pointer to the next character. When an error
 * occurs, this pointer will be a guess that depends on the particular
 * error, but it will always advance at least one byte.
 */
inline const char* utf8_decode(const char* buf, uint32_t* c, int* e) {
  static const int masks[] = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
  static const uint32_t mins[] = {4194304, 0, 128, 2048, 65536};
  static const int shiftc[] = {0, 18, 12, 6, 0};
  static const int shifte[] = {0, 6, 4, 2, 0};

  int len = code_point_length(buf);
  const char* next = buf + len;

  // Assume a four-byte character and load four bytes. Unused bits are
  // shifted out.
  auto s = reinterpret_cast<const unsigned char*>(buf);
  *c = uint32_t(s[0] & masks[len]) << 18;
  *c |= uint32_t(s[1] & 0x3f) << 12;
  *c |= uint32_t(s[2] & 0x3f) << 6;
  *c |= uint32_t(s[3] & 0x3f) << 0;
  *c >>= shiftc[len];

  // Accumulate the various error conditions.
  *e = (*c < mins[len]) << 6;       // non-canonical encoding
  *e |= ((*c >> 11) == 0x1b) << 7;  // surrogate half?
  *e |= (*c > 0x10FFFF) << 8;       // out of range?
  *e |= (s[1] & 0xc0) >> 2;
  *e |= (s[2] & 0xc0) >> 4;
  *e |= (s[3]) >> 6;
  *e ^= 0x2a;  // top two bits of each tail byte correct?
  *e >>= shifte[len];

  return next;
}

struct stringifier {
  template <typename T> FMT_INLINE std::string operator()(T value) const {
    return to_string(value);
  }
  std::string operator()(basic_format_arg<format_context>::handle h) const {
    memory_buffer buf;
    format_parse_context parse_ctx({});
    format_context format_ctx(buffer_appender<char>(buf), {}, {});
    h.format(parse_ctx, format_ctx);
    return to_string(buf);
  }
};
}  // namespace detail

template <> struct formatter<detail::bigint> {
  format_parse_context::iterator parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  format_context::iterator format(const detail::bigint& n,
                                  format_context& ctx) {
    auto out = ctx.out();
    bool first = true;
    for (auto i = n.bigits_.size(); i > 0; --i) {
      auto value = n.bigits_[i - 1u];
      if (first) {
        out = format_to(out, "{:x}", value);
        first = false;
        continue;
      }
      out = format_to(out, "{:08x}", value);
    }
    if (n.exp_ > 0)
      out = format_to(out, "p{}", n.exp_ * detail::bigint::bigit_bits);
    return out;
  }
};

FMT_FUNC detail::utf8_to_utf16::utf8_to_utf16(string_view s) {
  auto transcode = [this](const char* p) {
    auto cp = uint32_t();
    auto error = 0;
    p = utf8_decode(p, &cp, &error);
    if (error != 0) FMT_THROW(std::runtime_error("invalid utf8"));
    if (cp <= 0xFFFF) {
      buffer_.push_back(static_cast<wchar_t>(cp));
    } else {
      cp -= 0x10000;
      buffer_.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
      buffer_.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
    }
    return p;
  };
  auto p = s.data();
  const size_t block_size = 4;  // utf8_decode always reads blocks of 4 chars.
  if (s.size() >= block_size) {
    for (auto end = p + s.size() - block_size + 1; p < end;) p = transcode(p);
  }
  if (auto num_chars_left = s.data() + s.size() - p) {
    char buf[2 * block_size - 1] = {};
    memcpy(buf, p, to_unsigned(num_chars_left));
    p = buf;
    do {
      p = transcode(p);
    } while (p - buf < num_chars_left);
  }
  buffer_.push_back(0);
}

FMT_FUNC void format_system_error(detail::buffer<char>& out, int error_code,
                                  string_view message) FMT_NOEXCEPT {
  FMT_TRY {
    memory_buffer buf;
    buf.resize(inline_buffer_size);
    for (;;) {
      char* system_message = &buf[0];
      int result =
          detail::safe_strerror(error_code, system_message, buf.size());
      if (result == 0) {
        format_to(detail::buffer_appender<char>(out), "{}: {}", message,
                  system_message);
        return;
      }
      if (result != ERANGE)
        break;  // Can't get error message, report error code instead.
      buf.resize(buf.size() * 2);
    }
  }
  FMT_CATCH(...) {}
  format_error_code(out, error_code, message);
}

FMT_FUNC void detail::error_handler::on_error(const char* message) {
  FMT_THROW(format_error(message));
}

FMT_FUNC void report_system_error(int error_code,
                                  fmt::string_view message) FMT_NOEXCEPT {
  report_error(format_system_error, error_code, message);
}

FMT_FUNC std::string detail::vformat(string_view format_str, format_args args) {
  if (format_str.size() == 2 && equal2(format_str.data(), "{}")) {
    auto arg = args.get(0);
    if (!arg) error_handler().on_error("argument not found");
    return visit_format_arg(stringifier(), arg);
  }
  memory_buffer buffer;
  detail::vformat_to(buffer, format_str, args);
  return to_string(buffer);
}

#ifdef _WIN32
namespace detail {
using dword = conditional_t<sizeof(long) == 4, unsigned long, unsigned>;
extern "C" __declspec(dllimport) int __stdcall WriteConsoleW(  //
    void*, const void*, dword, dword*, void*);
}  // namespace detail
#endif

FMT_FUNC void vprint(std::FILE* f, string_view format_str, format_args args) {
  memory_buffer buffer;
  detail::vformat_to(buffer, format_str,
                     basic_format_args<buffer_context<char>>(args));
#ifdef _WIN32
  auto fd = _fileno(f);
  if (_isatty(fd)) {
    detail::utf8_to_utf16 u16(string_view(buffer.data(), buffer.size()));
    auto written = detail::dword();
    if (!detail::WriteConsoleW(reinterpret_cast<void*>(_get_osfhandle(fd)),
                               u16.c_str(), static_cast<uint32_t>(u16.size()),
                               &written, nullptr)) {
      FMT_THROW(format_error("failed to write to console"));
    }
    return;
  }
#endif
  detail::fwrite_fully(buffer.data(), 1, buffer.size(), f);
}

#ifdef _WIN32
// Print assuming legacy (non-Unicode) encoding.
FMT_FUNC void detail::vprint_mojibake(std::FILE* f, string_view format_str,
                                      format_args args) {
  memory_buffer buffer;
  detail::vformat_to(buffer, format_str,
                     basic_format_args<buffer_context<char>>(args));
  fwrite_fully(buffer.data(), 1, buffer.size(), f);
}
#endif

FMT_FUNC void vprint(string_view format_str, format_args args) {
  vprint(stdout, format_str, args);
}

FMT_END_NAMESPACE

#endif  // FMT_FORMAT_INL_H_
