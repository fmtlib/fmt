// Formatting library for C++ - module tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2021 - present, Daniela Engert
// All Rights Reserved
// {fmt} module.

#include <bit>
#include <chrono>
#include <exception>
#include <iterator>
#include <locale>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <gmock/gmock.h>

#if (__has_include(<fcntl.h>) || defined(__APPLE__) || \
     defined(__linux__)) &&                              \
    (!defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP))
#  include <fcntl.h>
#  define FMT_USE_FCNTL 1
#else
#  define FMT_USE_FCNTL 0
#endif
#if defined(_WIN32) && !defined(__MINGW32__)
#  define FMT_POSIX(call) _##call
#else
#  define FMT_POSIX(call) call
#endif

import fmt;

#define FMT_OS_H_  // don't pull in os.h, neither directly nor indirectly
#include "gtest-extra.h"

// an implicitly exported namespace must be visible [module.interface]/2.2
TEST(module_test, namespace) {
  using namespace fmt;
  using namespace fmt::literals;
  ASSERT_TRUE(true);
}

// Macros must not be imported from a named module [cpp.import]/5.1.
TEST(module_test, macros) {
#if defined(FMT_BASE_H_) || defined(FMT_FORMAT_H_)
  FAIL() << "Macros are leaking from a named module";
#endif
}

// The following is less about functional testing (that's done elsewhere)
// but rather visibility of all client-facing overloads, reachability of
// non-exported entities, name lookup and overload resolution within
// template instantitions.
// Exercise all exported entities of the API at least once.
// Instantiate as many code paths as possible.

TEST(module_test, to_string) {
  EXPECT_EQ("42", fmt::to_string(42));
  EXPECT_EQ("42", fmt::to_string(42.0));

  EXPECT_EQ(L"42", fmt::to_wstring(42));
  EXPECT_EQ(L"42", fmt::to_wstring(42.0));
}

TEST(module_test, format) {
  EXPECT_EQ("42", fmt::format("{:}", 42));
  EXPECT_EQ("-42", fmt::format("{0}", -42.0));

  EXPECT_EQ(L"42", fmt::format(L"{:}", 42));
  EXPECT_EQ(L"-42", fmt::format(L"{0}", -42.0));
}

TEST(module_test, format_to) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "{}", 42);
  EXPECT_EQ("42", s);

  char buffer[4] = {0};
  fmt::format_to(buffer, "{}", 42);
  EXPECT_EQ("42", std::string_view(buffer));

  fmt::memory_buffer mb;
  fmt::format_to(std::back_inserter(mb), "{}", 42);
  EXPECT_EQ("42", std::string_view(buffer));

  std::wstring w;
  fmt::format_to(std::back_inserter(w), L"{}", 42);
  EXPECT_EQ(L"42", w);

  wchar_t wbuffer[4] = {0};
  fmt::format_to(wbuffer, L"{}", 42);
  EXPECT_EQ(L"42", std::wstring_view(wbuffer));

  fmt::wmemory_buffer wb;
  fmt::format_to(std::back_inserter(wb), L"{}", 42);
  EXPECT_EQ(L"42", std::wstring_view(wbuffer));
}

TEST(module_test, formatted_size) {
  EXPECT_EQ(2u, fmt::formatted_size("{}", 42));
  EXPECT_EQ(2u, fmt::formatted_size(L"{}", 42));
}

TEST(module_test, format_to_n) {
  std::string s;
  auto result = fmt::format_to_n(std::back_inserter(s), 1, "{}", 42);
  EXPECT_EQ(2u, result.size);
  char buffer[4] = {0};
  fmt::format_to_n(buffer, 3, "{}", 12345);

  std::wstring w;
  auto wresult = fmt::format_to_n(std::back_inserter(w), 1, L"{}", 42);
  EXPECT_EQ(2u, wresult.size);
  wchar_t wbuffer[4] = {0};
  fmt::format_to_n(wbuffer, 3, L"{}", 12345);
}

TEST(module_test, format_args) {
  int n = 42;
  auto store = fmt::make_format_args(n);
  fmt::format_args args = store;
  EXPECT_TRUE(args.get(0));
  EXPECT_FALSE(args.get(1));
}

TEST(module_test, vformat) {
  int n = 42;
  EXPECT_EQ("42", fmt::vformat("{}", fmt::make_format_args(n)));
}

TEST(module_test, vformat_to) {
  int n = 42;
  auto store = fmt::make_format_args(n);
  std::string s;
  fmt::vformat_to(std::back_inserter(s), "{}", store);
  EXPECT_EQ("42", s);
}

TEST(module_test, vformat_to_n) {
  int n = 12345;
  auto store = fmt::make_format_args(n);
  std::string s;
  fmt::vformat_to_n(std::back_inserter(s), 1, "{}", store);
  char buffer[4] = {0};
  fmt::vformat_to_n(buffer, 3, "{:}", store);
}

std::string as_string(std::wstring_view text) {
  return {reinterpret_cast<const char*>(text.data()),
          text.size() * sizeof(text[0])};
}

TEST(module_test, print) {
  EXPECT_WRITE(stdout, fmt::print("{}µ", 42), "42µ");
  EXPECT_WRITE(stderr, fmt::print(stderr, "{}µ", 4.2), "4.2µ");
}

TEST(module_test, vprint) {
  int n = 42;
  double m = 4.2;
  EXPECT_WRITE(stdout, fmt::vprint("{:}µ", fmt::make_format_args(n)), "42µ");
  EXPECT_WRITE(stderr, fmt::vprint(stderr, "{}", fmt::make_format_args(m)),
               "4.2");
}

TEST(module_test, named_args) {
  EXPECT_EQ("42", fmt::format("{answer}", fmt::arg("answer", 42)));
  EXPECT_EQ(L"42", fmt::format(L"{answer}", fmt::arg(L"answer", 42)));
}

TEST(module_test, literals) {
  using namespace fmt::literals;
  EXPECT_EQ("42", fmt::format("{answer}", "answer"_a = 42));
  EXPECT_EQ(L"42", fmt::format(L"{answer}", L"answer"_a = 42));
}

TEST(module_test, locale) {
  const double m = 4.2;
  auto store = fmt::make_format_args(m);
  const auto classic = std::locale::classic();
  EXPECT_EQ("4.2", fmt::format(classic, "{:L}", 4.2));
  EXPECT_EQ("4.2", fmt::vformat(classic, "{:L}", store));
  std::string s;
  fmt::vformat_to(std::back_inserter(s), classic, "{:L}", store);
  EXPECT_EQ("4.2", s);
  EXPECT_EQ("4.2", fmt::format("{:L}", 4.2));
}

TEST(module_test, string_view) {
  fmt::string_view nsv("fmt");
  EXPECT_EQ("fmt", nsv);
  EXPECT_TRUE(fmt::string_view("fmt") == nsv);

  fmt::wstring_view wsv(L"fmt");
  EXPECT_EQ(L"fmt", wsv);
  EXPECT_TRUE(fmt::wstring_view(L"fmt") == wsv);
}

TEST(module_test, memory_buffer) {
  fmt::basic_memory_buffer<char, fmt::inline_buffer_size> buffer;
  fmt::format_to(std::back_inserter(buffer), "{}", "42");
  EXPECT_EQ("42", to_string(buffer));
  fmt::memory_buffer nbuffer(std::move(buffer));
  EXPECT_EQ("42", to_string(nbuffer));
  buffer = std::move(nbuffer);
  EXPECT_EQ("42", to_string(buffer));
  nbuffer.clear();
  EXPECT_EQ(0u, to_string(nbuffer).size());
}

TEST(module_test, ptr) {
  uintptr_t answer = 42;
  auto p = std::bit_cast<int*>(answer);
  EXPECT_EQ("0x2a", fmt::to_string(fmt::ptr(p)));
}

TEST(module_test, errors) {
  int n = 42;
  auto store = fmt::make_format_args(n);
  EXPECT_THROW(throw fmt::format_error("oops"), std::exception);
  EXPECT_THROW(throw fmt::vsystem_error(0, "{}", store), std::system_error);
  EXPECT_THROW(throw fmt::system_error(0, "{}", 42), std::system_error);

  fmt::memory_buffer buffer;
  fmt::format_system_error(buffer, 0, "oops");
  auto oops = to_string(buffer);
  EXPECT_TRUE(oops.size() > 0);
  EXPECT_WRITE(stderr, fmt::report_system_error(0, "oops"), oops + '\n');

#ifdef _WIN32
  EXPECT_THROW(throw fmt::vwindows_error(0, "{}", store), std::system_error);
  EXPECT_THROW(throw fmt::windows_error(0, "{}", 42), std::system_error);
  output_redirect redirect(stderr);
  fmt::report_windows_error(0, "oops");
  EXPECT_TRUE(redirect.restore_and_read().size() > 0);
#endif
}

TEST(module_test, error_code) {
  EXPECT_EQ("generic:42",
            fmt::format("{0}", std::error_code(42, std::generic_category())));
  EXPECT_EQ("system:42",
            fmt::format("{0}", std::error_code(42, fmt::system_category())));
}

TEST(module_test, format_int) {
  fmt::format_int sanswer(42);
  EXPECT_EQ("42", fmt::string_view(sanswer.data(), sanswer.size()));
  fmt::format_int uanswer(42u);
  EXPECT_EQ("42", fmt::string_view(uanswer.data(), uanswer.size()));
}

struct test_formatter : fmt::formatter<char> {
  bool check() { return true; }
};

TEST(module_test, formatter) { EXPECT_TRUE(test_formatter{}.check()); }

TEST(module_test, join) {
  int arr[3] = {1, 2, 3};
  std::vector<double> vec{1.0, 2.0, 3.0};
  auto sep = fmt::string_view(", ");
  EXPECT_EQ("1, 2, 3", to_string(fmt::join(arr + 0, arr + 3, sep)));
  EXPECT_EQ("1, 2, 3", to_string(fmt::join(arr, sep)));
  EXPECT_EQ("1, 2, 3", to_string(fmt::join(vec.begin(), vec.end(), sep)));
  EXPECT_EQ("1, 2, 3", to_string(fmt::join(vec, sep)));

  auto wsep = fmt::wstring_view(L", ");
  EXPECT_EQ(L"1, 2, 3", fmt::format(L"{}", fmt::join(arr + 0, arr + 3, wsep)));
  EXPECT_EQ(L"1, 2, 3", fmt::format(L"{}", fmt::join(arr, wsep)));
}

TEST(module_test, time) {
  auto time_now = std::time(nullptr);
  EXPECT_TRUE(fmt::gmtime(time_now).tm_year > 120);
  auto chrono_now = std::chrono::system_clock::now();
  EXPECT_TRUE(fmt::gmtime(chrono_now).tm_year > 120);
}

TEST(module_test, time_point) {
  auto now = std::chrono::system_clock::now();
  std::string_view past("2021-05-20 10:30:15");
  EXPECT_TRUE(past < fmt::format("{:%Y-%m-%d %H:%M:%S}", now));
  std::wstring_view wpast(L"2021-05-20 10:30:15");
  EXPECT_TRUE(wpast < fmt::format(L"{:%Y-%m-%d %H:%M:%S}", now));
}

TEST(module_test, time_duration) {
  using us = std::chrono::duration<double, std::micro>;
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::seconds{42}));
  EXPECT_EQ("4.2µs", fmt::format("{:3.1}", us{4.234}));
  EXPECT_EQ("4.2µs", fmt::format(std::locale::classic(), "{:L}", us{4.2}));

  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds{42}));
  EXPECT_EQ(L"4.2µs", fmt::format(L"{:3.1}", us{4.234}));
  EXPECT_EQ(L"4.2µs", fmt::format(std::locale::classic(), L"{:L}", us{4.2}));
}

TEST(module_test, weekday) {
  EXPECT_EQ("Mon", fmt::format(std::locale::classic(), "{}", fmt::weekday(1)));
}

TEST(module_test, printf) {
  EXPECT_WRITE(stdout, fmt::printf("%f", 42.123456), "42.123456");
  EXPECT_WRITE(stdout, fmt::printf("%d", 42), "42");
}

TEST(module_test, fprintf) {
  EXPECT_WRITE(stderr, fmt::fprintf(stderr, "%d", 42), "42");
}

TEST(module_test, sprintf) {
  EXPECT_EQ("42", fmt::sprintf("%d", 42));
}

TEST(module_test, color) {
  EXPECT_EQ("\x1B[30m42\x1B[0m",
            fmt::format(fg(fmt::terminal_color::black), "{}", 42));
  EXPECT_EQ(L"\x1B[30m42\x1B[0m",
            fmt::format(fg(fmt::terminal_color::black), L"{}", 42));
}

TEST(module_test, cstring_view) {
  auto s = "fmt";
  EXPECT_EQ(s, fmt::cstring_view(s).c_str());
  auto w = L"fmt";
  EXPECT_EQ(w, fmt::wcstring_view(w).c_str());
}

TEST(module_test, buffered_file) {
  EXPECT_TRUE(fmt::buffered_file{}.get() == nullptr);
}

TEST(module_test, output_file) {
#ifdef __clang__
  fmt::println("\033[0;33m[=disabled=] {}\033[0;0m",
               "Clang 16.0 emits multiple copies of vtables");
#else
  fmt::ostream out = fmt::output_file("module-test", fmt::buffer_size = 1);
  out.close();
#endif
}

struct custom_context {
  using char_type = char;
  using parse_context_type = fmt::format_parse_context;
};

TEST(module_test, custom_context) {
  fmt::basic_format_arg<custom_context> custom_arg;
  EXPECT_TRUE(!custom_arg);
}

TEST(module_test, compile_format_string) {
  using namespace fmt::literals;
#ifdef __clang__
  fmt::println("\033[0;33m[=disabled=] {}\033[0;0m",
               "Clang 16.0 fails to import user-defined literals");
#else
  EXPECT_EQ("42", fmt::format("{0:x}"_cf, 0x42));
  EXPECT_EQ(L"42", fmt::format(L"{:}"_cf, 42));
  EXPECT_EQ("4.2", fmt::format("{arg:3.1f}"_cf, "arg"_a = 4.2));
  EXPECT_EQ(L" 42", fmt::format(L"{arg:>3}"_cf, L"arg"_a = L"42"));
#endif
}
