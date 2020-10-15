// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_STRING_ALIAS 1
#include "fmt/format.h"

struct test {};

// Test that there is no issues with specializations when fmt/ostream.h is
// included after fmt/format.h.
namespace fmt {
template <> struct formatter<test> : formatter<int> {
  template <typename FormatContext>
  typename FormatContext::iterator format(const test&, FormatContext& ctx) {
    return formatter<int>::format(42, ctx);
  }
};
}  // namespace fmt

#include <sstream>

#include "fmt/ostream.h"
#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::format;
using fmt::format_error;

static std::ostream& operator<<(std::ostream& os, const Date& d) {
  os << d.year() << '-' << d.month() << '-' << d.day();
  return os;
}

static std::wostream& operator<<(std::wostream& os, const Date& d) {
  os << d.year() << L'-' << d.month() << L'-' << d.day();
  return os;
}

// Make sure that overloaded comma operators do no harm to is_streamable.
struct type_with_comma_op {};
template <typename T> void operator,(type_with_comma_op, const T&);
template <typename T> type_with_comma_op operator<<(T&, const Date&);

enum streamable_enum {};
static std::ostream& operator<<(std::ostream& os, streamable_enum) {
  return os << "streamable_enum";
}

static std::wostream& operator<<(std::wostream& os, streamable_enum) {
  return os << L"streamable_enum";
}

enum unstreamable_enum {};

TEST(OStreamTest, Enum) {
  EXPECT_EQ("streamable_enum", fmt::format("{}", streamable_enum()));
  EXPECT_EQ("0", fmt::format("{}", unstreamable_enum()));
  EXPECT_EQ(L"streamable_enum", fmt::format(L"{}", streamable_enum()));
  EXPECT_EQ(L"0", fmt::format(L"{}", unstreamable_enum()));
}

struct test_arg_formatter
    : fmt::detail::arg_formatter<fmt::format_context::iterator, char> {
  fmt::format_parse_context parse_ctx;
  test_arg_formatter(fmt::format_context& ctx, fmt::format_specs& s)
      : fmt::detail::arg_formatter<fmt::format_context::iterator, char>(
            ctx, &parse_ctx, &s),
        parse_ctx("") {}
};

TEST(OStreamTest, CustomArg) {
  fmt::memory_buffer buffer;
  fmt::format_context ctx(fmt::detail::buffer_appender<char>{buffer},
                          fmt::format_args());
  fmt::format_specs spec;
  test_arg_formatter af(ctx, spec);
  fmt::visit_format_arg(
      af, fmt::detail::make_arg<fmt::format_context>(streamable_enum()));
  EXPECT_EQ("streamable_enum", std::string(buffer.data(), buffer.size()));
}

TEST(OStreamTest, Format) {
  EXPECT_EQ("a string", format("{0}", TestString("a string")));
  std::string s = format("The date is {0}", Date(2012, 12, 9));
  EXPECT_EQ("The date is 2012-12-9", s);
  Date date(2012, 12, 9);
  EXPECT_EQ(L"The date is 2012-12-9",
            format(L"The date is {0}", Date(2012, 12, 9)));
}

TEST(OStreamTest, FormatSpecs) {
  EXPECT_EQ("def  ", format("{0:<5}", TestString("def")));
  EXPECT_EQ("  def", format("{0:>5}", TestString("def")));
#if FMT_DEPRECATED_NUMERIC_ALIGN
  EXPECT_THROW_MSG(format("{0:=5}", TestString("def")), format_error,
                   "format specifier requires numeric argument");
#endif
  EXPECT_EQ(" def ", format("{0:^5}", TestString("def")));
  EXPECT_EQ("def**", format("{0:*<5}", TestString("def")));
  EXPECT_THROW_MSG(format("{0:+}", TestString()), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", TestString()), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", TestString()), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", TestString()), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", TestString()), format_error,
                   "format specifier requires numeric argument");
  EXPECT_EQ("test         ", format("{0:13}", TestString("test")));
  EXPECT_EQ("test         ", format("{0:{1}}", TestString("test"), 13));
  EXPECT_EQ("te", format("{0:.2}", TestString("test")));
  EXPECT_EQ("te", format("{0:.{1}}", TestString("test"), 2));
}

struct EmptyTest {};
static std::ostream& operator<<(std::ostream& os, EmptyTest) {
  return os << "";
}

TEST(OStreamTest, EmptyCustomOutput) {
  EXPECT_EQ("", fmt::format("{}", EmptyTest()));
}

TEST(OStreamTest, Print) {
  std::ostringstream os;
  fmt::print(os, "Don't {}!", "panic");
  EXPECT_EQ("Don't panic!", os.str());
  std::wostringstream wos;
  fmt::print(wos, L"Don't {}!", L"panic");
  EXPECT_EQ(L"Don't panic!", wos.str());
}

TEST(OStreamTest, WriteToOStream) {
  std::ostringstream os;
  fmt::memory_buffer buffer;
  const char* foo = "foo";
  buffer.append(foo, foo + std::strlen(foo));
  fmt::detail::write_buffer(os, buffer);
  EXPECT_EQ("foo", os.str());
}

TEST(OStreamTest, WriteToOStreamMaxSize) {
  size_t max_size = fmt::detail::max_value<size_t>();
  std::streamsize max_streamsize = fmt::detail::max_value<std::streamsize>();
  if (max_size <= fmt::detail::to_unsigned(max_streamsize)) return;

  struct test_buffer final : fmt::detail::buffer<char> {
    explicit test_buffer(size_t size)
      : fmt::detail::buffer<char>(nullptr, size, size) {}
    void grow(size_t) {}
  } buffer(max_size);

  struct mock_streambuf : std::streambuf {
    MOCK_METHOD2(xsputn, std::streamsize(const void* s, std::streamsize n));
    std::streamsize xsputn(const char* s, std::streamsize n) {
      const void* v = s;
      return xsputn(v, n);
    }
  } streambuf;

  struct test_ostream : std::ostream {
    explicit test_ostream(mock_streambuf& buffer) : std::ostream(&buffer) {}
  } os(streambuf);

  testing::InSequence sequence;
  const char* data = nullptr;
  typedef std::make_unsigned<std::streamsize>::type ustreamsize;
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

TEST(OStreamTest, Join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ("1, 2, 3", fmt::format("{}", fmt::join(v, v + 3, ", ")));
}

#if FMT_USE_CONSTEXPR
TEST(OStreamTest, ConstexprString) {
  EXPECT_EQ("42", format(FMT_STRING("{}"), std::string("42")));
  EXPECT_EQ("a string", format(FMT_STRING("{0}"), TestString("a string")));
}
#endif

namespace fmt_test {
struct ABC {};

template <typename Output> Output& operator<<(Output& out, ABC) {
  out << "ABC";
  return out;
}
}  // namespace fmt_test

template <typename T> struct TestTemplate {};

template <typename T>
std::ostream& operator<<(std::ostream& os, TestTemplate<T>) {
  return os << 1;
}

namespace fmt {
template <typename T> struct formatter<TestTemplate<T>> : formatter<int> {
  template <typename FormatContext>
  typename FormatContext::iterator format(TestTemplate<T>, FormatContext& ctx) {
    return formatter<int>::format(2, ctx);
  }
};
}  // namespace fmt

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 407
TEST(OStreamTest, Template) {
  EXPECT_EQ("2", fmt::format("{}", TestTemplate<int>()));
}

TEST(FormatTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, "{}", fmt_test::ABC());
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("ABCx", fmt::string_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, "x{}y", fmt_test::ABC());
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("xABx", fmt::string_view(buffer, 4));
}
#endif

#if FMT_USE_USER_DEFINED_LITERALS
TEST(FormatTest, UDL) {
  using namespace fmt::literals;
  EXPECT_EQ("{}"_format("test"), "test");
}
#endif

template <typename T> struct convertible {
  T value;
  explicit convertible(const T& val) : value(val) {}
  operator T() const { return value; }
};

TEST(OStreamTest, DisableBuiltinOStreamOperators) {
  EXPECT_EQ("42", fmt::format("{:d}", convertible<unsigned short>(42)));
  EXPECT_EQ(L"42", fmt::format(L"{:d}", convertible<unsigned short>(42)));
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

TEST(OStreamTest, FormatExplicitlyConvertibleToStringLike) {
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

TEST(OStreamTest, FormatExplicitlyConvertibleToStdStringView) {
  EXPECT_EQ("bar", fmt::format("{}", explicitly_convertible_to_string_like()));
}
#endif  // FMT_USE_STRING_VIEW

struct streamable_and_convertible_to_bool {
  operator bool() const { return true; }
};

std::ostream& operator<<(std::ostream& os, streamable_and_convertible_to_bool) {
  return os << "foo";
}

TEST(OStreamTest, FormatConvertibleToBool) {
  EXPECT_EQ("foo", fmt::format("{}", streamable_and_convertible_to_bool()));
}

struct copyfmt_test {};

std::ostream& operator<<(std::ostream& os, copyfmt_test) {
  std::ios ios(nullptr);
  ios.copyfmt(os);
  return os << "foo";
}

TEST(OStreamTest, CopyFmt) {
  EXPECT_EQ("foo", fmt::format("{}", copyfmt_test()));
}

TEST(OStreamTest, CompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
}

TEST(OStreamTest, ToString) {
  EXPECT_EQ("ABC", fmt::to_string(fmt_test::ABC()));
}
