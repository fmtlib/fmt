// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_STRING_ALIAS 1
#include "fmt/ostream.h"

#include <sstream>
#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::format;
using fmt::format_error;

static std::ostream &operator<<(std::ostream &os, const Date &d) {
  os << d.year() << '-' << d.month() << '-' << d.day();
  return os;
}

static std::wostream &operator<<(std::wostream &os, const Date &d) {
  os << d.year() << L'-' << d.month() << L'-' << d.day();
  return os;
}

enum TestEnum {};
static std::ostream &operator<<(std::ostream &os, TestEnum) {
  return os << "TestEnum";
}

static std::wostream &operator<<(std::wostream &os, TestEnum) {
  return os << L"TestEnum";
}

enum TestEnum2 {A};

TEST(OStreamTest, Enum) {
  EXPECT_FALSE((fmt::convert_to_int<TestEnum, char>::value));
  EXPECT_EQ("TestEnum", fmt::format("{}", TestEnum()));
  EXPECT_EQ("0", fmt::format("{}", A));
  EXPECT_FALSE((fmt::convert_to_int<TestEnum, wchar_t>::value));
  EXPECT_EQ(L"TestEnum", fmt::format(L"{}", TestEnum()));
  EXPECT_EQ(L"0", fmt::format(L"{}", A));
}

typedef fmt::back_insert_range<fmt::internal::buffer> range;

struct test_arg_formatter: fmt::arg_formatter<range> {
  test_arg_formatter(fmt::format_context &ctx, fmt::format_specs &s)
    : fmt::arg_formatter<range>(ctx, &s) {}
};

TEST(OStreamTest, CustomArg) {
  fmt::memory_buffer buffer;
  fmt::internal::buffer &base = buffer;
  fmt::format_context ctx(std::back_inserter(base), "", fmt::format_args());
  fmt::format_specs spec;
  test_arg_formatter af(ctx, spec);
  visit(af, fmt::internal::make_arg<fmt::format_context>(TestEnum()));
  EXPECT_EQ("TestEnum", std::string(buffer.data(), buffer.size()));
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
  EXPECT_THROW_MSG(format("{0:=5}", TestString("def")),
      format_error, "format specifier requires numeric argument");
  EXPECT_EQ(" def ", format("{0:^5}", TestString("def")));
  EXPECT_EQ("def**", format("{0:*<5}", TestString("def")));
  EXPECT_THROW_MSG(format("{0:+}", TestString()),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", TestString()),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", TestString()),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", TestString()),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", TestString()),
      format_error, "format specifier requires numeric argument");
  EXPECT_EQ("test         ", format("{0:13}", TestString("test")));
  EXPECT_EQ("test         ", format("{0:{1}}", TestString("test"), 13));
  EXPECT_EQ("te", format("{0:.2}", TestString("test")));
  EXPECT_EQ("te", format("{0:.{1}}", TestString("test"), 2));
}

struct EmptyTest {};
static std::ostream &operator<<(std::ostream &os, EmptyTest) {
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
  const char *foo = "foo";
  buffer.append(foo, foo + std::strlen(foo));
  fmt::internal::write(os, buffer);
  EXPECT_EQ("foo", os.str());
}

TEST(OStreamTest, WriteToOStreamMaxSize) {
  std::size_t max_size = std::numeric_limits<std::size_t>::max();
  std::streamsize max_streamsize = std::numeric_limits<std::streamsize>::max();
  if (max_size <= fmt::internal::to_unsigned(max_streamsize))
    return;

  struct test_buffer : fmt::internal::buffer {
    explicit test_buffer(std::size_t size) { resize(size); }
    void grow(std::size_t) {}
  } buffer(max_size);

  struct mock_streambuf : std::streambuf {
    MOCK_METHOD2(xsputn, std::streamsize (const void *s, std::streamsize n));
    std::streamsize xsputn(const char *s, std::streamsize n) {
      const void *v = s;
      return xsputn(v, n);
    }
  } streambuf;

  struct test_ostream : std::ostream {
    explicit test_ostream(mock_streambuf &buffer) : std::ostream(&buffer) {}
  } os(streambuf);

  testing::InSequence sequence;
  const char *data = FMT_NULL;
  std::size_t size = max_size;
  do {
    typedef std::make_unsigned<std::streamsize>::type ustreamsize;
    ustreamsize n = std::min<ustreamsize>(
          size, fmt::internal::to_unsigned(max_streamsize));
    EXPECT_CALL(streambuf, xsputn(data, static_cast<std::streamsize>(n)))
        .WillOnce(testing::Return(max_streamsize));
    data += n;
    size -= n;
  } while (size != 0);
  fmt::internal::write(os, buffer);
}

TEST(OStreamTest, Join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ("1, 2, 3", fmt::format("{}", fmt::join(v, v + 3, ", ")));
}

#if FMT_USE_CONSTEXPR
TEST(OStreamTest, ConstexprString) {
  EXPECT_EQ("42", format(fmt("{}"), std::string("42")));
}
#endif

namespace fmt_test {
struct ABC {};

template <typename Output> Output &operator<<(Output &out, ABC) {
  out << "ABC";
  return out;
}
} // namespace fmt_test

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

#if FMT_USE_USER_DEFINED_LITERALS
TEST(FormatTest, UDL) {
  using namespace fmt::literals;
  EXPECT_EQ("{}"_format("test"), "test");
}
#endif
