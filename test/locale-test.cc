// Formatting library for C++ - locale tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/locale.h"

#include <complex>

#include "gmock.h"

using fmt::detail::max_value;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template <typename Char> struct numpunct : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '?'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\03"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return '~'; }
};

template <typename Char> struct no_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return ""; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

template <typename Char> struct special_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\03\02"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

template <typename Char> struct small_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\01"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

TEST(LocaleTest, DoubleDecimalPoint) {
  std::locale loc(std::locale(), new numpunct<char>());
  EXPECT_EQ("1?23", fmt::format(loc, "{:L}", 1.23));
}

TEST(LocaleTest, Format) {
  std::locale loc(std::locale(), new numpunct<char>());
  EXPECT_EQ("1234567", fmt::format(std::locale(), "{:L}", 1234567));
  EXPECT_EQ("1~234~567", fmt::format(loc, "{:L}", 1234567));
  EXPECT_EQ("-1~234~567", fmt::format(loc, "{:L}", -1234567));
  EXPECT_EQ("-256", fmt::format(loc, "{:L}", -256));
  fmt::format_arg_store<fmt::format_context, int> as{1234567};
  EXPECT_EQ("1~234~567", fmt::vformat(loc, "{:L}", fmt::format_args(as)));
  std::string s;
  fmt::format_to(std::back_inserter(s), loc, "{:L}", 1234567);
  EXPECT_EQ("1~234~567", s);

  std::locale no_grouping_loc(std::locale(), new no_grouping<char>());
  EXPECT_EQ("1234567", fmt::format(no_grouping_loc, "{:L}", 1234567));

  std::locale special_grouping_loc(std::locale(), new special_grouping<char>());
  EXPECT_EQ("1,23,45,678", fmt::format(special_grouping_loc, "{:L}", 12345678));
  EXPECT_EQ("12,345", fmt::format(special_grouping_loc, "{:L}", 12345));

  std::locale small_grouping_loc(std::locale(), new small_grouping<char>());
  EXPECT_EQ("4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, "{:L}", max_value<uint32_t>()));
}

TEST(LocaleTest, FormatDetaultAlign) {
  std::locale special_grouping_loc(std::locale(), new special_grouping<char>());
  EXPECT_EQ("  12,345", fmt::format(special_grouping_loc, "{:8L}", 12345));
}

TEST(LocaleTest, WFormat) {
  std::locale loc(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(std::locale(), L"{:L}", 1234567));
  EXPECT_EQ(L"1~234~567", fmt::format(loc, L"{:L}", 1234567));
  fmt::format_arg_store<fmt::wformat_context, int> as{1234567};
  EXPECT_EQ(L"1~234~567", fmt::vformat(loc, L"{:L}", fmt::wformat_args(as)));
  EXPECT_EQ(L"1234567", fmt::format(std::locale("C"), L"{:L}", 1234567));

  std::locale no_grouping_loc(std::locale(), new no_grouping<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(no_grouping_loc, L"{:L}", 1234567));

  std::locale special_grouping_loc(std::locale(),
                                   new special_grouping<wchar_t>());
  EXPECT_EQ(L"1,23,45,678",
            fmt::format(special_grouping_loc, L"{:L}", 12345678));

  std::locale small_grouping_loc(std::locale(), new small_grouping<wchar_t>());
  EXPECT_EQ(L"4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, L"{:L}", max_value<uint32_t>()));
}

TEST(LocaleTest, DoubleFormatter) {
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
  typename basic_format_parse_context<charT>::iterator parse(
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
    auto format_specs = std::string();
    if (specs_.precision > 0)
      format_specs = fmt::format(".{}", specs_.precision);
    if (specs_.type)
      format_specs += specs_.type;
    auto real = fmt::format(ctx.locale().template get<std::locale>(),
                            "{:" + format_specs + "}", c.real());
    auto imag = fmt::format(ctx.locale().template get<std::locale>(),
                            "{:" + format_specs + "}", c.imag());
    auto fill_align_width = std::string();
    if (specs_.width > 0)
      fill_align_width = fmt::format(">{}", specs_.width);
    return format_to(
        ctx.out(), "{:" + fill_align_width + "}",
        fmt::format(c.real() != 0 ? "({0}+{1}i)" : "{1}i", real, imag));
  }
};
FMT_END_NAMESPACE

TEST(FormatTest, Complex) {
  std::string s = fmt::format("{}", std::complex<double>(1, 2));
  EXPECT_EQ(s, "(1+2i)");
  EXPECT_EQ(fmt::format("{:.2f}", std::complex<double>(1, 2)), "(1.00+2.00i)");
  EXPECT_EQ(fmt::format("{:8}", std::complex<double>(1, 2)), "  (1+2i)");
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
