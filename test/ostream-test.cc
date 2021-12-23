// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format.h"

using fmt::runtime;

struct test {};

// Test that there is no issues with specializations when fmt/ostream.h is
// included after fmt/format.h.
namespace fmt {
template <> struct formatter<test> : formatter<int> {
  auto format(const test&, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
}  // namespace fmt

#include <sstream>

#include "fmt/compile.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"
#include "gmock/gmock.h"
#include "gtest-extra.h"
#include "util.h"

std::ostream& operator<<(std::ostream& os, const date& d) {
  os << d.year() << '-' << d.month() << '-' << d.day();
  return os;
}

std::wostream& operator<<(std::wostream& os, const date& d) {
  os << d.year() << L'-' << d.month() << L'-' << d.day();
  return os;
}

// Make sure that overloaded comma operators do no harm to is_streamable.
struct type_with_comma_op {};
template <typename T> void operator,(type_with_comma_op, const T&);
template <typename T> type_with_comma_op operator<<(T&, const date&);

enum streamable_enum {};

std::ostream& operator<<(std::ostream& os, streamable_enum) {
  return os << "streamable_enum";
}

enum unstreamable_enum {};

TEST(ostream_test, enum) {
  EXPECT_EQ("streamable_enum", fmt::format("{}", streamable_enum()));
  EXPECT_EQ("0", fmt::format("{}", unstreamable_enum()));
}

TEST(ostream_test, format) {
  EXPECT_EQ("a string", fmt::format("{0}", test_string("a string")));
  EXPECT_EQ("The date is 2012-12-9",
            fmt::format("The date is {0}", date(2012, 12, 9)));
}

TEST(ostream_test, format_specs) {
  using fmt::format_error;
  EXPECT_EQ("def  ", fmt::format("{0:<5}", test_string("def")));
  EXPECT_EQ("  def", fmt::format("{0:>5}", test_string("def")));
  EXPECT_EQ(" def ", fmt::format("{0:^5}", test_string("def")));
  EXPECT_EQ("def**", fmt::format("{0:*<5}", test_string("def")));
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:+}"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:-}"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0: }"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:#}"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{0:05}"), test_string()),
                   format_error, "format specifier requires numeric argument");
  EXPECT_EQ("test         ", fmt::format("{0:13}", test_string("test")));
  EXPECT_EQ("test         ", fmt::format("{0:{1}}", test_string("test"), 13));
  EXPECT_EQ("te", fmt::format("{0:.2}", test_string("test")));
  EXPECT_EQ("te", fmt::format("{0:.{1}}", test_string("test"), 2));
}

struct empty_test {};
std::ostream& operator<<(std::ostream& os, empty_test) { return os << ""; }

TEST(ostream_test, empty_custom_output) {
  EXPECT_EQ("", fmt::format("{}", empty_test()));
}

TEST(ostream_test, print) {
  std::ostringstream os;
  fmt::print(os, "Don't {}!", "panic");
  EXPECT_EQ("Don't panic!", os.str());
}

TEST(ostream_test, write_to_ostream) {
  std::ostringstream os;
  fmt::memory_buffer buffer;
  const char* foo = "foo";
  buffer.append(foo, foo + std::strlen(foo));
  fmt::detail::write_buffer(os, buffer);
  EXPECT_EQ("foo", os.str());
}

TEST(ostream_test, write_to_ostream_max_size) {
  auto max_size = fmt::detail::max_value<size_t>();
  auto max_streamsize = fmt::detail::max_value<std::streamsize>();
  if (max_size <= fmt::detail::to_unsigned(max_streamsize)) return;

  struct test_buffer final : fmt::detail::buffer<char> {
    explicit test_buffer(size_t size)
        : fmt::detail::buffer<char>(nullptr, size, size) {}
    void grow(size_t) override {}
  } buffer(max_size);

  struct mock_streambuf : std::streambuf {
    MOCK_METHOD2(xsputn, std::streamsize(const void* s, std::streamsize n));
    std::streamsize xsputn(const char* s, std::streamsize n) override {
      const void* v = s;
      return xsputn(v, n);
    }
  } streambuf;

  struct test_ostream : std::ostream {
    explicit test_ostream(mock_streambuf& output_buffer)
        : std::ostream(&output_buffer) {}
  } os(streambuf);

  testing::InSequence sequence;
  const char* data = nullptr;
  using ustreamsize = std::make_unsigned<std::streamsize>::type;
  ustreamsize size = max_size;
  do {
    auto n = std::min(size, fmt::detail::to_unsigned(max_streamsize));
    EXPECT_CALL(streambuf, xsputn(data, static_cast<std::streamsize>(n)))
        .WillOnce(testing::Return(max_streamsize));
    data += n;
    size -= n;
  } while (size != 0);
  fmt::detail::write_buffer(os, buffer);
}

TEST(ostream_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ("1, 2, 3", fmt::format("{}", fmt::join(v, v + 3, ", ")));
}

TEST(ostream_test, join_fallback_formatter) {
  auto strs = std::vector<test_string>{test_string("foo"), test_string("bar")};
  EXPECT_EQ("foo, bar", fmt::format("{}", fmt::join(strs, ", ")));
}

#if FMT_USE_CONSTEXPR
TEST(ostream_test, constexpr_string) {
  EXPECT_EQ("42", format(FMT_STRING("{}"), std::string("42")));
  EXPECT_EQ("a string", format(FMT_STRING("{0}"), test_string("a string")));
}
#endif

namespace fmt_test {
struct abc {};

template <typename Output> Output& operator<<(Output& out, abc) {
  return out << "abc";
}
}  // namespace fmt_test

template <typename T> struct test_template {};

template <typename T>
std::ostream& operator<<(std::ostream& os, test_template<T>) {
  return os << 1;
}

namespace fmt {
template <typename T> struct formatter<test_template<T>> : formatter<int> {
  auto format(test_template<T>, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(2, ctx);
  }
};
}  // namespace fmt

TEST(ostream_test, template) {
  EXPECT_EQ("2", fmt::format("{}", test_template<int>()));
}

TEST(ostream_test, format_to_n) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, "{}", fmt_test::abc());
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("abcx", fmt::string_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, "x{}y", fmt_test::abc());
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("xabx", fmt::string_view(buffer, 4));
}

template <typename T> struct convertible {
  T value;
  explicit convertible(const T& val) : value(val) {}
  operator T() const { return value; }
};

TEST(ostream_test, disable_builtin_ostream_operators) {
  EXPECT_EQ("42", fmt::format("{:d}", convertible<unsigned short>(42)));
  EXPECT_EQ("foo", fmt::format("{}", convertible<const char*>("foo")));
}

struct explicitly_convertible_to_string_like {
  template <typename String,
            typename = typename std::enable_if<std::is_constructible<
                String, const char*, size_t>::value>::type>
  explicit operator String() const {
    return String("foo", 3u);
  }
};

std::ostream& operator<<(std::ostream& os,
                         explicitly_convertible_to_string_like) {
  return os << "bar";
}

TEST(ostream_test, format_explicitly_convertible_to_string_like) {
  EXPECT_EQ("bar", fmt::format("{}", explicitly_convertible_to_string_like()));
}

#ifdef FMT_USE_STRING_VIEW
struct explicitly_convertible_to_std_string_view {
  explicit operator fmt::detail::std_string_view<char>() const {
    return {"foo", 3u};
  }
};

std::ostream& operator<<(std::ostream& os,
                         explicitly_convertible_to_std_string_view) {
  return os << "bar";
}

TEST(ostream_test, format_explicitly_convertible_to_std_string_view) {
  EXPECT_EQ("bar", fmt::format("{}", explicitly_convertible_to_string_like()));
}
#endif  // FMT_USE_STRING_VIEW

struct streamable_and_convertible_to_bool {
  operator bool() const { return true; }
};

std::ostream& operator<<(std::ostream& os, streamable_and_convertible_to_bool) {
  return os << "foo";
}

TEST(ostream_test, format_convertible_to_bool) {
  // operator<< is intentionally not used because of potential ODR violations.
  EXPECT_EQ(fmt::format("{}", streamable_and_convertible_to_bool()), "true");
}

struct copyfmt_test {};

std::ostream& operator<<(std::ostream& os, copyfmt_test) {
  std::ios ios(nullptr);
  ios.copyfmt(os);
  return os << "foo";
}

TEST(ostream_test, copyfmt) {
  EXPECT_EQ("foo", fmt::format("{}", copyfmt_test()));
}

TEST(ostream_test, to_string) {
  EXPECT_EQ("abc", fmt::to_string(fmt_test::abc()));
}

TEST(ostream_test, range) {
  auto strs = std::vector<test_string>{test_string("foo"), test_string("bar")};
  EXPECT_EQ("[foo, bar]", fmt::format("{}", strs));
}

struct abstract {
  virtual ~abstract() = default;
  virtual void f() = 0;
  friend std::ostream& operator<<(std::ostream& os, const abstract&) {
    return os;
  }
};

void format_abstract_compiles(const abstract& a) {
  fmt::format(FMT_COMPILE("{}"), a);
}

TEST(ostream_test, is_formattable) {
  EXPECT_TRUE(fmt::is_formattable<std::string>());
  EXPECT_TRUE(fmt::is_formattable<fmt::detail::std_string_view<char>>());
}
