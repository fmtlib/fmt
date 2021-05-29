// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/wchar.h"

#include <complex>

#include "gtest/gtest.h"

using fmt::detail::max_value;

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VER || FMT_MSC_VER >= 1900
struct explicitly_convertible_to_wstring_view {
  explicit operator fmt::wstring_view() const { return L"foo"; }
};

TEST(wchar_test, format_explicitly_convertible_to_wstring_view) {
  EXPECT_EQ(L"foo",
            fmt::format(L"{}", explicitly_convertible_to_wstring_view()));
}
#endif

TEST(wchar_test, vformat_to) {
  using wcontext = fmt::wformat_context;
  fmt::basic_format_arg<wcontext> warg = fmt::detail::make_arg<wcontext>(42);
  auto wargs = fmt::basic_format_args<wcontext>(&warg, 1);
  auto w = std::wstring();
  fmt::vformat_to(std::back_inserter(w), L"{}", wargs);
  EXPECT_EQ(L"42", w);
  w.clear();
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

TEST(wchar_test, format_to_memory_buffer) {
  auto buf = fmt::wmemory_buffer();
  fmt::format_to(buf, L"{}", L"foo");
  EXPECT_EQ(L"foo", to_string(buf));
}

TEST(format_test, wide_format_to_n) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, L"{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = fmt::format_to_n(buffer, 3, L"{}", L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", fmt::wstring_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, L"{}{} ", L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", fmt::wstring_view(buffer, 4));
}

#if FMT_USE_USER_DEFINED_LITERALS
TEST(wchar_test, format_udl) {
  using namespace fmt::literals;
  EXPECT_EQ(L"{}c{}"_format(L"ab", 1), fmt::format(L"{}c{}", L"ab", 1));
}

TEST(wchar_test, named_arg_udl) {
  using namespace fmt::literals;
  auto udl_a =
      fmt::format(L"{first}{second}{first}{third}", L"first"_a = L"abra",
                  L"second"_a = L"cad", L"third"_a = 99);
  EXPECT_EQ(
      fmt::format(L"{first}{second}{first}{third}", fmt::arg(L"first", L"abra"),
                  fmt::arg(L"second", L"cad"), fmt::arg(L"third", 99)),
      udl_a);
}
#endif  // FMT_USE_USER_DEFINED_LITERALS

TEST(wchar_test, print) {
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}

TEST(wchar_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ(fmt::format(L"({})", fmt::join(v, v + 3, L", ")), L"(1, 2, 3)");
}

TEST(wchar_test, to_wstring) { EXPECT_EQ(L"42", fmt::to_wstring(42)); }

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template <typename Char> struct numpunct : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '?'; }
  std::string do_grouping() const override { return "\03"; }
  Char do_thousands_sep() const override { return '~'; }
};

template <typename Char> struct no_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return ""; }
  Char do_thousands_sep() const override { return ','; }
};

template <typename Char> struct special_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return "\03\02"; }
  Char do_thousands_sep() const override { return ','; }
};

template <typename Char> struct small_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const override { return '.'; }
  std::string do_grouping() const override { return "\01"; }
  Char do_thousands_sep() const override { return ','; }
};

TEST(locale_test, double_decimal_point) {
  auto loc = std::locale(std::locale(), new numpunct<char>());
  EXPECT_EQ("1?23", fmt::format(loc, "{:L}", 1.23));
  EXPECT_EQ("1?230000", fmt::format(loc, "{:Lf}", 1.23));
}

TEST(locale_test, format) {
  auto loc = std::locale(std::locale(), new numpunct<char>());
  EXPECT_EQ("1234567", fmt::format(std::locale(), "{:L}", 1234567));
  EXPECT_EQ("1~234~567", fmt::format(loc, "{:L}", 1234567));
  EXPECT_EQ("-1~234~567", fmt::format(loc, "{:L}", -1234567));
  EXPECT_EQ("-256", fmt::format(loc, "{:L}", -256));
  fmt::format_arg_store<fmt::format_context, int> as{1234567};
  EXPECT_EQ("1~234~567", fmt::vformat(loc, "{:L}", fmt::format_args(as)));
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), loc, "{:L}", 1234567);
  EXPECT_EQ("1~234~567", s);

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<char>());
  EXPECT_EQ("1234567", fmt::format(no_grouping_loc, "{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<char>());
  EXPECT_EQ("1,23,45,678", fmt::format(special_grouping_loc, "{:L}", 12345678));
  EXPECT_EQ("12,345", fmt::format(special_grouping_loc, "{:L}", 12345));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<char>());
  EXPECT_EQ("4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, "{:L}", max_value<uint32_t>()));
}

TEST(locale_test, format_detault_align) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("  12,345", fmt::format(loc, "{:8L}", 12345));
}

TEST(locale_test, format_plus) {
  auto loc = std::locale({}, new special_grouping<char>());
  EXPECT_EQ("+100", fmt::format(loc, "{:+L}", 100));
}

TEST(locale_test, wformat) {
  auto loc = std::locale(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(std::locale(), L"{:L}", 1234567));
  EXPECT_EQ(L"1~234~567", fmt::format(loc, L"{:L}", 1234567));
  using wcontext = fmt::buffer_context<wchar_t>;
  fmt::format_arg_store<wcontext, int> as{1234567};
  EXPECT_EQ(L"1~234~567",
            fmt::vformat(loc, L"{:L}", fmt::basic_format_args<wcontext>(as)));
  EXPECT_EQ(L"1234567", fmt::format(std::locale("C"), L"{:L}", 1234567));

  auto no_grouping_loc = std::locale(std::locale(), new no_grouping<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(no_grouping_loc, L"{:L}", 1234567));

  auto special_grouping_loc =
      std::locale(std::locale(), new special_grouping<wchar_t>());
  EXPECT_EQ(L"1,23,45,678",
            fmt::format(special_grouping_loc, L"{:L}", 12345678));

  auto small_grouping_loc =
      std::locale(std::locale(), new small_grouping<wchar_t>());
  EXPECT_EQ(L"4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, L"{:L}", max_value<uint32_t>()));
}

TEST(locale_test, double_formatter) {
  auto loc = std::locale(std::locale(), new special_grouping<char>());
  auto f = fmt::formatter<int>();
  auto parse_ctx = fmt::format_parse_context("L");
  f.parse(parse_ctx);
  char buf[10] = {};
  fmt::basic_format_context<char*, char> format_ctx(
      buf, {}, fmt::detail::locale_ref(loc));
  *f.format(12345, format_ctx) = 0;
  EXPECT_STREQ("12,345", buf);
}

FMT_BEGIN_NAMESPACE
template <class charT> struct formatter<std::complex<double>, charT> {
 private:
  detail::dynamic_format_specs<char> specs_;

 public:
  FMT_CONSTEXPR typename basic_format_parse_context<charT>::iterator parse(
      basic_format_parse_context<charT>& ctx) {
    using handler_type =
        detail::dynamic_specs_handler<basic_format_parse_context<charT>>;
    detail::specs_checker<handler_type> handler(handler_type(specs_, ctx),
                                                detail::type::string_type);
    auto it = parse_format_specs(ctx.begin(), ctx.end(), handler);
    detail::parse_float_type_spec(specs_, ctx.error_handler());
    return it;
  }

  template <class FormatContext>
  typename FormatContext::iterator format(const std::complex<double>& c,
                                          FormatContext& ctx) {
    detail::handle_dynamic_spec<detail::precision_checker>(
        specs_.precision, specs_.precision_ref, ctx);
    auto specs = std::string();
    if (specs_.precision > 0) specs = fmt::format(".{}", specs_.precision);
    if (specs_.type) specs += specs_.type;
    auto real = fmt::format(ctx.locale().template get<std::locale>(),
                            fmt::runtime("{:" + specs + "}"), c.real());
    auto imag = fmt::format(ctx.locale().template get<std::locale>(),
                            fmt::runtime("{:" + specs + "}"), c.imag());
    auto fill_align_width = std::string();
    if (specs_.width > 0) fill_align_width = fmt::format(">{}", specs_.width);
    return format_to(ctx.out(), runtime("{:" + fill_align_width + "}"),
                     c.real() != 0 ? fmt::format("({}+{}i)", real, imag)
                                   : fmt::format("{}i", imag));
  }
};
FMT_END_NAMESPACE

TEST(locale_test, complex) {
  std::string s = fmt::format("{}", std::complex<double>(1, 2));
  EXPECT_EQ(s, "(1+2i)");
  EXPECT_EQ(fmt::format("{:.2f}", std::complex<double>(1, 2)), "(1.00+2.00i)");
  EXPECT_EQ(fmt::format("{:8}", std::complex<double>(1, 2)), "  (1+2i)");
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
