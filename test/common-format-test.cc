// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <stdint.h>
#include <string>

// Check if fmt/prepare.h compiles with windows.h included before it.
#ifdef _WIN32
#include <windows.h>
#endif

#include "fmt/prepare.h"
#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

#undef ERROR
#undef min
#undef max

using fmt::format_error;
using fmt::string_view;

#define TEST_FMT_CHAR(S) typename fmt::internal::char_t<S>::type

namespace {
template <typename Format>
static std::basic_string<TEST_FMT_CHAR(Format)>
get_runtime_format(Format &&format_str) {
  const auto format_view = fmt::internal::to_string_view(format_str);
  return std::basic_string<TEST_FMT_CHAR(Format)>(format_view.begin(),
                                                  format_view.size());
}
}

// A wrapper that simply forwards format string as a compile string to
// fmt::format*() functions.
struct CompiletimeFormatFunctionWrapper {
  template <typename Format, typename... Args>
  static std::basic_string<TEST_FMT_CHAR(Format)>
  format(const Format &format_str, const Args &... args) {
    return fmt::format(format_str, args...);
  }

  template <typename S, typename... Args,
            std::size_t SIZE = fmt::inline_buffer_size,
            typename Char = typename fmt::internal::char_t<S>::type>
  static inline typename fmt::buffer_context<Char>::type::iterator
  format_to(fmt::basic_memory_buffer<Char, SIZE> &buf, const S &format_str,
            const Args &... args) {
    return fmt::format_to(buf, format_str, args...);
  }

  template <typename OutputIt, typename S, typename... Args>
  static typename std::enable_if<
      fmt::internal::is_string<S>::value &&
          fmt::internal::is_output_iterator<OutputIt>::value,
      OutputIt>::type
  format_to(OutputIt out, const S &format_str, const Args &... args) {
    return fmt::format_to(out, format_str, args...);
  }

  template <typename Container, typename S, typename... Args>
  static typename std::enable_if<fmt::is_contiguous<Container>::value &&
                                     fmt::internal::is_string<S>::value,
                                 std::back_insert_iterator<Container>>::type
  format_to(std::back_insert_iterator<Container> out, const S &format_str,
            const Args &... args) {
    return fmt::format_to(out, format_str, args...);
  }

  template <typename Out, typename Format, typename... Args>
  static auto format_to_n(Out out, std::size_t n, Format &&format_str,
                          const Args &... args)
      -> fmt::format_to_n_result<Out> {
    return fmt::format_to_n(std::forward<Out>(out), n, format_str, args...);
  }

  template <typename Format, typename... Args>
  static std::size_t formatted_size(const Format &format_str,
                                    const Args &... args) {
    return fmt::formatted_size(format_str, args...);
  }
};

// A wrapper that converts the format string from a compile string to a
// std::basic_string and passes it to fmt::format*() functions.
struct RuntimeFormatFunctionWrapper {
  template <typename Format, typename... Args>
  static std::basic_string<TEST_FMT_CHAR(Format)>
  format(const Format &format_str, const Args &... args) {
    return fmt::format(get_runtime_format(format_str), args...);
  }

  template <typename S, typename... Args,
            std::size_t SIZE = fmt::inline_buffer_size,
            typename Char = typename fmt::internal::char_t<S>::type>
  static inline typename fmt::buffer_context<Char>::type::iterator
  format_to(fmt::basic_memory_buffer<Char, SIZE> &buf, const S &format_str,
            const Args &... args) {
    return fmt::format_to(buf, get_runtime_format(format_str), args...);
  }

  template <typename OutputIt, typename S, typename... Args>
  static typename std::enable_if<
      fmt::internal::is_string<S>::value &&
          fmt::internal::is_output_iterator<OutputIt>::value,
      OutputIt>::type
  format_to(OutputIt out, const S &format_str, const Args &... args) {
    return fmt::format_to(out, get_runtime_format(format_str), args...);
  }

  template <typename Container, typename S, typename... Args>
  static typename std::enable_if<fmt::is_contiguous<Container>::value &&
                                     fmt::internal::is_string<S>::value,
                                 std::back_insert_iterator<Container>>::type
  format_to(std::back_insert_iterator<Container> out, const S &format_str,
            const Args &... args) {
    return fmt::format_to(out, get_runtime_format(format_str), args...);
  }

  template <typename Out, typename Format, typename... Args>
  static auto format_to_n(Out out, std::size_t n, Format &&format_str,
                          const Args &... args)
      -> fmt::format_to_n_result<Out> {
    return fmt::format_to_n(std::forward<Out>(out), n,
                            get_runtime_format(format_str), args...);
  }

  template <typename Format, typename... Args>
  static std::size_t formatted_size(const Format &format_str,
                                    const Args &... args) {
    return fmt::formatted_size(get_runtime_format(format_str), args...);
  }
};

// A wrapper that converts the format string from a compile string to a
// std::basic_string and passes it to fmt::prepare() function.
class RuntimePreparedFormatWrapper {
public:
  template <typename Format, typename... Args>
  static std::basic_string<TEST_FMT_CHAR(Format)>
  format(const Format &format_str, const Args &... args) {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.format(args...);
  }

  template <typename S, typename... Args,
            std::size_t SIZE = fmt::inline_buffer_size,
            typename Char = typename fmt::internal::char_t<S>::type>
  static typename fmt::buffer_context<Char>::type::iterator
  format_to(fmt::basic_memory_buffer<Char, SIZE> &buf, const S &format_str,
            const Args &... args) {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.format_to(buf, args...);
  }

  template <typename OutputIt, typename S, typename... Args>
  static typename std::enable_if<
      fmt::internal::is_string<S>::value &&
          fmt::internal::is_output_iterator<OutputIt>::value,
      OutputIt>::type
  format_to(OutputIt out, const S &format_str, const Args &... args) {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.format_to(out, args...);
  }

  template <typename Container, typename S, typename... Args>
  static typename std::enable_if<fmt::is_contiguous<Container>::value &&
                                     fmt::internal::is_string<S>::value,
                                 std::back_insert_iterator<Container>>::type
  format_to(std::back_insert_iterator<Container> out, const S &format_str,
            const Args &... args) {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.format_to(out, args...);
  }

  template <typename Out, typename Format, typename... Args>
  static auto format_to_n(Out out, std::size_t n, const Format &format_str,
                          const Args &... args)
      -> fmt::format_to_n_result<Out> {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.format_to_n(std::forward<Out>(out),
                                 static_cast<unsigned>(n), args...);
  }

  template <typename Format, typename... Args>
  static std::size_t formatted_size(const Format &format_str,
                                    const Args &... args) {
    auto formatter = fmt::prepare<Args...>(get_runtime_format(format_str));
    return formatter.formatted_size(args...);
  }
};

// A wrapper that simply forwards format string as a compile string to
// fmt::prepare() function.
struct CompiletimePreparedFormatWrapper {
  template <typename Format, typename... Args>
  static std::basic_string<TEST_FMT_CHAR(Format)>
  format(const Format &format_str, const Args &... args) {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.format(args...);
  }

  template <typename S, typename... Args,
            std::size_t SIZE = fmt::inline_buffer_size,
            typename Char = typename fmt::internal::char_t<S>::type>
  static typename fmt::buffer_context<Char>::type::iterator
  format_to(fmt::basic_memory_buffer<Char, SIZE> &buf, const S &format_str,
            const Args &... args) {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.format_to(buf, args...);
  }

  template <typename OutputIt, typename S, typename... Args>
  static typename std::enable_if<
      fmt::internal::is_string<S>::value &&
          fmt::internal::is_output_iterator<OutputIt>::value,
      OutputIt>::type
  format_to(OutputIt out, const S &format_str, const Args &... args) {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.format_to(out, args...);
  }

  template <typename Container, typename S, typename... Args>
  static typename std::enable_if<fmt::is_contiguous<Container>::value &&
                                     fmt::internal::is_string<S>::value,
                                 std::back_insert_iterator<Container>>::type
  format_to(std::back_insert_iterator<Container> out, const S &format_str,
            const Args &... args) {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.format_to(out, args...);
  }

  template <typename Format, typename... Args>
  static std::size_t formatted_size(const Format &format_str,
                                    const Args &... args) {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.formatted_size(args...);
  }

  template <typename Out, typename Format, typename... Args>
  static auto format_to_n(Out out, std::size_t n, const Format &format_str,
                          const Args &... args)
      -> fmt::format_to_n_result<Out> {
    auto formatter = fmt::prepare<Args...>(format_str);
    return formatter.format_to_n(std::forward<Out>(out),
                                 static_cast<unsigned>(n), args...);
  }
};

#if FMT_USE_CONSTEXPR
// We could make a typedef for it, but the GCC 4.4 has problem to compile a
// TYPED_TEST_CASE with a typename: TYPED_TEST_CASE(Test, typename Foo::type)
#define ALL_WRAPPERS                                                           \
  ::testing::Types<CompiletimeFormatFunctionWrapper, \
RuntimeFormatFunctionWrapper,                                                  \
                   \
RuntimePreparedFormatWrapper,                                                  \
                   \
CompiletimePreparedFormatWrapper>

#define FMT(s) FMT_STRING(s)

#else
#define ALL_WRAPPERS                                                           \
  ::testing::Types<RuntimeFormatFunctionWrapper, \
RuntimePreparedFormatWrapper>

#define FMT(s) s
#endif

#define RUNTIME_WRAPPERS                                                       \
  ::testing::Types<RuntimeFormatFunctionWrapper, \
RuntimePreparedFormatWrapper>

#define FORMAT_FUNCTION_WRAPPERS                                               \
  ::testing::Types<CompiletimeFormatFunctionWrapper,                           \
                   RuntimeFormatFunctionWrapper>
#define RUNTIME_FORMAT_FUNCTION_WRAPPERS                                       \
  ::testing::Types<RuntimeFormatFunctionWrapper>

template <typename T> class FormatToTest : public ::testing::Test {};
TYPED_TEST_CASE(FormatToTest, ALL_WRAPPERS);

template <typename T> class FormatterTest : public ::testing::Test {};
TYPED_TEST_CASE(FormatterTest, ALL_WRAPPERS);

template <typename T> class RuntimeFormattersTest : public ::testing::Test {};
TYPED_TEST_CASE(RuntimeFormattersTest, RUNTIME_WRAPPERS);

template <typename T> class FormatterThrowTest : public ::testing::Test {};
TYPED_TEST_CASE(FormatterThrowTest, RUNTIME_WRAPPERS);

template <typename T> class FormatFunctionTest : public ::testing::Test {};
TYPED_TEST_CASE(FormatFunctionTest, FORMAT_FUNCTION_WRAPPERS);

template <typename T>
class RuntimeFormatFunctionTest : public ::testing::Test {};
TYPED_TEST_CASE(RuntimeFormatFunctionTest, RUNTIME_FORMAT_FUNCTION_WRAPPERS);

TYPED_TEST(FormatToTest, FormatWithoutArgs) {
  std::string s;
  TypeParam::format_to(std::back_inserter(s), FMT("test"));
  EXPECT_EQ("test", s);
}

TYPED_TEST(FormatToTest, MultipleFormatToBackOfContainer) {
  std::string s;
  TypeParam::format_to(std::back_inserter(s), FMT("part{0}"), 1);
  EXPECT_EQ("part1", s);
  TypeParam::format_to(std::back_inserter(s), FMT("part{0}"), 2);
  EXPECT_EQ("part1part2", s);
}

TYPED_TEST(FormatToTest, WideString) {
  std::vector<wchar_t> buf;
  TypeParam::format_to(std::back_inserter(buf), FMT(L"{}{}"), 42, L'\0');
  EXPECT_STREQ(buf.data(), L"42");
}

TYPED_TEST(FormatToTest,
           FormatToNonbackInsertIteratorWithSignAndNumericAlignment) {
  char buffer[16] = {};
  TypeParam::format_to(fmt::internal::make_checked(buffer, 16), FMT("{: =+}"),
                       42.0);
  EXPECT_STREQ("+42", buffer);
}

TYPED_TEST(FormatToTest, FormatToMemoryBuffer) {
  fmt::basic_memory_buffer<char, 100> buffer;
  TypeParam::format_to(buffer, FMT("{}"), "foo");
  EXPECT_EQ("foo", to_string(buffer));
  fmt::wmemory_buffer wbuffer;
  TypeParam::format_to(wbuffer, FMT(L"{}"), L"foo");
  EXPECT_EQ(L"foo", to_string(wbuffer));
}

TYPED_TEST(FormatterTest, Escape) {
  EXPECT_EQ("{", TypeParam::format(FMT("{{")));
  EXPECT_EQ("before {", TypeParam::format(FMT("before {{")));
  EXPECT_EQ("{ after", TypeParam::format(FMT("{{ after")));
  EXPECT_EQ("before { after", TypeParam::format(FMT("before {{ after")));

  EXPECT_EQ("}", TypeParam::format(FMT("}}")));
  EXPECT_EQ("before }", TypeParam::format(FMT("before }}")));
  EXPECT_EQ("} after", TypeParam::format(FMT("}} after")));
  EXPECT_EQ("before } after", TypeParam::format(FMT("before }} after")));

  EXPECT_EQ("{}", TypeParam::format(FMT("{{}}")));
  EXPECT_EQ("{42}", TypeParam::format(FMT("{{{0}}}"), 42));
}

TYPED_TEST(FormatterTest, NoArgs) {
  EXPECT_EQ("test", TypeParam::format(FMT("test")));
}

TYPED_TEST(FormatterTest, ArgsInDifferentPositions) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0}"), 42));
  EXPECT_EQ("before 42", TypeParam::format(FMT("before {0}"), 42));
  EXPECT_EQ("42 after", TypeParam::format(FMT("{0} after"), 42));
  EXPECT_EQ("before 42 after", TypeParam::format(FMT("before {0} after"), 42));
  EXPECT_EQ("answer = 42", TypeParam::format(FMT("{0} = {1}"), "answer", 42));
  EXPECT_EQ("42 is the answer",
            TypeParam::format(FMT("{1} is the {0}"), "answer", 42));
  EXPECT_EQ("abracadabra", TypeParam::format(FMT("{0}{1}{0}"), "abra", "cad"));
}

TYPED_TEST(FormatterThrowTest, UnmatchedBraces) {
  EXPECT_THROW_MSG(TypeParam::format("{"), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("}"), format_error,
                   "unmatched '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0{}"), format_error,
                   "invalid format string");
}

TYPED_TEST(FormatterThrowTest, ArgErrors) {
  EXPECT_THROW_MSG(TypeParam::format("{"), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{?}"), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0"), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0}"), format_error,
                   "argument index out of range");

  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{%u", INT_MAX);
  EXPECT_THROW_MSG(TypeParam::format(format_str), format_error,
                   "invalid format string");
  safe_sprintf(format_str, "{%u}", INT_MAX);
  EXPECT_THROW_MSG(TypeParam::format(format_str), format_error,
                   "argument index out of range");

  safe_sprintf(format_str, "{%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str), format_error,
                   "number is too big");
  safe_sprintf(format_str, "{%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str), format_error,
                   "number is too big");
}

template <typename Wrapper, int N> struct TestFormat {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args &... args) {
    return TestFormat<Wrapper, N - 1>::format(format_str, N - 1, args...);
  }
};

template <typename Wrapper> struct TestFormat<Wrapper, 0> {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args &... args) {
    return Wrapper::format(format_str, args...);
  }
};

TYPED_TEST(FormatterTest, ManyArgs) {
  typedef TestFormat<TypeParam, 20> test_format_20;
  EXPECT_EQ("19", test_format_20::format(FMT("{19}")));
}

TYPED_TEST(FormatterThrowTest, ManyArgs) {
  typedef TestFormat<TypeParam, 20> test_format_20;
  EXPECT_THROW_MSG(test_format_20::format("{20}"), format_error,
                   "argument index out of range");

  typedef TestFormat<TypeParam, 21> test_format_21;
  EXPECT_THROW_MSG(test_format_21::format("{21}"), format_error,
                   "argument index out of range");
  enum { max_packed_args = fmt::internal::max_packed_args };
  std::string format_str = fmt::format("{{{}}}", max_packed_args + 1);
  typedef TestFormat<TypeParam, max_packed_args> test_format_max;
  EXPECT_THROW_MSG(test_format_max::format(format_str), format_error,
                   "argument index out of range");
}

TYPED_TEST(FormatterThrowTest, NamedArg) {
  EXPECT_THROW_MSG(TypeParam::format("{a}"), format_error,
                   "argument not found");
}

TYPED_TEST(FormatterTest, NamedArg) {
  EXPECT_EQ("1/a/A",
            TypeParam::format(FMT("{_1}/{a_}/{A_}"), fmt::arg("a_", 'a'),
                              fmt::arg("A_", "A"), fmt::arg("_1", 1)));
  EXPECT_EQ(" -42",
            TypeParam::format(FMT("{0:{width}}"), -42, fmt::arg("width", 4)));
  EXPECT_EQ("st", TypeParam::format(FMT("{0:.{precision}}"), "str",
                                    fmt::arg("precision", 2)));
  EXPECT_EQ("1 2", TypeParam::format(FMT("{} {two}"), 1, fmt::arg("two", 2)));
  EXPECT_EQ("42", TypeParam::format(
                      FMT("{c}"), fmt::arg("a", 0), fmt::arg("b", 0),
                      fmt::arg("c", 42), fmt::arg("d", 0), fmt::arg("e", 0),
                      fmt::arg("f", 0), fmt::arg("g", 0), fmt::arg("h", 0),
                      fmt::arg("i", 0), fmt::arg("j", 0), fmt::arg("k", 0),
                      fmt::arg("l", 0), fmt::arg("m", 0), fmt::arg("n", 0),
                      fmt::arg("o", 0), fmt::arg("p", 0)));
}

TYPED_TEST(FormatterTest, AutoArgIndex) {
  EXPECT_EQ("abc", TypeParam::format(FMT("{}{}{}"), 'a', 'b', 'c'));
  EXPECT_EQ("1.2", TypeParam::format(FMT("{:.{}}"), 1.2345, 2));
}

TYPED_TEST(FormatterThrowTest, AutoArgIndex) {
  EXPECT_THROW_MSG(TypeParam::format("{0}{}", 'a', 'b'), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{}{0}", 'a', 'b'), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{0}:.{}", 1.2345, 2), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{:.{0}}", 1.2345, 2), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{}"), format_error,
                   "argument index out of range");
}

TYPED_TEST(FormatterTest, EmptySpecs) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0:}"), 42));
}

TYPED_TEST(FormatterTest, LeftAlign) {
  EXPECT_EQ("42  ", TypeParam::format(FMT("{0:<4}"), 42));
  EXPECT_EQ("42  ", TypeParam::format(FMT("{0:<4o}"), 042));
  EXPECT_EQ("42  ", TypeParam::format(FMT("{0:<4x}"), 0x42));
  EXPECT_EQ("-42  ", TypeParam::format(FMT("{0:<5}"), -42));
  EXPECT_EQ("42   ", TypeParam::format(FMT("{0:<5}"), 42u));
  EXPECT_EQ("-42  ", TypeParam::format(FMT("{0:<5}"), -42l));
  EXPECT_EQ("42   ", TypeParam::format(FMT("{0:<5}"), 42ul));
  EXPECT_EQ("-42  ", TypeParam::format(FMT("{0:<5}"), -42ll));
  EXPECT_EQ("42   ", TypeParam::format(FMT("{0:<5}"), 42ull));
  EXPECT_EQ("-42  ", TypeParam::format(FMT("{0:<5}"), -42.0));
  EXPECT_EQ("-42  ", TypeParam::format(FMT("{0:<5}"), -42.0l));
  EXPECT_EQ("c    ", TypeParam::format(FMT("{0:<5}"), 'c'));
  EXPECT_EQ("abc  ", TypeParam::format(FMT("{0:<5}"), "abc"));
  EXPECT_EQ("0xface  ",
            TypeParam::format(FMT("{0:<8}"), reinterpret_cast<void *>(0xface)));
}

TYPED_TEST(FormatterTest, RightAlign) {
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:>4}"), 42));
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:>4o}"), 042));
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:>4x}"), 0x42));
  EXPECT_EQ("  -42", TypeParam::format(FMT("{0:>5}"), -42));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:>5}"), 42u));
  EXPECT_EQ("  -42", TypeParam::format(FMT("{0:>5}"), -42l));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:>5}"), 42ul));
  EXPECT_EQ("  -42", TypeParam::format(FMT("{0:>5}"), -42ll));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:>5}"), 42ull));
  EXPECT_EQ("  -42", TypeParam::format(FMT("{0:>5}"), -42.0));
  EXPECT_EQ("  -42", TypeParam::format(FMT("{0:>5}"), -42.0l));
  EXPECT_EQ("    c", TypeParam::format(FMT("{0:>5}"), 'c'));
  EXPECT_EQ("  abc", TypeParam::format(FMT("{0:>5}"), "abc"));
  EXPECT_EQ("  0xface",
            TypeParam::format(FMT("{0:>8}"), reinterpret_cast<void *>(0xface)));
}

TYPED_TEST(FormatterTest, NumericAlign) {
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:=4}"), 42));
  EXPECT_EQ("+ 42", TypeParam::format(FMT("{0:=+4}"), 42));
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:=4o}"), 042));
  EXPECT_EQ("+ 42", TypeParam::format(FMT("{0:=+4o}"), 042));
  EXPECT_EQ("  42", TypeParam::format(FMT("{0:=4x}"), 0x42));
  EXPECT_EQ("+ 42", TypeParam::format(FMT("{0:=+4x}"), 0x42));
  EXPECT_EQ("-  42", TypeParam::format(FMT("{0:=5}"), -42));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:=5}"), 42u));
  EXPECT_EQ("-  42", TypeParam::format(FMT("{0:=5}"), -42l));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:=5}"), 42ul));
  EXPECT_EQ("-  42", TypeParam::format(FMT("{0:=5}"), -42ll));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:=5}"), 42ull));
  EXPECT_EQ("-  42", TypeParam::format(FMT("{0:=5}"), -42.0));
  EXPECT_EQ("-  42", TypeParam::format(FMT("{0:=5}"), -42.0l));
  EXPECT_EQ(" 1", TypeParam::format(FMT("{:= }"), 1.0));
}

TYPED_TEST(FormatterThrowTest, NumericAlign) {
  EXPECT_THROW_MSG(TypeParam::format("{0:=5", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:=5}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0:=5}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(
      TypeParam::format("{0:=8}", reinterpret_cast<void *>(0xface)),
      format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, CenterAlign) {
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5}"), 42));
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5o}"), 042));
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5x}"), 0x42));
  EXPECT_EQ(" -42 ", TypeParam::format(FMT("{0:^5}"), -42));
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5}"), 42u));
  EXPECT_EQ(" -42 ", TypeParam::format(FMT("{0:^5}"), -42l));
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5}"), 42ul));
  EXPECT_EQ(" -42 ", TypeParam::format(FMT("{0:^5}"), -42ll));
  EXPECT_EQ(" 42  ", TypeParam::format(FMT("{0:^5}"), 42ull));
  EXPECT_EQ(" -42  ", TypeParam::format(FMT("{0:^6}"), -42.0));
  EXPECT_EQ(" -42 ", TypeParam::format(FMT("{0:^5}"), -42.0l));
  EXPECT_EQ("  c  ", TypeParam::format(FMT("{0:^5}"), 'c'));
  EXPECT_EQ(" abc  ", TypeParam::format(FMT("{0:^6}"), "abc"));
  EXPECT_EQ(" 0xface ",
            TypeParam::format(FMT("{0:^8}"), reinterpret_cast<void *>(0xface)));
}

TYPED_TEST(FormatterThrowTest, Fill) {
  EXPECT_THROW_MSG(TypeParam::format("{0:{<5}", 'c'), format_error,
                   "invalid fill character '{'");
  EXPECT_THROW_MSG(TypeParam::format("{0:{<5}}", 'c'), format_error,
                   "invalid fill character '{'");
}

TYPED_TEST(FormatterTest, Fill) {
  EXPECT_EQ("**42", TypeParam::format(FMT("{0:*>4}"), 42));
  EXPECT_EQ("**-42", TypeParam::format(FMT("{0:*>5}"), -42));
  EXPECT_EQ("***42", TypeParam::format(FMT("{0:*>5}"), 42u));
  EXPECT_EQ("**-42", TypeParam::format(FMT("{0:*>5}"), -42l));
  EXPECT_EQ("***42", TypeParam::format(FMT("{0:*>5}"), 42ul));
  EXPECT_EQ("**-42", TypeParam::format(FMT("{0:*>5}"), -42ll));
  EXPECT_EQ("***42", TypeParam::format(FMT("{0:*>5}"), 42ull));
  EXPECT_EQ("**-42", TypeParam::format(FMT("{0:*>5}"), -42.0));
  EXPECT_EQ("**-42", TypeParam::format(FMT("{0:*>5}"), -42.0l));
  EXPECT_EQ("c****", TypeParam::format(FMT("{0:*<5}"), 'c'));
  EXPECT_EQ("abc**", TypeParam::format(FMT("{0:*<5}"), "abc"));
  EXPECT_EQ("**0xface", TypeParam::format(FMT("{0:*>8}"),
                                          reinterpret_cast<void *>(0xface)));
  EXPECT_EQ("foo=", TypeParam::format(FMT("{:}="), "foo"));
}

TYPED_TEST(FormatterThrowTest, PlusSign) {
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:+", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:+}", reinterpret_cast<void *>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, PlusSign) {
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0:+}"), -42));
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42));
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42l));
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42ll));
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42.0));
  EXPECT_EQ("+42", TypeParam::format(FMT("{0:+}"), 42.0l));
}

TYPED_TEST(FormatterThrowTest, MinusSign) {
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:-", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:-}", reinterpret_cast<void *>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, MinusSign) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0:-}"), -42));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42l));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42ll));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42.0));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:-}"), 42.0l));
}

TYPED_TEST(FormatterThrowTest, SpaceSign) {
  EXPECT_THROW_MSG(TypeParam::format("{0: }", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0: }", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0: }", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_THROW_MSG(TypeParam::format("{0: ", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0: }", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0: }", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{0: }", reinterpret_cast<void *>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, SpaceSign) {
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0: }"), -42));
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42));
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42l));
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42ll));
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42.0));
  EXPECT_EQ(" 42", TypeParam::format(FMT("{0: }"), 42.0l));
}

TYPED_TEST(FormatterThrowTest, HashFlag) {
  EXPECT_THROW_MSG(TypeParam::format("{0:#", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:#}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0:#}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:#}", reinterpret_cast<void *>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, HashFlag) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0:#}"), 42));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0:#}"), -42));
  EXPECT_EQ("0b101010", TypeParam::format(FMT("{0:#b}"), 42));
  EXPECT_EQ("0B101010", TypeParam::format(FMT("{0:#B}"), 42));
  EXPECT_EQ("-0b101010", TypeParam::format(FMT("{0:#b}"), -42));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42));
  EXPECT_EQ("0X42", TypeParam::format(FMT("{0:#X}"), 0x42));
  EXPECT_EQ("-0x42", TypeParam::format(FMT("{0:#x}"), -0x42));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042));
  EXPECT_EQ("-042", TypeParam::format(FMT("{0:#o}"), -042));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:#}"), 42u));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42u));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042u));

  EXPECT_EQ("-42", TypeParam::format(FMT("{0:#}"), -42l));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42l));
  EXPECT_EQ("-0x42", TypeParam::format(FMT("{0:#x}"), -0x42l));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042l));
  EXPECT_EQ("-042", TypeParam::format(FMT("{0:#o}"), -042l));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:#}"), 42ul));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42ul));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042ul));

  EXPECT_EQ("-42", TypeParam::format(FMT("{0:#}"), -42ll));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42ll));
  EXPECT_EQ("-0x42", TypeParam::format(FMT("{0:#x}"), -0x42ll));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042ll));
  EXPECT_EQ("-042", TypeParam::format(FMT("{0:#o}"), -042ll));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:#}"), 42ull));
  EXPECT_EQ("0x42", TypeParam::format(FMT("{0:#x}"), 0x42ull));
  EXPECT_EQ("042", TypeParam::format(FMT("{0:#o}"), 042ull));

  if (FMT_USE_GRISU)
    EXPECT_EQ("-42.0", TypeParam::format(FMT("{0:#}"), -42.0));
  else
    EXPECT_EQ("-42.0000", TypeParam::format(FMT("{0:#}"), -42.0));

  EXPECT_EQ("-42.0000", TypeParam::format(FMT("{0:#}"), -42.0l));
}

TYPED_TEST(FormatterThrowTest, ZeroFlag) {
  EXPECT_THROW_MSG(TypeParam::format("{0:0", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:05}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(TypeParam::format("{0:05}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{0:05}", reinterpret_cast<void *>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TYPED_TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0:0}"), 42));
  EXPECT_EQ("-0042", TypeParam::format(FMT("{0:05}"), -42));
  EXPECT_EQ("00042", TypeParam::format(FMT("{0:05}"), 42u));
  EXPECT_EQ("-0042", TypeParam::format(FMT("{0:05}"), -42l));
  EXPECT_EQ("00042", TypeParam::format(FMT("{0:05}"), 42ul));
  EXPECT_EQ("-0042", TypeParam::format(FMT("{0:05}"), -42ll));
  EXPECT_EQ("00042", TypeParam::format(FMT("{0:05}"), 42ull));
  EXPECT_EQ("-0042", TypeParam::format(FMT("{0:05}"), -42.0));
  EXPECT_EQ("-0042", TypeParam::format(FMT("{0:05}"), -42.0l));
}

TYPED_TEST(FormatterThrowTest, Width) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:%u", UINT_MAX);
  increment(format_str + 3);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");

  safe_sprintf(format_str, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  safe_sprintf(format_str, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
}

TYPED_TEST(FormatterTest, Width) {
  EXPECT_EQ(" -42", TypeParam::format(FMT("{0:4}"), -42));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:5}"), 42u));
  EXPECT_EQ("   -42", TypeParam::format(FMT("{0:6}"), -42l));
  EXPECT_EQ("     42", TypeParam::format(FMT("{0:7}"), 42ul));
  EXPECT_EQ("   -42", TypeParam::format(FMT("{0:6}"), -42ll));
  EXPECT_EQ("     42", TypeParam::format(FMT("{0:7}"), 42ull));
  EXPECT_EQ("   -1.23", TypeParam::format(FMT("{0:8}"), -1.23));
  EXPECT_EQ("    -1.23", TypeParam::format(FMT("{0:9}"), -1.23l));
  EXPECT_EQ("    0xcafe",
            TypeParam::format(FMT("{0:10}"), reinterpret_cast<void *>(0xcafe)));
  EXPECT_EQ("x          ", TypeParam::format(FMT("{0:11}"), 'x'));
  EXPECT_EQ("str         ", TypeParam::format(FMT("{0:12}"), "str"));
}

TYPED_TEST(FormatterThrowTest, RuntimeWidth) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:{%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");

  EXPECT_THROW_MSG(TypeParam::format("{0:{", 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:{}", 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{0:{?}}", 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0), format_error,
                   "argument index out of range");

  EXPECT_THROW_MSG(TypeParam::format("{0:{0:}}", 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, -1), format_error,
                   "negative width");
  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, (INT_MAX + 1u)),
                   format_error, "number is too big");
  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, -1l), format_error,
                   "negative width");
  if (fmt::internal::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, (value + 1)), format_error,
                     "number is too big");
  }
  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, (INT_MAX + 1ul)),
                   format_error, "number is too big");

  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, '0'), format_error,
                   "width is not integer");
  EXPECT_THROW_MSG(TypeParam::format("{0:{1}}", 0, 0.0), format_error,
                   "width is not integer");
}

TYPED_TEST(FormatterTest, RuntimeWidth) {
  EXPECT_EQ(" -42", TypeParam::format(FMT("{0:{1}}"), -42, 4));
  EXPECT_EQ("   42", TypeParam::format(FMT("{0:{1}}"), 42u, 5));
  EXPECT_EQ("   -42", TypeParam::format(FMT("{0:{1}}"), -42l, 6));
  EXPECT_EQ("     42", TypeParam::format(FMT("{0:{1}}"), 42ul, 7));
  EXPECT_EQ("   -42", TypeParam::format(FMT("{0:{1}}"), -42ll, 6));
  EXPECT_EQ("     42", TypeParam::format(FMT("{0:{1}}"), 42ull, 7));
  EXPECT_EQ("   -1.23", TypeParam::format(FMT("{0:{1}}"), -1.23, 8));
  EXPECT_EQ("    -1.23", TypeParam::format(FMT("{0:{1}}"), -1.23l, 9));
  EXPECT_EQ(
      "    0xcafe",
      TypeParam::format(FMT("{0:{1}}"), reinterpret_cast<void *>(0xcafe), 10));
  EXPECT_EQ("x          ", TypeParam::format(FMT("{0:{1}}"), 'x', 11));
  EXPECT_EQ("str         ", TypeParam::format(FMT("{0:{1}}"), "str", 12));
}

// These two tests need to be separated. They throw exceptions with different
// messages.
// prepare(..) throws at preparation time, when arguments are not known, thus it
// cannot know if precision is allowed for given argument type
TEST(PreparedFormatterTest, PrecisionMissingBraceInFormatString) {
  EXPECT_THROW_MSG(fmt::prepare<int>("{0:.2"), format_error,
                   "missing '}' in format string");
}

TEST(FormatterTest, PrecisionNotAllowedArgumentTypeInMalformedFormat) {
  EXPECT_THROW_MSG(fmt::format("{0:.2", 0), format_error,
                   "precision not allowed for this argument type");
}

TYPED_TEST(FormatterThrowTest, Precision) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:.%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");

  safe_sprintf(format_str, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  safe_sprintf(format_str, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");

  EXPECT_THROW_MSG(TypeParam::format("{0:.", 0), format_error,
                   "missing precision specifier");
  EXPECT_THROW_MSG(TypeParam::format("{0:.}", 0), format_error,
                   "missing precision specifier");

  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2}", 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.2f}", 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:3.0}", 'x'), format_error,
                   "precision not allowed for this argument type");

  EXPECT_THROW_MSG(
      TypeParam::format("{0:.2}", reinterpret_cast<void *>(0xcafe)),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(
      TypeParam::format("{0:.2f}", reinterpret_cast<void *>(0xcafe)),
      format_error, "precision not allowed for this argument type");
}

TYPED_TEST(FormatterTest, Precision) {
  EXPECT_EQ("1.2", TypeParam::format(FMT("{0:.2}"), 1.2345));
  EXPECT_EQ("1.2", TypeParam::format(FMT("{0:.2}"), 1.2345l));
  EXPECT_EQ("st", TypeParam::format(FMT("{0:.2}"), "str"));
}

// These two tests need to be separated. They throw exceptions with different
// messages.
// prepare(..) throws at preparation time, when arguments are not known, thus it
// cannot know if precision is allowed for given argument type
TEST(PreparedFormatterTest, RuntimePrecisionMissingBraceInFormatString) {
  EXPECT_THROW_MSG(fmt::prepare<int>("{0:.{1}"), format_error,
                   "missing '}' in format string");
}

TEST(FormatterTest, RuntimePrecisionNotAllowedArgumentTypeInMalformedFormat) {
  EXPECT_THROW_MSG(fmt::format("{0:.{1}", 0, 0), format_error,
                   "precision not allowed for this argument type");
}

TYPED_TEST(FormatterThrowTest, RuntimePrecision) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:.{%u", UINT_MAX);
  increment(format_str + 5);
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(TypeParam::format(format_str, 0), format_error,
                   "number is too big");

  EXPECT_THROW_MSG(TypeParam::format("{0:.{", 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{}", 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{?}}", 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0), format_error,
                   "argument index out of range");

  EXPECT_THROW_MSG(TypeParam::format("{0:.{0:}}", 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, -1), format_error,
                   "negative precision");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, (INT_MAX + 1u)),
                   format_error, "number is too big");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, -1l), format_error,
                   "negative precision");
  if (fmt::internal::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, (value + 1)),
                     format_error, "number is too big");
  }
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, (INT_MAX + 1ul)),
                   format_error, "number is too big");

  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, '0'), format_error,
                   "precision is not integer");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 0, 0.0), format_error,
                   "precision is not integer");

  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42u, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42u, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42l, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42l, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42ul, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42ul, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42ll, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42ll, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}}", 42ull, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:.{1}f}", 42ull, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(TypeParam::format("{0:3.{1}}", 'x', 0), format_error,
                   "precision not allowed for this argument type");

  EXPECT_THROW_MSG(
      TypeParam::format("{0:.{1}}", reinterpret_cast<void *>(0xcafe), 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(
      TypeParam::format("{0:.{1}f}", reinterpret_cast<void *>(0xcafe), 2),
      format_error, "precision not allowed for this argument type");
}

TYPED_TEST(FormatterTest, RuntimePrecision) {
  EXPECT_EQ("1.2", TypeParam::format(FMT("{0:.{1}}"), 1.2345, 2));
  EXPECT_EQ("1.2", TypeParam::format(FMT("{1:.{0}}"), 2, 1.2345l));
  EXPECT_EQ("st", TypeParam::format(FMT("{0:.{1}}"), "str", 2));
}

TYPED_TEST(FormatterTest, FormatBool) {
  EXPECT_EQ("true", TypeParam::format(FMT("{}"), true));
  EXPECT_EQ("false", TypeParam::format(FMT("{}"), false));
  EXPECT_EQ("1", TypeParam::format(FMT("{:d}"), true));
  EXPECT_EQ("true ", TypeParam::format(FMT("{:5}"), true));
  EXPECT_EQ(L"true", TypeParam::format(FMT(L"{}"), true));
}

TYPED_TEST(FormatterTest, FormatShort) {
  short s = 42;
  EXPECT_EQ("42", TypeParam::format(FMT("{0:d}"), s));
  unsigned short us = 42;
  EXPECT_EQ("42", TypeParam::format(FMT("{0:d}"), us));
}

template <typename FormatWrapper, typename T>
void check_unknown_types(const T &value, const char *types, const char *) {
  char format_str[BUFFER_SIZE];
  const char *special = ".0123456789}";
  for (int i = CHAR_MIN; i <= CHAR_MAX; ++i) {
    char c = static_cast<char>(i);
    if (std::strchr(types, c) || std::strchr(special, c) || !c)
      continue;
    safe_sprintf(format_str, "{0:10%c}", c);
    const char *message = "invalid type specifier";
    EXPECT_THROW_MSG(FormatWrapper::format(format_str, value), format_error,
                     message)
        << format_str << " " << message;
  }
}

TYPED_TEST(FormatterThrowTest, FormatInt) {
  EXPECT_THROW_MSG(TypeParam::format("{0:v", 42), format_error,
                   "missing '}' in format string");
  check_unknown_types<TypeParam>(42, "bBdoxXn", "integer");
}

TYPED_TEST(FormatterTest, FormatBin) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0:b}"), 0));
  EXPECT_EQ("101010", TypeParam::format(FMT("{0:b}"), 42));
  EXPECT_EQ("101010", TypeParam::format(FMT("{0:b}"), 42u));
  EXPECT_EQ("-101010", TypeParam::format(FMT("{0:b}"), -42));
  EXPECT_EQ("11000000111001", TypeParam::format(FMT("{0:b}"), 12345));
  EXPECT_EQ("10010001101000101011001111000",
            TypeParam::format(FMT("{0:b}"), 0x12345678));
  EXPECT_EQ("10010000101010111100110111101111",
            TypeParam::format(FMT("{0:b}"), 0x90ABCDEF));
  EXPECT_EQ(
      "11111111111111111111111111111111",
      TypeParam::format(FMT("{0:b}"), std::numeric_limits<uint32_t>::max()));
}

TYPED_TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0}"), 0));
  EXPECT_EQ("42", TypeParam::format(FMT("{0}"), 42));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:d}"), 42));
  EXPECT_EQ("42", TypeParam::format(FMT("{0}"), 42u));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0}"), -42));
  EXPECT_EQ("12345", TypeParam::format(FMT("{0}"), 12345));
  EXPECT_EQ("67890", TypeParam::format(FMT("{0}"), 67890));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), INT_MIN));
  safe_sprintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), INT_MAX));
  safe_sprintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), UINT_MAX));
  safe_sprintf(buffer, "%ld", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), LONG_MIN));
  safe_sprintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), LONG_MAX));
  safe_sprintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0}"), ULONG_MAX));
}

TYPED_TEST(FormatterTest, FormatHex) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0:x}"), 0));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:x}"), 0x42));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:x}"), 0x42u));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0:x}"), -0x42));
  EXPECT_EQ("12345678", TypeParam::format(FMT("{0:x}"), 0x12345678));
  EXPECT_EQ("90abcdef", TypeParam::format(FMT("{0:x}"), 0x90abcdef));
  EXPECT_EQ("12345678", TypeParam::format(FMT("{0:X}"), 0x12345678));
  EXPECT_EQ("90ABCDEF", TypeParam::format(FMT("{0:X}"), 0x90ABCDEF));

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "-%x", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), INT_MIN));
  safe_sprintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), INT_MAX));
  safe_sprintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), UINT_MAX));
  safe_sprintf(buffer, "-%lx", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), LONG_MIN));
  safe_sprintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), LONG_MAX));
  safe_sprintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:x}"), ULONG_MAX));
}

TYPED_TEST(FormatterTest, FormatOct) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0:o}"), 0));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:o}"), 042));
  EXPECT_EQ("42", TypeParam::format(FMT("{0:o}"), 042u));
  EXPECT_EQ("-42", TypeParam::format(FMT("{0:o}"), -042));
  EXPECT_EQ("12345670", TypeParam::format(FMT("{0:o}"), 012345670));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "-%o", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), INT_MIN));
  safe_sprintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), INT_MAX));
  safe_sprintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), UINT_MAX));
  safe_sprintf(buffer, "-%lo", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), LONG_MIN));
  safe_sprintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), LONG_MAX));
  safe_sprintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:o}"), ULONG_MAX));
}

TYPED_TEST(FormatterTest, FormatIntLocale) {
  EXPECT_EQ("123", TypeParam::format(FMT("{0:n}"), 123));
  EXPECT_EQ("1,234", TypeParam::format(FMT("{0:n}"), 1234));
  EXPECT_EQ("1,234,567", TypeParam::format(FMT("{0:n}"), 1234567));
  EXPECT_EQ(
      "4,294,967,295",
      TypeParam::format(FMT("{0:n}"), std::numeric_limits<uint32_t>::max()));
}

struct ConvertibleToLongLong {
  operator long long() const { return 1LL << 32; }
};

TYPED_TEST(FormatterTest, FormatConvertibleToLongLong) {
  EXPECT_EQ("100000000",
            TypeParam::format(FMT("{0:x}"), ConvertibleToLongLong()));
}

TYPED_TEST(FormatterTest, FormatFloat) {
  EXPECT_EQ("392.500000", TypeParam::format(FMT("{0:f}"), 392.5f));
}

TYPED_TEST(FormatterThrowTest, FormatDouble) {
  check_unknown_types<TypeParam>(1.2, "eEfFgGaA", "double");
}

TYPED_TEST(FormatterTest, FormatDouble) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0:}"), 0.0));
  EXPECT_EQ("0.000000", TypeParam::format(FMT("{0:f}"), 0.0));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:}"), 392.65));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:g}"), 392.65));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:G}"), 392.65));
  EXPECT_EQ("392.650000", TypeParam::format(FMT("{0:f}"), 392.65));
  EXPECT_EQ("392.650000", TypeParam::format(FMT("{0:F}"), 392.65));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%e", 392.65);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:e}"), 392.65));
  safe_sprintf(buffer, "%E", 392.65);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:E}"), 392.65));
  EXPECT_EQ("+0000392.6", TypeParam::format(FMT("{0:+010.4g}"), 392.65));
  safe_sprintf(buffer, "%a", -42.0);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{:a}"), -42.0));
  safe_sprintf(buffer, "%A", -42.0);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{:A}"), -42.0));
}

TYPED_TEST(FormatterTest, FormatDoubleBigPrecision) {
  // sprintf with big precision is broken in MSVC2013, so only test on Grisu.
  if (FMT_USE_GRISU) {
    const auto result1 = TypeParam::format(FMT("0.{:0<1000}"), "");
    const auto result2 = TypeParam::format(FMT("{:.1000f}"), 0.0);
    EXPECT_EQ(result1, result2);
  }
}

TYPED_TEST(FormatterTest, FormatNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ("nan", TypeParam::format(FMT("{}"), nan));
  EXPECT_EQ("+nan", TypeParam::format(FMT("{:+}"), nan));
  EXPECT_EQ(" nan", TypeParam::format(FMT("{: }"), nan));
  EXPECT_EQ("NAN", TypeParam::format(FMT("{:F}"), nan));
  EXPECT_EQ("nan    ", TypeParam::format(FMT("{:<7}"), nan));
  EXPECT_EQ("  nan  ", TypeParam::format(FMT("{:^7}"), nan));
  EXPECT_EQ("    nan", TypeParam::format(FMT("{:>7}"), nan));
}

TYPED_TEST(FormatterTest, FormatInfinity) {
  double inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ("inf", TypeParam::format(FMT("{}"), inf));
  EXPECT_EQ("+inf", TypeParam::format(FMT("{:+}"), inf));
  EXPECT_EQ("-inf", TypeParam::format(FMT("{}"), -inf));
  EXPECT_EQ(" inf", TypeParam::format(FMT("{: }"), inf));
  EXPECT_EQ("INF", TypeParam::format(FMT("{:F}"), inf));
  EXPECT_EQ("inf    ", TypeParam::format(FMT("{:<7}"), inf));
  EXPECT_EQ("  inf  ", TypeParam::format(FMT("{:^7}"), inf));
  EXPECT_EQ("    inf", TypeParam::format(FMT("{:>7}"), inf));
}

TYPED_TEST(FormatterTest, FormatLongDouble) {
  EXPECT_EQ("0", TypeParam::format(FMT("{0:}"), 0.0l));
  EXPECT_EQ("0.000000", TypeParam::format(FMT("{0:f}"), 0.0l));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:}"), 392.65l));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:g}"), 392.65l));
  EXPECT_EQ("392.65", TypeParam::format(FMT("{0:G}"), 392.65l));
  EXPECT_EQ("392.650000", TypeParam::format(FMT("{0:f}"), 392.65l));
  EXPECT_EQ("392.650000", TypeParam::format(FMT("{0:F}"), 392.65l));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%Le", 392.65l);
  EXPECT_EQ(buffer, TypeParam::format(FMT("{0:e}"), 392.65l));
  EXPECT_EQ("+0000392.6", TypeParam::format(FMT("{0:+010.4g}"), 392.64l));
}
TYPED_TEST(FormatterThrowTest, FormatChar) {
  const char types[] = "cbBdoxXn";
  check_unknown_types<TypeParam>('a', types, "char");
}

TYPED_TEST(FormatterTest, FormatChar) {
  EXPECT_EQ("a", TypeParam::format(FMT("{0}"), 'a'));
  EXPECT_EQ("z", TypeParam::format(FMT("{0:c}"), 'z'));
  EXPECT_EQ(L"a", TypeParam::format(FMT(L"{0}"), 'a'));

  int x = 'x';
  auto result = TypeParam::format(FMT("{:b}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:b}"), 'x'));
  result = TypeParam::format(FMT("{:B}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:B}"), 'x'));
  result = TypeParam::format(FMT("{:d}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:d}"), 'x'));
  result = TypeParam::format(FMT("{:o}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:o}"), 'x'));
  result = TypeParam::format(FMT("{:x}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:x}"), 'x'));
  result = TypeParam::format(FMT("{:X}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:X}"), 'x'));
  result = TypeParam::format(FMT("{:n}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:n}"), 'x'));
  result = TypeParam::format(FMT("{:02X}"), x);
  EXPECT_EQ(result, TypeParam::format(FMT("{:02X}"), 'x'));
}

TYPED_TEST(FormatterTest, FormatUnsignedChar) {
  EXPECT_EQ("42", TypeParam::format(FMT("{}"), static_cast<unsigned char>(42)));
  EXPECT_EQ("42", TypeParam::format(FMT("{}"), static_cast<uint8_t>(42)));
}

TYPED_TEST(FormatterTest, FormatWChar) {
  EXPECT_EQ(L"a", TypeParam::format(FMT(L"{0}"), L'a'));
  // This shouldn't compile:
  // format("{}", L'a');
}
TYPED_TEST(FormatterThrowTest, FormatCString) {
  check_unknown_types<TypeParam>("test", "sp", "string");
  EXPECT_THROW_MSG(
      TypeParam::format("{0}", static_cast<const char *>(FMT_NULL)),
      format_error, "string pointer is null");
}

TYPED_TEST(FormatterTest, FormatCString) {
  EXPECT_EQ("test", TypeParam::format(FMT("{0}"), "test"));
  EXPECT_EQ("test", TypeParam::format(FMT("{0:s}"), "test"));
  char nonconst[] = "nonconst";
  EXPECT_EQ("nonconst", TypeParam::format(FMT("{0}"), nonconst));
}

// FormatUCharString and FormatUCharString workaround for GCC < 4.7 bug #20140
TEST(FormatterTest, FormatSCharString) {
  signed char str[] = "test";
  EXPECT_EQ("test", fmt::format("{0:s}", str));
  {
    auto prepared = fmt::prepare<signed char *>("{0:s}");
    EXPECT_EQ("test", prepared.format(str));
  }
#if FMT_USE_CONSTEXPR
  {
    auto prepared = fmt::prepare<signed char *>(FMT("{0:s}"));
    EXPECT_EQ("test", prepared.format(str));
  }
#endif

  const signed char *const_str = str;
  EXPECT_EQ("test", fmt::format("{0:s}", const_str));
  {
    auto prepared = fmt::prepare<const signed char *>("{0:s}");
    EXPECT_EQ("test", prepared.format(const_str));
  }
#if FMT_USE_CONSTEXPR
  {
    auto prepared = fmt::prepare<const signed char *>(FMT("{0:s}"));
    EXPECT_EQ("test", prepared.format(const_str));
  }
#endif
}

TEST(FormatterTest, FormatUCharString) {
  unsigned char str[] = "test";
  EXPECT_EQ("test", fmt::format("{0:s}", str));
  {
    auto prepared = fmt::prepare<unsigned char *>("{0:s}");
    EXPECT_EQ("test", prepared.format(str));
  }
#if FMT_USE_CONSTEXPR
  {
    auto prepared = fmt::prepare<unsigned char *>(FMT("{0:s}"));
    EXPECT_EQ("test", prepared.format(str));
  }
#endif

  const unsigned char *const_str = str;
  EXPECT_EQ("test", fmt::format("{0:s}", str));
  {
    auto prepared = fmt::prepare<const unsigned char *>("{0:s}");
    EXPECT_EQ("test", prepared.format(str));
  }
#if FMT_USE_CONSTEXPR
  {
    auto prepared = fmt::prepare<const unsigned char *>(FMT("{0:s}"));
    EXPECT_EQ("test", prepared.format(str));
  }
#endif

  unsigned char *ptr = str;
  EXPECT_EQ("test", fmt::format("{0:s}", str));
  {
    auto prepared = fmt::prepare<unsigned char *>("{0:s}");
    EXPECT_EQ("test", prepared.format(str));
  }
#if FMT_USE_CONSTEXPR
  {
    auto prepared = fmt::prepare<unsigned char *>(FMT("{0:s}"));
    EXPECT_EQ("test", prepared.format(str));
  }
#endif
}
TYPED_TEST(FormatterThrowTest, FormatPointer) {
  check_unknown_types<TypeParam>(reinterpret_cast<void *>(0x1234), "p",
                                 "pointer");
}

TYPED_TEST(FormatterTest, FormatPointer) {
  EXPECT_EQ("0x0",
            TypeParam::format(FMT("{0}"), static_cast<void *>(FMT_NULL)));
  EXPECT_EQ("0x1234",
            TypeParam::format(FMT("{0}"), reinterpret_cast<void *>(0x1234)));
  EXPECT_EQ("0x1234",
            TypeParam::format(FMT("{0:p}"), reinterpret_cast<void *>(0x1234)));
  EXPECT_EQ(
      "0x" + std::string(sizeof(void *) * CHAR_BIT / 4, 'f'),
      TypeParam::format(FMT("{0}"), reinterpret_cast<void *>(~uintptr_t())));
  EXPECT_EQ("0x1234",
            TypeParam::format(FMT("{}"), reinterpret_cast<void *>(0x1234)));
#if FMT_USE_NULLPTR
  EXPECT_EQ("0x0", TypeParam::format(FMT("{}"), FMT_NULL));
#endif
}

TYPED_TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", TypeParam::format(FMT("{0}"), std::string("test")));
}

TYPED_TEST(FormatterTest, FormatStringView) {
  EXPECT_EQ("test", TypeParam::format(FMT("{}"), string_view("test")));
  EXPECT_EQ("", TypeParam::format(FMT("{}"), string_view()));
}

#ifdef FMT_USE_STD_STRING_VIEW
TYPED_TEST(FormatterTest, FormatStdStringView) {
  EXPECT_EQ("test", TypeParam::format(FMT("{}"), std::string_view("test")));
}
#endif

FMT_BEGIN_NAMESPACE
template <> struct formatter<Date> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (*it == 'd') {
      return ++it;
    }
    return it;
  }

  auto format(const Date &d, format_context &ctx) -> decltype(ctx.out()) {
    format_to(ctx.out(), "{}-{}-{}", d.year(), d.month(), d.day());
    return ctx.out();
  }
};
FMT_END_NAMESPACE

// This test should be enabled for fmt::prepare() wrappers once it will support
// custom types formatting.
TYPED_TEST(RuntimeFormatFunctionTest, FormatCustom) {
  Date date(2012, 12, 9);
  EXPECT_THROW_MSG(TypeParam::format("{:s}", date), format_error,
                   "unknown format specifier");
}

class Answer {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<Answer> : formatter<int> {
  template <typename FormatContext>
  auto format(Answer, FormatContext &ctx) -> decltype(ctx.begin()) {
    return formatter<int>::format(42, ctx);
  }
};
FMT_END_NAMESPACE

// This test should be enabled for fmt::prepare() wrappers once it will support
// custom types formatting.
TYPED_TEST(RuntimeFormatFunctionTest, CustomFormat) {
  EXPECT_EQ("42", TypeParam::format(FMT("{0}"), Answer()));
  EXPECT_EQ("0042", TypeParam::format(FMT("{:04}"), Answer()));
}

// This test should be enabled for fmt::prepare() wrappers once it will support
// custom types formatting.
TYPED_TEST(RuntimeFormatFunctionTest, CustomFormatTo) {
  char buf[10] = {};
  auto end = &*TypeParam::format_to(fmt::internal::make_checked(buf, 10),
                                    FMT("{}"), Answer());
  EXPECT_EQ(end, buf + 2);
  EXPECT_STREQ(buf, "42");
}

TYPED_TEST(FormatterTest, WideFormatString) {
  EXPECT_EQ(L"42", TypeParam::format(FMT(L"{}"), 42));
  EXPECT_EQ(L"4.2", TypeParam::format(FMT(L"{}"), 4.2));
  EXPECT_EQ(L"abc", TypeParam::format(FMT(L"{}"), L"abc"));
  EXPECT_EQ(L"z", TypeParam::format(FMT(L"{}"), L'z'));
}

TYPED_TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
            TypeParam::format(FMT("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%"),
                              1.234, 42, 3.13, "str",
                              reinterpret_cast<void *>(1000), 'X'));
}

// This test should be enabled for fmt::prepare() wrappers once it will support
// custom types formatting.
TYPED_TEST(FormatFunctionTest, JoinArg) {
  using fmt::join;
  int v1[3] = {1, 2, 3};
  std::vector<float> v2;
  v2.push_back(1.2f);
  v2.push_back(3.4f);
  void *v3[2] = {&v1[0], &v1[1]};

  EXPECT_EQ("(1, 2, 3)",
            TypeParam::format(FMT("({})"), join(v1, v1 + 3, ", ")));
  EXPECT_EQ("(1)", TypeParam::format(FMT("({})"), join(v1, v1 + 1, ", ")));
  EXPECT_EQ("()", TypeParam::format(FMT("({})"), join(v1, v1, ", ")));
  EXPECT_EQ("(001, 002, 003)",
            TypeParam::format(FMT("({:03})"), join(v1, v1 + 3, ", ")));
  EXPECT_EQ(
      "(+01.20, +03.40)",
      TypeParam::format(FMT("({:+06.2f})"), join(v2.begin(), v2.end(), ", ")));

  EXPECT_EQ(L"(1, 2, 3)",
            TypeParam::format(FMT(L"({})"), join(v1, v1 + 3, L", ")));
  EXPECT_EQ("1, 2, 3",
            TypeParam::format(FMT("{0:{1}}"), join(v1, v1 + 3, ", "), 1));

  const auto result = TypeParam::format(FMT("{}, {}"), v3[0], v3[1]);
  EXPECT_EQ(result, TypeParam::format(FMT("{}"), join(v3, v3 + 2, ", ")));

#if FMT_USE_TRAILING_RETURN && (!FMT_GCC_VERSION || FMT_GCC_VERSION >= 405)
  EXPECT_EQ("(1, 2, 3)", TypeParam::format(FMT("({})"), join(v1, ", ")));
  EXPECT_EQ("(+01.20, +03.40)",
            TypeParam::format(FMT("({:+06.2f})"), join(v2, ", ")));
#endif
}

TYPED_TEST(FormatterTest, UnpackedArgs) {
  EXPECT_EQ("0123456789abcdefg",
            TypeParam::format(FMT("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}"), 0, 1,
                              2, 3, 4, 5, 6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e',
                              'f', 'g'));
}

enum TestEnum { A };

TYPED_TEST(FormatterTest, Enum) {
  EXPECT_EQ("0", TypeParam::format(FMT("{}"), A));
}

TEST(FormatTest, NonNullTerminatedFormatString) {
  EXPECT_EQ("42", format(string_view("{}foo", 2), 42));

  {
    auto prepared_formatter = fmt::prepare<int>(string_view("{}foo", 2));
    const auto result = prepared_formatter.format(42);
    EXPECT_EQ("42", result);
  }
}

struct variant {
  enum { INT, STRING } type;
  explicit variant(int) : type(INT) {}
  explicit variant(const char *) : type(STRING) {}
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<variant> : dynamic_formatter<> {
  template <typename FormatContext>
  auto format(variant value, FormatContext &ctx) -> decltype(ctx.out()) {
    if (value.type == variant::INT)
      return dynamic_formatter<>::format(42, ctx);
    return dynamic_formatter<>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

// This test should be enabled for fmt::prepare() wrappers once it will support
// custom types formatting.
TYPED_TEST(RuntimeFormatFunctionTest, DynamicFormatter) {
  auto num = variant(42);
  auto str = variant("foo");
  EXPECT_THROW_MSG(TypeParam::format("{0:{}}", num), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{:{0}}", num), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(TypeParam::format("{:=}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{:+}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{:-}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{: }", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{:#}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{:0}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(TypeParam::format("{:.2}", num), format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("42", TypeParam::format(FMT("{:d}"), num));
  EXPECT_EQ("foo", TypeParam::format(FMT("{:s}"), str));
  EXPECT_EQ(" 42 foo ", TypeParam::format(FMT("{:{}} {:{}}"), num, 3, str, 4));
}

#if FMT_USE_USER_DEFINED_LITERALS
TYPED_TEST(RuntimeFormattersTest, U8StringViewLiteral) {
  using namespace fmt::literals;
  fmt::u8string_view s = "ab"_u;
  EXPECT_EQ(s.size(), 2u);
  const fmt::char8_t *data = s.data();
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[1], 'b');
  EXPECT_EQ("**🤡**"_u, TypeParam::format("{:*^5}"_u, "🤡"_u));
}
#endif

TYPED_TEST(RuntimeFormattersTest, FormatU8String) {
  EXPECT_EQ(fmt::u8string_view("42"),
            TypeParam::format(fmt::u8string_view("{}"), 42));
}

TYPED_TEST(FormatterTest, FormattedSize) {
  EXPECT_EQ(2u, TypeParam::formatted_size(FMT("{}"), 42));
}

TYPED_TEST(FormatterTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = TypeParam::format_to_n(buffer, 3, FMT("{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("123x", fmt::string_view(buffer, 4));
  result = TypeParam::format_to_n(buffer, 3, FMT("{:s}"), "foobar");
  EXPECT_EQ(6u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("foox", fmt::string_view(buffer, 4));
}

TYPED_TEST(FormatterTest, WideFormatToN) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  const auto result = TypeParam::format_to_n(buffer, 3, FMT(L"{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
}
