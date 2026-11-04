// Formatting library for C++ - styled std::ostream tests
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/color-ostream.h"

#include <iomanip>
#include <sstream>

#include "gtest-extra.h"

TEST(color_ostream_test, insertion) {
  std::ostringstream os;
  const auto style = fmt::text_style{fmt::emphasis::bold, fmt::rgb{1, 2, 3},
                                    fmt::term_color::black};
  const std::string text("a\0{}b", 5);
  auto& result = (os << fmt::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << ' ' << fmt::styled(42, fmt::emphasis::underline) << '!';
#if FMT_COLOR_COMPACT_SGR == 1
  const std::string opening = "\x1b[1;38;2;1;2;3;40m";
#else
  const std::string opening = "\x1b[1m\x1b[38;2;001;002;003m\x1b[40m";
#endif
  EXPECT_EQ(os.str(), opening + text + "\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_wide) {
  std::wostringstream os;
  const auto style = fmt::text_style{fmt::emphasis::bold, fmt::rgb{1, 2, 3},
                                    fmt::term_color::black};
  const std::wstring text(L"a\0{}b", 5);
  auto& result = (os << fmt::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << L' ' << fmt::styled(42, fmt::emphasis::underline) << L'!';
#if FMT_COLOR_COMPACT_SGR == 1
  const std::wstring opening = L"\x1b[1;38;2;1;2;3;40m";
#else
  const std::wstring opening = L"\x1b[1m\x1b[38;2;001;002;003m\x1b[40m";
#endif
  EXPECT_EQ(os.str(), opening + text + L"\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_empty_style_and_text) {
  std::ostringstream os;
  os << fmt::styled("plain", fmt::text_style{})
     << fmt::styled("", fmt::text_style{});
  EXPECT_EQ(os.str(), "plain");
  os << fmt::styled("", fmt::emphasis::bold);
  EXPECT_EQ(os.str(), "plain\x1b[1m\x1b[0m");

  std::wostringstream wide;
  wide << fmt::styled(L"plain", fmt::text_style{})
       << fmt::styled(L"", fmt::text_style{})
       << fmt::styled(L"", fmt::emphasis::bold);
  EXPECT_EQ(wide.str(), L"plain\x1b[1m\x1b[0m");
}

TEST(color_ostream_test, insertion_stream_formatting) {
  std::ostringstream os;
  os << std::hex << std::setfill('*') << std::setw(4)
     << fmt::styled(42, fmt::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::fixed << std::setprecision(2)
     << fmt::styled(1.25, fmt::emphasis::underline);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m\x1b[4m1.25\x1b[0m");
  EXPECT_EQ(os.precision(), 2);

  std::wostringstream wide;
  wide << std::hex << std::setfill(L'*') << std::setw(4)
       << fmt::styled(42, fmt::emphasis::bold);
  EXPECT_EQ(wide.str(), L"\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(wide.width(), 0);
  wide << std::setw(4) << fmt::styled(42, fmt::text_style{});
  EXPECT_EQ(wide.str(), L"\x1b[1m**2a\x1b[0m**2a");
}

namespace {
struct stream_only_value {};

auto operator<<(std::ostream& os, const stream_only_value&) -> std::ostream& {
  return os << "custom";
}
}  // namespace

TEST(color_ostream_test, insertion_stream_only_value) {
  std::ostringstream os;
  const stream_only_value value{};
  os << fmt::styled(value, fmt::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1mcustom\x1b[0m");
}
TEST(color_ostream_test, wide_print_and_println) {
  std::wostringstream os;
  const auto style = fmt::text_style{fmt::emphasis::bold, fmt::rgb{1, 2, 3},
                                    fmt::term_color::black};
  fmt::print(os, style, L"Result: {:04x} {}", 42, L"done");
#if FMT_COLOR_COMPACT_SGR == 1
  EXPECT_EQ(os.str(), L"\x1b[1;38;2;1;2;3;40mResult: 002a done\x1b[0m");
#else
  EXPECT_EQ(os.str(),
            L"\x1b[1m\x1b[38;2;001;002;003m\x1b[40mResult: 002a done\x1b[0m");
#endif
  os.str(L"");
  fmt::println(os, fmt::emphasis::underline, L"{}", 42);
  EXPECT_EQ(os.str(), L"\x1b[4m42\x1b[0m\n");
  os.str(L"");
  fmt::print(os, fmt::text_style{}, L"{}", L"plain");
  fmt::println(os, fmt::text_style{}, L"{}", 42);
  EXPECT_EQ(os.str(), L"plain42\n");
  os.str(L"");
  fmt::print(os, style, L"");
  EXPECT_EQ(os.str(), fmt::to_sgr<wchar_t>(style) + L"\x1b[0m");
}

TEST(color_ostream_test, wide_argument_packs) {
  std::wostringstream os;
  int value = 42;
  auto args = fmt::make_wformat_args(value);
  fmt::vprint(os, fmt::emphasis::bold, L"{:04}", args);
  EXPECT_EQ(os.str(), L"\x1b[1m0042\x1b[0m");
  os.str(L"");
  fmt::vprintln(os, fmt::emphasis::bold, L"{}", args);
  EXPECT_EQ(os.str(), L"\x1b[1m42\x1b[0m\n");
}

TEST(color_ostream_test, wide_print_preserves_stream_formatting) {
  std::wostringstream os;
  os << std::hex << std::setprecision(2) << std::setfill(L'*') << std::setw(30);
  const std::wstring text(L"a\0{}b", 5);
  fmt::print(os, fmt::emphasis::bold, L"{} {}", text, 42);
  const wchar_t expected[] = L"\x1b[1ma\0{}b 42\x1b[0m";
  EXPECT_EQ(os.str(), std::wstring(expected, sizeof(expected) / sizeof(*expected) - 1));
  EXPECT_EQ(os.width(), 30);
  EXPECT_EQ(os.fill(), L'*');
  EXPECT_EQ(os.precision(), 2);
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, wide_formatting_error) {
  std::wostringstream os;
  os << L"original";
  EXPECT_THROW(fmt::print(os, fmt::emphasis::bold, fmt::runtime(L"{")),
               fmt::format_error);
  EXPECT_EQ(os.str(), L"original");
}

namespace {
struct failing_wstreambuf : std::wstreambuf {
  std::streamsize xsputn(const wchar_t*, std::streamsize) override { return 0; }
};
}  // namespace

TEST(color_ostream_test, wide_stream_failure) {
  failing_wstreambuf buffer;
  std::wostream os(&buffer);
  fmt::print(os, fmt::emphasis::bold, L"{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(fmt::println(os, fmt::emphasis::bold, L"{}", 42),
               std::ios_base::failure);
}
TEST(color_ostream_test, print) {
  std::ostringstream os;
  const auto style = fmt::text_style{fmt::emphasis::bold, fmt::rgb{1, 2, 3},
                                    fmt::term_color::black};
  fmt::print(os, style, "Result: {:04x} {}", 42, "done");
#if FMT_COLOR_COMPACT_SGR == 1
  EXPECT_EQ(os.str(), "\x1b[1;38;2;1;2;3;40mResult: 002a done\x1b[0m");
#else
  EXPECT_EQ(os.str(),
            "\x1b[1m\x1b[38;2;001;002;003m\x1b[40mResult: 002a done\x1b[0m");
#endif
  fmt::print(os, " plain");
  EXPECT_EQ(os.str().substr(os.str().size() - 10), "\x1b[0m plain");
}

TEST(color_ostream_test, println) {
  std::ostringstream os;
  fmt::println(os, fmt::emphasis::underline, "Result: {}", 42);
  EXPECT_EQ(os.str(), "\x1b[4mResult: 42\x1b[0m\n");
}

TEST(color_ostream_test, argument_packs) {
  std::ostringstream os;
  int value = 42;
  auto args = fmt::make_format_args(value);
  fmt::vprint(os, fmt::emphasis::bold, "{:04}", args);
  EXPECT_EQ(os.str(), "\x1b[1m0042\x1b[0m");
  os.str("");
  fmt::vprintln(os, fmt::emphasis::bold, "{}", args);
  EXPECT_EQ(os.str(), "\x1b[1m42\x1b[0m\n");
}

TEST(color_ostream_test, empty_style_and_text) {
  std::ostringstream os;
  fmt::print(os, fmt::text_style{}, "{}", "plain");
  fmt::println(os, fmt::text_style{}, "{}", 42);
  EXPECT_EQ(os.str(), "plain42\n");
  os.str("");
  fmt::print(os, fmt::text_style{}, "");
  EXPECT_EQ(os.str(), "");
  fmt::print(os, fmt::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m");
  os.str("");
  fmt::println(os, fmt::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m\n");
}

TEST(color_ostream_test, literal_text_and_stream_state) {
  std::ostringstream os;
  os.width(30);
  os.fill('*');
  os << std::hex;
  const char text[] = "a\0{}b";
  fmt::print(os, fmt::emphasis::bold, "{} {}",
             fmt::string_view(text, sizeof(text) - 1), 42);
  const char expected[] = "\x1b[1ma\0{}b 42\x1b[0m";
  EXPECT_EQ(os.str(), std::string(expected, sizeof(expected) - 1));
  EXPECT_EQ(os.width(), 30);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, long_output) {
  std::ostringstream os;
  const auto text = std::string(4096, 'x');
  fmt::print(os, fmt::emphasis::bold, "{}", text);
  EXPECT_EQ(os.str(), "\x1b[1m" + text + "\x1b[0m");
}

TEST(color_ostream_test, formatting_error) {
  std::ostringstream os;
  os << "original";
  EXPECT_THROW(fmt::print(os, fmt::emphasis::bold, fmt::runtime("{")),
               fmt::format_error);
  EXPECT_EQ(os.str(), "original");
}

namespace {
struct failing_streambuf : std::streambuf {
  std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
};
}  // namespace

TEST(color_ostream_test, stream_failure) {
  failing_streambuf buf;
  std::ostream os(&buf);
  fmt::print(os, fmt::emphasis::bold, "{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(fmt::println(os, fmt::emphasis::bold, "{}", 42),
               std::ios_base::failure);
}

#if FMT_USE_EXCEPTIONS
namespace {
struct insertion_error {};
struct reset_error {};

struct recovery_streambuf : std::stringbuf {
  bool fail = false;
  int attempts = 0;

  std::streamsize xsputn(const char* text, std::streamsize size) override {
    ++attempts;
    if (fail) throw reset_error{};
    return std::stringbuf::xsputn(text, size);
  }
};

struct throwing_value {
  recovery_streambuf& buffer;
  bool fail_reset;
};

auto operator<<(std::ostream& os, const throwing_value& value) -> std::ostream& {
  os << "partial";
  value.buffer.fail = value.fail_reset;
  throw insertion_error{};
}
}  // namespace

TEST(color_ostream_test, insertion_exception_reset) {
  for (bool fail_reset : {false, true}) {
    SCOPED_TRACE(fail_reset);
    recovery_streambuf buffer;
    std::ostream os(&buffer);
    os.exceptions(std::ios_base::badbit);
    const throwing_value value{buffer, fail_reset};
    EXPECT_THROW(os << fmt::styled(value, fmt::emphasis::bold), insertion_error);
    EXPECT_EQ(buffer.attempts, 3);
    EXPECT_EQ(buffer.str(), fail_reset ? "\x1b[1mpartial"
                                       : "\x1b[1mpartial\x1b[0m");
    EXPECT_EQ(os.bad(), fail_reset);
    EXPECT_EQ(os.exceptions(), std::ios_base::badbit);
  }
}
#endif
