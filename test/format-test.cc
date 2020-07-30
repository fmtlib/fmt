// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <stdint.h>

#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <list>
#include <memory>
#include <string>

// Check if fmt/format.h compiles with windows.h included before it.
#ifdef _WIN32
#  include <windows.h>
#endif

// Check if fmt/format.h compiles with the X11 index macro defined.
#define index(x, y) no nice things

#include "fmt/color.h"
#include "fmt/format.h"

#undef index

#include "gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

#undef ERROR

using fmt::basic_memory_buffer;
using fmt::format;
using fmt::format_error;
using fmt::memory_buffer;
using fmt::string_view;
using fmt::wmemory_buffer;
using fmt::wstring_view;
using fmt::detail::max_value;

using testing::Return;
using testing::StrictMock;

namespace {

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 408
template <typename Char, typename T> bool check_enabled_formatter() {
  static_assert(std::is_default_constructible<fmt::formatter<T, Char>>::value,
                "");
  return true;
}

template <typename Char, typename... T> void check_enabled_formatters() {
  auto dummy = {check_enabled_formatter<Char, T>()...};
  (void)dummy;
}

TEST(FormatterTest, TestFormattersEnabled) {
  check_enabled_formatters<char, bool, char, signed char, unsigned char, short,
                           unsigned short, int, unsigned, long, unsigned long,
                           long long, unsigned long long, float, double,
                           long double, void*, const void*, char*, const char*,
                           std::string, std::nullptr_t>();
  check_enabled_formatters<wchar_t, bool, wchar_t, signed char, unsigned char,
                           short, unsigned short, int, unsigned, long,
                           unsigned long, long long, unsigned long long, float,
                           double, long double, void*, const void*, wchar_t*,
                           const wchar_t*, std::wstring, std::nullptr_t>();
}
#endif

// Format value using the standard library.
template <typename Char, typename T>
void std_format(const T& value, std::basic_string<Char>& result) {
  std::basic_ostringstream<Char> os;
  os << value;
  result = os.str();
}

#ifdef __MINGW32__
// Workaround a bug in formatting long double in MinGW.
void std_format(long double value, std::string& result) {
  char buffer[100];
  safe_sprintf(buffer, "%Lg", value);
  result = buffer;
}
void std_format(long double value, std::wstring& result) {
  wchar_t buffer[100];
  swprintf(buffer, L"%Lg", value);
  result = buffer;
}
#endif
}  // namespace

struct uint32_pair {
  uint32_t u[2];
};

TEST(UtilTest, BitCast) {
  auto s = fmt::detail::bit_cast<uint32_pair>(uint64_t{42});
  EXPECT_EQ(fmt::detail::bit_cast<uint64_t>(s), 42ull);
  s = fmt::detail::bit_cast<uint32_pair>(uint64_t(~0ull));
  EXPECT_EQ(fmt::detail::bit_cast<uint64_t>(s), ~0ull);
}

TEST(UtilTest, Increment) {
  char s[10] = "123";
  increment(s);
  EXPECT_STREQ("124", s);
  s[2] = '8';
  increment(s);
  EXPECT_STREQ("129", s);
  increment(s);
  EXPECT_STREQ("130", s);
  s[1] = s[2] = '9';
  increment(s);
  EXPECT_STREQ("200", s);
}

TEST(UtilTest, ParseNonnegativeInt) {
  if (max_value<int>() != static_cast<int>(static_cast<unsigned>(1) << 31)) {
    fmt::print("Skipping parse_nonnegative_int test\n");
    return;
  }
  fmt::string_view s = "10000000000";
  auto begin = s.begin(), end = s.end();
  EXPECT_THROW_MSG(
      parse_nonnegative_int(begin, end, fmt::detail::error_handler()),
      fmt::format_error, "number is too big");
  s = "2147483649";
  begin = s.begin();
  end = s.end();
  EXPECT_THROW_MSG(
      parse_nonnegative_int(begin, end, fmt::detail::error_handler()),
      fmt::format_error, "number is too big");
}

TEST(IteratorTest, CountingIterator) {
  fmt::detail::counting_iterator it;
  auto prev = it++;
  EXPECT_EQ(prev.count(), 0);
  EXPECT_EQ(it.count(), 1);
}

TEST(IteratorTest, TruncatingIterator) {
  char* p = nullptr;
  fmt::detail::truncating_iterator<char*> it(p, 3);
  auto prev = it++;
  EXPECT_EQ(prev.base(), p);
  EXPECT_EQ(it.base(), p + 1);
}

TEST(IteratorTest, TruncatingBackInserter) {
  std::string buffer;
  auto bi = std::back_inserter(buffer);
  fmt::detail::truncating_iterator<decltype(bi)> it(bi, 2);
  *it++ = '4';
  *it++ = '2';
  *it++ = '1';
  EXPECT_EQ(buffer.size(), 2);
  EXPECT_EQ(buffer, "42");
}

TEST(IteratorTest, IsOutputIterator) {
  EXPECT_TRUE(fmt::detail::is_output_iterator<char*>::value);
  EXPECT_FALSE(fmt::detail::is_output_iterator<const char*>::value);
  EXPECT_FALSE(fmt::detail::is_output_iterator<std::string>::value);
  EXPECT_TRUE(fmt::detail::is_output_iterator<
              std::back_insert_iterator<std::string>>::value);
  EXPECT_TRUE(fmt::detail::is_output_iterator<std::string::iterator>::value);
  EXPECT_FALSE(
      fmt::detail::is_output_iterator<std::string::const_iterator>::value);
  EXPECT_FALSE(fmt::detail::is_output_iterator<std::list<char>>::value);
  EXPECT_TRUE(
      fmt::detail::is_output_iterator<std::list<char>::iterator>::value);
  EXPECT_FALSE(
      fmt::detail::is_output_iterator<std::list<char>::const_iterator>::value);
  EXPECT_FALSE(fmt::detail::is_output_iterator<uint32_pair>::value);
}

TEST(MemoryBufferTest, Ctor) {
  basic_memory_buffer<char, 123> buffer;
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
}

static void check_forwarding(mock_allocator<int>& alloc,
                             allocator_ref<mock_allocator<int>>& ref) {
  int mem;
  // Check if value_type is properly defined.
  allocator_ref<mock_allocator<int>>::value_type* ptr = &mem;
  // Check forwarding.
  EXPECT_CALL(alloc, allocate(42)).WillOnce(testing::Return(ptr));
  ref.allocate(42);
  EXPECT_CALL(alloc, deallocate(ptr, 42));
  ref.deallocate(ptr, 42);
}

TEST(AllocatorTest, allocator_ref) {
  StrictMock<mock_allocator<int>> alloc;
  typedef allocator_ref<mock_allocator<int>> test_allocator_ref;
  test_allocator_ref ref(&alloc);
  // Check if allocator_ref forwards to the underlying allocator.
  check_forwarding(alloc, ref);
  test_allocator_ref ref2(ref);
  check_forwarding(alloc, ref2);
  test_allocator_ref ref3;
  EXPECT_EQ(nullptr, ref3.get());
  ref3 = ref;
  check_forwarding(alloc, ref3);
}

typedef allocator_ref<std::allocator<char>> TestAllocator;

static void check_move_buffer(
    const char* str, basic_memory_buffer<char, 5, TestAllocator>& buffer) {
  std::allocator<char>* alloc = buffer.get_allocator().get();
  basic_memory_buffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
  // Move should transfer allocator.
  EXPECT_EQ(nullptr, buffer.get_allocator().get());
  EXPECT_EQ(alloc, buffer2.get_allocator().get());
}

TEST(MemoryBufferTest, MoveCtorInlineBuffer) {
  std::allocator<char> alloc;
  basic_memory_buffer<char, 5, TestAllocator> buffer((TestAllocator(&alloc)));
  const char test[] = "test";
  buffer.append(test, test + 4);
  check_move_buffer("test", buffer);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  buffer.push_back('a');
  check_move_buffer("testa", buffer);
}

TEST(MemoryBufferTest, MoveCtorDynamicBuffer) {
  std::allocator<char> alloc;
  basic_memory_buffer<char, 4, TestAllocator> buffer((TestAllocator(&alloc)));
  const char test[] = "test";
  buffer.append(test, test + 4);
  const char* inline_buffer_ptr = &buffer[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  buffer.push_back('a');
  basic_memory_buffer<char, 4, TestAllocator> buffer2(std::move(buffer));
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testa", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 4u);
}

static void check_move_assign_buffer(const char* str,
                                     basic_memory_buffer<char, 5>& buffer) {
  basic_memory_buffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
}

TEST(MemoryBufferTest, MoveAssignment) {
  basic_memory_buffer<char, 5> buffer;
  const char test[] = "test";
  buffer.append(test, test + 4);
  check_move_assign_buffer("test", buffer);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  buffer.push_back('a');
  check_move_assign_buffer("testa", buffer);
  const char* inline_buffer_ptr = &buffer[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  buffer.push_back('b');
  basic_memory_buffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testab", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 5u);
}

TEST(MemoryBufferTest, Grow) {
  typedef allocator_ref<mock_allocator<int>> Allocator;
  typedef basic_memory_buffer<int, 10, Allocator> Base;
  mock_allocator<int> alloc;
  struct TestMemoryBuffer : Base {
    TestMemoryBuffer(Allocator alloc) : Base(alloc) {}
    void grow(size_t size) { Base::grow(size); }
  } buffer((Allocator(&alloc)));
  buffer.resize(7);
  using fmt::detail::to_unsigned;
  for (int i = 0; i < 7; ++i) buffer[to_unsigned(i)] = i * i;
  EXPECT_EQ(10u, buffer.capacity());
  int mem[20];
  mem[7] = 0xdead;
  EXPECT_CALL(alloc, allocate(20)).WillOnce(Return(mem));
  buffer.grow(20);
  EXPECT_EQ(20u, buffer.capacity());
  // Check if size elements have been copied
  for (int i = 0; i < 7; ++i) EXPECT_EQ(i * i, buffer[to_unsigned(i)]);
  // and no more than that.
  EXPECT_EQ(0xdead, buffer[7]);
  EXPECT_CALL(alloc, deallocate(mem, 20));
}

TEST(MemoryBufferTest, Allocator) {
  typedef allocator_ref<mock_allocator<char>> TestAllocator;
  basic_memory_buffer<char, 10, TestAllocator> buffer;
  EXPECT_EQ(nullptr, buffer.get_allocator().get());
  StrictMock<mock_allocator<char>> alloc;
  char mem;
  {
    basic_memory_buffer<char, 10, TestAllocator> buffer2(
        (TestAllocator(&alloc)));
    EXPECT_EQ(&alloc, buffer2.get_allocator().get());
    size_t size = 2 * fmt::inline_buffer_size;
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem));
    buffer2.reserve(size);
    EXPECT_CALL(alloc, deallocate(&mem, size));
  }
}

TEST(MemoryBufferTest, ExceptionInDeallocate) {
  typedef allocator_ref<mock_allocator<char>> TestAllocator;
  StrictMock<mock_allocator<char>> alloc;
  basic_memory_buffer<char, 10, TestAllocator> buffer((TestAllocator(&alloc)));
  size_t size = 2 * fmt::inline_buffer_size;
  std::vector<char> mem(size);
  {
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem[0]));
    buffer.resize(size);
    std::fill(&buffer[0], &buffer[0] + size, 'x');
  }
  std::vector<char> mem2(2 * size);
  {
    EXPECT_CALL(alloc, allocate(2 * size)).WillOnce(Return(&mem2[0]));
    std::exception e;
    EXPECT_CALL(alloc, deallocate(&mem[0], size)).WillOnce(testing::Throw(e));
    EXPECT_THROW(buffer.reserve(2 * size), std::exception);
    EXPECT_EQ(&mem2[0], &buffer[0]);
    // Check that the data has been copied.
    for (size_t i = 0; i < size; ++i) EXPECT_EQ('x', buffer[i]);
  }
  EXPECT_CALL(alloc, deallocate(&mem2[0], 2 * size));
}

TEST(UtilTest, UTF8ToUTF16) {
  fmt::detail::utf8_to_utf16 u("лошадка");
  EXPECT_EQ(L"\x043B\x043E\x0448\x0430\x0434\x043A\x0430", u.str());
  EXPECT_EQ(7, u.size());
  // U+10437 { DESERET SMALL LETTER YEE }
  EXPECT_EQ(L"\xD801\xDC37", fmt::detail::utf8_to_utf16("𐐷").str());
  EXPECT_THROW_MSG(fmt::detail::utf8_to_utf16("\xc3\x28"), std::runtime_error,
                   "invalid utf8");
  EXPECT_THROW_MSG(fmt::detail::utf8_to_utf16(fmt::string_view("л", 1)),
                   std::runtime_error, "invalid utf8");
  EXPECT_EQ(L"123456", fmt::detail::utf8_to_utf16("123456").str());
}

TEST(UtilTest, UTF8ToUTF16EmptyString) {
  std::string s = "";
  fmt::detail::utf8_to_utf16 u(s.c_str());
  EXPECT_EQ(L"", u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, FormatSystemError) {
  fmt::memory_buffer message;
  fmt::format_system_error(message, EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)),
            to_string(message));
  message = fmt::memory_buffer();

  // Check if std::allocator throws on allocating max size_t / 2 chars.
  size_t max_size = max_value<size_t>() / 2;
  bool throws_on_alloc = false;
  try {
    std::allocator<char> alloc;
    alloc.deallocate(alloc.allocate(max_size), max_size);
  } catch (const std::bad_alloc&) {
    throws_on_alloc = true;
  }
  if (!throws_on_alloc) {
    fmt::print("warning: std::allocator allocates {} chars", max_size);
    return;
  }
  fmt::format_system_error(message, EDOM, fmt::string_view(nullptr, max_size));
  EXPECT_EQ(fmt::format("error {}", EDOM), to_string(message));
}

TEST(UtilTest, SystemError) {
  fmt::system_error e(EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)), e.what());
  EXPECT_EQ(EDOM, e.error_code());

  fmt::system_error error(0, "");
  try {
    throw fmt::system_error(EDOM, "test {}", "error");
  } catch (const fmt::system_error& e) {
    error = e;
  }
  fmt::memory_buffer message;
  fmt::format_system_error(message, EDOM, "test error");
  EXPECT_EQ(to_string(message), error.what());
  EXPECT_EQ(EDOM, error.error_code());
}

TEST(UtilTest, ReportSystemError) {
  fmt::memory_buffer out;
  fmt::format_system_error(out, EDOM, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr, fmt::report_system_error(EDOM, "test error"),
               to_string(out));
}

TEST(StringViewTest, Ctor) {
  EXPECT_STREQ("abc", string_view("abc").data());
  EXPECT_EQ(3u, string_view("abc").size());

  EXPECT_STREQ("defg", string_view(std::string("defg")).data());
  EXPECT_EQ(4u, string_view(std::string("defg")).size());
}

TEST(FormatToTest, FormatWithoutArgs) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "test");
  EXPECT_EQ("test", s);
}

TEST(FormatToTest, Format) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "part{0}", 1);
  EXPECT_EQ("part1", s);
  fmt::format_to(std::back_inserter(s), "part{0}", 2);
  EXPECT_EQ("part1part2", s);
}

TEST(FormatToTest, WideString) {
  std::vector<wchar_t> buf;
  fmt::format_to(std::back_inserter(buf), L"{}{}", 42, L'\0');
  EXPECT_STREQ(buf.data(), L"42");
}

TEST(FormatToTest, FormatToMemoryBuffer) {
  fmt::basic_memory_buffer<char, 100> buffer;
  fmt::format_to(buffer, "{}", "foo");
  EXPECT_EQ("foo", to_string(buffer));
  fmt::wmemory_buffer wbuffer;
  fmt::format_to(wbuffer, L"{}", L"foo");
  EXPECT_EQ(L"foo", to_string(wbuffer));
}

TEST(FormatterTest, Escape) {
  EXPECT_EQ("{", format("{{"));
  EXPECT_EQ("before {", format("before {{"));
  EXPECT_EQ("{ after", format("{{ after"));
  EXPECT_EQ("before { after", format("before {{ after"));

  EXPECT_EQ("}", format("}}"));
  EXPECT_EQ("before }", format("before }}"));
  EXPECT_EQ("} after", format("}} after"));
  EXPECT_EQ("before } after", format("before }} after"));

  EXPECT_EQ("{}", format("{{}}"));
  EXPECT_EQ("{42}", format("{{{0}}}", 42));
}

TEST(FormatterTest, UnmatchedBraces) {
  EXPECT_THROW_MSG(format("{"), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("}"), format_error, "unmatched '}' in format string");
  EXPECT_THROW_MSG(format("{0{}"), format_error, "invalid format string");
}

TEST(FormatterTest, NoArgs) { EXPECT_EQ("test", format("test")); }

TEST(FormatterTest, ArgsInDifferentPositions) {
  EXPECT_EQ("42", format("{0}", 42));
  EXPECT_EQ("before 42", format("before {0}", 42));
  EXPECT_EQ("42 after", format("{0} after", 42));
  EXPECT_EQ("before 42 after", format("before {0} after", 42));
  EXPECT_EQ("answer = 42", format("{0} = {1}", "answer", 42));
  EXPECT_EQ("42 is the answer", format("{1} is the {0}", "answer", 42));
  EXPECT_EQ("abracadabra", format("{0}{1}{0}", "abra", "cad"));
}

TEST(FormatterTest, ArgErrors) {
  EXPECT_THROW_MSG(format("{"), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{?}"), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0"), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0}"), format_error, "argument not found");
  EXPECT_THROW_MSG(format("{00}", 42), format_error, "invalid format string");

  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{%u", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), format_error, "invalid format string");
  safe_sprintf(format_str, "{%u}", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), format_error, "argument not found");

  safe_sprintf(format_str, "{%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), format_error, "number is too big");
  safe_sprintf(format_str, "{%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), format_error, "number is too big");
}

template <int N> struct TestFormat {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args&... args) {
    return TestFormat<N - 1>::format(format_str, N - 1, args...);
  }
};

template <> struct TestFormat<0> {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args&... args) {
    return fmt::format(format_str, args...);
  }
};

TEST(FormatterTest, ManyArgs) {
  EXPECT_EQ("19", TestFormat<20>::format("{19}"));
  EXPECT_THROW_MSG(TestFormat<20>::format("{20}"), format_error,
                   "argument not found");
  EXPECT_THROW_MSG(TestFormat<21>::format("{21}"), format_error,
                   "argument not found");
  enum { max_packed_args = fmt::detail::max_packed_args };
  std::string format_str = fmt::format("{{{}}}", max_packed_args + 1);
  EXPECT_THROW_MSG(TestFormat<max_packed_args>::format(format_str),
                   format_error, "argument not found");
}

TEST(FormatterTest, NamedArg) {
  EXPECT_EQ("1/a/A", format("{_1}/{a_}/{A_}", fmt::arg("a_", 'a'),
                            fmt::arg("A_", "A"), fmt::arg("_1", 1)));
  EXPECT_EQ(" -42", format("{0:{width}}", -42, fmt::arg("width", 4)));
  EXPECT_EQ("st", format("{0:.{precision}}", "str", fmt::arg("precision", 2)));
  EXPECT_EQ("1 2", format("{} {two}", 1, fmt::arg("two", 2)));
  EXPECT_EQ("42", format("{c}", fmt::arg("a", 0), fmt::arg("b", 0),
                         fmt::arg("c", 42), fmt::arg("d", 0), fmt::arg("e", 0),
                         fmt::arg("f", 0), fmt::arg("g", 0), fmt::arg("h", 0),
                         fmt::arg("i", 0), fmt::arg("j", 0), fmt::arg("k", 0),
                         fmt::arg("l", 0), fmt::arg("m", 0), fmt::arg("n", 0),
                         fmt::arg("o", 0), fmt::arg("p", 0)));
  EXPECT_THROW_MSG(format("{a}"), format_error, "argument not found");
  EXPECT_THROW_MSG(format("{a}", 42), format_error, "argument not found");
}

TEST(FormatterTest, AutoArgIndex) {
  EXPECT_EQ("abc", format("{}{}{}", 'a', 'b', 'c'));
  EXPECT_THROW_MSG(format("{0}{}", 'a', 'b'), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{}{0}", 'a', 'b'), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_EQ("1.2", format("{:.{}}", 1.2345, 2));
  EXPECT_THROW_MSG(format("{0}:.{}", 1.2345, 2), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{:.{0}}", 1.2345, 2), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(format("{}"), format_error, "argument not found");
}

TEST(FormatterTest, EmptySpecs) { EXPECT_EQ("42", format("{0:}", 42)); }

TEST(FormatterTest, LeftAlign) {
  EXPECT_EQ("42  ", format("{0:<4}", 42));
  EXPECT_EQ("42  ", format("{0:<4o}", 042));
  EXPECT_EQ("42  ", format("{0:<4x}", 0x42));
  EXPECT_EQ("-42  ", format("{0:<5}", -42));
  EXPECT_EQ("42   ", format("{0:<5}", 42u));
  EXPECT_EQ("-42  ", format("{0:<5}", -42l));
  EXPECT_EQ("42   ", format("{0:<5}", 42ul));
  EXPECT_EQ("-42  ", format("{0:<5}", -42ll));
  EXPECT_EQ("42   ", format("{0:<5}", 42ull));
  EXPECT_EQ("-42.0  ", format("{0:<7}", -42.0));
  EXPECT_EQ("-42.0  ", format("{0:<7}", -42.0l));
  EXPECT_EQ("c    ", format("{0:<5}", 'c'));
  EXPECT_EQ("abc  ", format("{0:<5}", "abc"));
  EXPECT_EQ("0xface  ", format("{0:<8}", reinterpret_cast<void*>(0xface)));
}

TEST(FormatterTest, RightAlign) {
  EXPECT_EQ("  42", format("{0:>4}", 42));
  EXPECT_EQ("  42", format("{0:>4o}", 042));
  EXPECT_EQ("  42", format("{0:>4x}", 0x42));
  EXPECT_EQ("  -42", format("{0:>5}", -42));
  EXPECT_EQ("   42", format("{0:>5}", 42u));
  EXPECT_EQ("  -42", format("{0:>5}", -42l));
  EXPECT_EQ("   42", format("{0:>5}", 42ul));
  EXPECT_EQ("  -42", format("{0:>5}", -42ll));
  EXPECT_EQ("   42", format("{0:>5}", 42ull));
  EXPECT_EQ("  -42.0", format("{0:>7}", -42.0));
  EXPECT_EQ("  -42.0", format("{0:>7}", -42.0l));
  EXPECT_EQ("    c", format("{0:>5}", 'c'));
  EXPECT_EQ("  abc", format("{0:>5}", "abc"));
  EXPECT_EQ("  0xface", format("{0:>8}", reinterpret_cast<void*>(0xface)));
}

#if FMT_DEPRECATED_NUMERIC_ALIGN
TEST(FormatterTest, NumericAlign) { EXPECT_EQ("0042", format("{0:=4}", 42)); }
#endif

TEST(FormatterTest, CenterAlign) {
  EXPECT_EQ(" 42  ", format("{0:^5}", 42));
  EXPECT_EQ(" 42  ", format("{0:^5o}", 042));
  EXPECT_EQ(" 42  ", format("{0:^5x}", 0x42));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42u));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42l));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42ul));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42ll));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42ull));
  EXPECT_EQ(" -42.0 ", format("{0:^7}", -42.0));
  EXPECT_EQ(" -42.0 ", format("{0:^7}", -42.0l));
  EXPECT_EQ("  c  ", format("{0:^5}", 'c'));
  EXPECT_EQ(" abc  ", format("{0:^6}", "abc"));
  EXPECT_EQ(" 0xface ", format("{0:^8}", reinterpret_cast<void*>(0xface)));
}

TEST(FormatterTest, Fill) {
  EXPECT_THROW_MSG(format("{0:{<5}", 'c'), format_error,
                   "invalid fill character '{'");
  EXPECT_THROW_MSG(format("{0:{<5}}", 'c'), format_error,
                   "invalid fill character '{'");
  EXPECT_EQ("**42", format("{0:*>4}", 42));
  EXPECT_EQ("**-42", format("{0:*>5}", -42));
  EXPECT_EQ("***42", format("{0:*>5}", 42u));
  EXPECT_EQ("**-42", format("{0:*>5}", -42l));
  EXPECT_EQ("***42", format("{0:*>5}", 42ul));
  EXPECT_EQ("**-42", format("{0:*>5}", -42ll));
  EXPECT_EQ("***42", format("{0:*>5}", 42ull));
  EXPECT_EQ("**-42.0", format("{0:*>7}", -42.0));
  EXPECT_EQ("**-42.0", format("{0:*>7}", -42.0l));
  EXPECT_EQ("c****", format("{0:*<5}", 'c'));
  EXPECT_EQ("abc**", format("{0:*<5}", "abc"));
  EXPECT_EQ("**0xface", format("{0:*>8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("foo=", format("{:}=", "foo"));
  EXPECT_EQ(std::string("\0\0\0*", 4), format(string_view("{:\0>4}", 6), '*'));
  EXPECT_EQ("жж42", format("{0:ж>4}", 42));
  EXPECT_THROW_MSG(format("{:\x80\x80\x80\x80\x80>}", 0), format_error,
                   "invalid fill");
}

TEST(FormatterTest, PlusSign) {
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_EQ("-42", format("{0:+}", -42));
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_THROW_MSG(format("{0:+}", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42l));
  EXPECT_THROW_MSG(format("{0:+}", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42ll));
  EXPECT_THROW_MSG(format("{0:+}", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("+42.0", format("{0:+}", 42.0));
  EXPECT_EQ("+42.0", format("{0:+}", 42.0l));
  EXPECT_THROW_MSG(format("{0:+", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:+}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:+}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:+}", reinterpret_cast<void*>(0x42)), format_error,
                   "format specifier requires numeric argument");
}

TEST(FormatterTest, MinusSign) {
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_EQ("-42", format("{0:-}", -42));
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_THROW_MSG(format("{0:-}", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42l));
  EXPECT_THROW_MSG(format("{0:-}", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42ll));
  EXPECT_THROW_MSG(format("{0:-}", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ("42.0", format("{0:-}", 42.0));
  EXPECT_EQ("42.0", format("{0:-}", 42.0l));
  EXPECT_THROW_MSG(format("{0:-", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:-}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:-}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", reinterpret_cast<void*>(0x42)), format_error,
                   "format specifier requires numeric argument");
}

TEST(FormatterTest, SpaceSign) {
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_EQ("-42", format("{0: }", -42));
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_THROW_MSG(format("{0: }", 42u), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42l));
  EXPECT_THROW_MSG(format("{0: }", 42ul), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42ll));
  EXPECT_THROW_MSG(format("{0: }", 42ull), format_error,
                   "format specifier requires signed argument");
  EXPECT_EQ(" 42.0", format("{0: }", 42.0));
  EXPECT_EQ(" 42.0", format("{0: }", 42.0l));
  EXPECT_THROW_MSG(format("{0: ", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0: }", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0: }", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", reinterpret_cast<void*>(0x42)), format_error,
                   "format specifier requires numeric argument");
}

TEST(FormatterTest, HashFlag) {
  EXPECT_EQ("42", format("{0:#}", 42));
  EXPECT_EQ("-42", format("{0:#}", -42));
  EXPECT_EQ("0b101010", format("{0:#b}", 42));
  EXPECT_EQ("0B101010", format("{0:#B}", 42));
  EXPECT_EQ("-0b101010", format("{0:#b}", -42));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42));
  EXPECT_EQ("0X42", format("{0:#X}", 0x42));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42));
  EXPECT_EQ("0", format("{0:#o}", 0));
  EXPECT_EQ("042", format("{0:#o}", 042));
  EXPECT_EQ("-042", format("{0:#o}", -042));
  EXPECT_EQ("42", format("{0:#}", 42u));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42u));
  EXPECT_EQ("042", format("{0:#o}", 042u));

  EXPECT_EQ("-42", format("{0:#}", -42l));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42l));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42l));
  EXPECT_EQ("042", format("{0:#o}", 042l));
  EXPECT_EQ("-042", format("{0:#o}", -042l));
  EXPECT_EQ("42", format("{0:#}", 42ul));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ul));
  EXPECT_EQ("042", format("{0:#o}", 042ul));

  EXPECT_EQ("-42", format("{0:#}", -42ll));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ll));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42ll));
  EXPECT_EQ("042", format("{0:#o}", 042ll));
  EXPECT_EQ("-042", format("{0:#o}", -042ll));
  EXPECT_EQ("42", format("{0:#}", 42ull));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ull));
  EXPECT_EQ("042", format("{0:#o}", 042ull));

  EXPECT_EQ("-42.0", format("{0:#}", -42.0));
  EXPECT_EQ("-42.0", format("{0:#}", -42.0l));
  EXPECT_EQ("4.e+01", format("{:#.0e}", 42.0));
  EXPECT_EQ("0.", format("{:#.0f}", 0.01));
  auto s = format("{:#.0f}", 0.5);  // MSVC's printf uses wrong rounding mode.
  EXPECT_TRUE(s == "0." || s == "1.");
  EXPECT_THROW_MSG(format("{0:#", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:#}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:#}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", reinterpret_cast<void*>(0x42)), format_error,
                   "format specifier requires numeric argument");
}

TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", format("{0:0}", 42));
  EXPECT_EQ("-0042", format("{0:05}", -42));
  EXPECT_EQ("00042", format("{0:05}", 42u));
  EXPECT_EQ("-0042", format("{0:05}", -42l));
  EXPECT_EQ("00042", format("{0:05}", 42ul));
  EXPECT_EQ("-0042", format("{0:05}", -42ll));
  EXPECT_EQ("00042", format("{0:05}", 42ull));
  EXPECT_EQ("-0042.0", format("{0:07}", -42.0));
  EXPECT_EQ("-0042.0", format("{0:07}", -42.0l));
  EXPECT_THROW_MSG(format("{0:0", 'c'), format_error,
                   "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:05}", 'c'), format_error,
                   "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:05}", "abc"), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", reinterpret_cast<void*>(0x42)),
                   format_error, "format specifier requires numeric argument");
}

TEST(FormatterTest, Width) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:%u", UINT_MAX);
  increment(format_str + 3);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  safe_sprintf(format_str, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  safe_sprintf(format_str, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  EXPECT_EQ(" -42", format("{0:4}", -42));
  EXPECT_EQ("   42", format("{0:5}", 42u));
  EXPECT_EQ("   -42", format("{0:6}", -42l));
  EXPECT_EQ("     42", format("{0:7}", 42ul));
  EXPECT_EQ("   -42", format("{0:6}", -42ll));
  EXPECT_EQ("     42", format("{0:7}", 42ull));
  EXPECT_EQ("   -1.23", format("{0:8}", -1.23));
  EXPECT_EQ("    -1.23", format("{0:9}", -1.23l));
  EXPECT_EQ("    0xcafe", format("{0:10}", reinterpret_cast<void*>(0xcafe)));
  EXPECT_EQ("x          ", format("{0:11}", 'x'));
  EXPECT_EQ("str         ", format("{0:12}", "str"));
  EXPECT_EQ(fmt::format("{:*^5}", "🤡"), "**🤡**");
}

template <typename T> inline T const_check(T value) { return value; }

TEST(FormatterTest, RuntimeWidth) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:{%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:{", 0), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:{}", 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{0:{?}}", 0), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:{1}}", 0), format_error, "argument not found");

  EXPECT_THROW_MSG(format("{0:{0:}}", 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG(format("{0:{1}}", 0, -1), format_error, "negative width");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, (INT_MAX + 1u)), format_error,
                   "number is too big");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, -1l), format_error, "negative width");
  if (const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(format("{0:{1}}", 0, (value + 1)), format_error,
                     "number is too big");
  }
  EXPECT_THROW_MSG(format("{0:{1}}", 0, (INT_MAX + 1ul)), format_error,
                   "number is too big");

  EXPECT_THROW_MSG(format("{0:{1}}", 0, '0'), format_error,
                   "width is not integer");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, 0.0), format_error,
                   "width is not integer");

  EXPECT_EQ(" -42", format("{0:{1}}", -42, 4));
  EXPECT_EQ("   42", format("{0:{1}}", 42u, 5));
  EXPECT_EQ("   -42", format("{0:{1}}", -42l, 6));
  EXPECT_EQ("     42", format("{0:{1}}", 42ul, 7));
  EXPECT_EQ("   -42", format("{0:{1}}", -42ll, 6));
  EXPECT_EQ("     42", format("{0:{1}}", 42ull, 7));
  EXPECT_EQ("   -1.23", format("{0:{1}}", -1.23, 8));
  EXPECT_EQ("    -1.23", format("{0:{1}}", -1.23l, 9));
  EXPECT_EQ("    0xcafe",
            format("{0:{1}}", reinterpret_cast<void*>(0xcafe), 10));
  EXPECT_EQ("x          ", format("{0:{1}}", 'x', 11));
  EXPECT_EQ("str         ", format("{0:{1}}", "str", 12));
}

TEST(FormatterTest, Precision) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:.%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  safe_sprintf(format_str, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  safe_sprintf(format_str, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:.", 0), format_error,
                   "missing precision specifier");
  EXPECT_THROW_MSG(format("{0:.}", 0), format_error,
                   "missing precision specifier");

  EXPECT_THROW_MSG(format("{0:.2", 0), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42u), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42l), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ul), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ll), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ull), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:3.0}", 'x'), format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345));
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345l));
  EXPECT_EQ("1.2e+56", format("{:.2}", 1.234e56));
  EXPECT_EQ("1e+00", format("{:.0e}", 1.0L));
  EXPECT_EQ("  0.0e+00", format("{:9.1e}", 0.0));
  EXPECT_EQ(
      "4.9406564584124654417656879286822137236505980261432476442558568250067550"
      "727020875186529983636163599237979656469544571773092665671035593979639877"
      "479601078187812630071319031140452784581716784898210368871863605699873072"
      "305000638740915356498438731247339727316961514003171538539807412623856559"
      "117102665855668676818703956031062493194527159149245532930545654440112748"
      "012970999954193198940908041656332452475714786901472678015935523861155013"
      "480352649347201937902681071074917033322268447533357208324319361e-324",
      format("{:.494}", 4.9406564584124654E-324));
  EXPECT_EQ(
      "-0X1.41FE3FFE71C9E000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000P+127",
      format("{:.838A}", -2.14001164E+38));
  EXPECT_EQ("123.", format("{:#.0f}", 123.0));
  EXPECT_EQ("1.23", format("{:.02f}", 1.234));
  EXPECT_EQ("0.001", format("{:.1g}", 0.001));

  EXPECT_THROW_MSG(format("{0:.2}", reinterpret_cast<void*>(0xcafe)),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", reinterpret_cast<void*>(0xcafe)),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{:.{}e}", 42.0, fmt::detail::max_value<int>()),
                   format_error, "number is too big");

  EXPECT_EQ("st", format("{0:.2}", "str"));
}

TEST(FormatterTest, RuntimePrecision) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:.{%u", UINT_MAX);
  increment(format_str + 5);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:.{", 0), format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:.{}", 0), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{0:.{?}}", 0), format_error,
                   "invalid format string");
  EXPECT_THROW_MSG(format("{0:.{1}", 0, 0), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0), format_error, "argument not found");

  EXPECT_THROW_MSG(format("{0:.{0:}}", 0), format_error,
                   "invalid format string");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1), format_error,
                   "negative precision");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1u)), format_error,
                   "number is too big");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1l), format_error,
                   "negative precision");
  if (const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(format("{0:.{1}}", 0, (value + 1)), format_error,
                     "number is too big");
  }
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1ul)), format_error,
                   "number is too big");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, '0'), format_error,
                   "precision is not integer");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, 0.0), format_error,
                   "precision is not integer");

  EXPECT_THROW_MSG(format("{0:.{1}}", 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42u, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42u, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42l, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42l, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ul, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ul, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ll, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ll, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ull, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ull, 2), format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:3.{1}}", 'x', 0), format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2", format("{0:.{1}}", 1.2345, 2));
  EXPECT_EQ("1.2", format("{1:.{0}}", 2, 1.2345l));

  EXPECT_THROW_MSG(format("{0:.{1}}", reinterpret_cast<void*>(0xcafe), 2),
                   format_error,
                   "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", reinterpret_cast<void*>(0xcafe), 2),
                   format_error,
                   "precision not allowed for this argument type");

  EXPECT_EQ("st", format("{0:.{1}}", "str", 2));
}

template <typename T>
void check_unknown_types(const T& value, const char* types, const char*) {
  char format_str[BUFFER_SIZE];
  const char* special = ".0123456789}";
  for (int i = CHAR_MIN; i <= CHAR_MAX; ++i) {
    char c = static_cast<char>(i);
    if (std::strchr(types, c) || std::strchr(special, c) || !c) continue;
    safe_sprintf(format_str, "{0:10%c}", c);
    const char* message = "invalid type specifier";
    EXPECT_THROW_MSG(format(format_str, value), format_error, message)
        << format_str << " " << message;
  }
}

TEST(BoolTest, FormatBool) {
  EXPECT_EQ("true", format("{}", true));
  EXPECT_EQ("false", format("{}", false));
  EXPECT_EQ("1", format("{:d}", true));
  EXPECT_EQ("true ", format("{:5}", true));
  EXPECT_EQ(L"true", format(L"{}", true));
}

TEST(FormatterTest, FormatShort) {
  short s = 42;
  EXPECT_EQ("42", format("{0:d}", s));
  unsigned short us = 42;
  EXPECT_EQ("42", format("{0:d}", us));
}

TEST(FormatterTest, FormatInt) {
  EXPECT_THROW_MSG(format("{0:v", 42), format_error,
                   "missing '}' in format string");
  check_unknown_types(42, "bBdoxXnLc", "integer");
  EXPECT_EQ("x", format("{:c}", static_cast<int>('x')));
}

TEST(FormatterTest, FormatBin) {
  EXPECT_EQ("0", format("{0:b}", 0));
  EXPECT_EQ("101010", format("{0:b}", 42));
  EXPECT_EQ("101010", format("{0:b}", 42u));
  EXPECT_EQ("-101010", format("{0:b}", -42));
  EXPECT_EQ("11000000111001", format("{0:b}", 12345));
  EXPECT_EQ("10010001101000101011001111000", format("{0:b}", 0x12345678));
  EXPECT_EQ("10010000101010111100110111101111", format("{0:b}", 0x90ABCDEF));
  EXPECT_EQ("11111111111111111111111111111111",
            format("{0:b}", max_value<uint32_t>()));
}

#if FMT_USE_INT128
constexpr auto int128_max = static_cast<__int128_t>(
    (static_cast<__uint128_t>(1) << ((__SIZEOF_INT128__ * CHAR_BIT) - 1)) - 1);
constexpr auto int128_min = -int128_max - 1;

constexpr auto uint128_max = ~static_cast<__uint128_t>(0);
#endif

TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", format("{0}", 0));
  EXPECT_EQ("42", format("{0}", 42));
  EXPECT_EQ("42", format("{0:d}", 42));
  EXPECT_EQ("42", format("{0}", 42u));
  EXPECT_EQ("-42", format("{0}", -42));
  EXPECT_EQ("12345", format("{0}", 12345));
  EXPECT_EQ("67890", format("{0}", 67890));
#if FMT_USE_INT128
  EXPECT_EQ("0", format("{0}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", format("{0}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("9223372036854775808",
            format("{0}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-9223372036854775809",
            format("{0}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("18446744073709551616",
            format("{0}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("170141183460469231731687303715884105727",
            format("{0}", int128_max));
  EXPECT_EQ("-170141183460469231731687303715884105728",
            format("{0}", int128_min));
  EXPECT_EQ("340282366920938463463374607431768211455",
            format("{0}", uint128_max));
#endif

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, format("{0}", INT_MIN));
  safe_sprintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, format("{0}", INT_MAX));
  safe_sprintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, format("{0}", UINT_MAX));
  safe_sprintf(buffer, "%ld", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0}", LONG_MIN));
  safe_sprintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, format("{0}", LONG_MAX));
  safe_sprintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0}", ULONG_MAX));
}

TEST(FormatterTest, FormatHex) {
  EXPECT_EQ("0", format("{0:x}", 0));
  EXPECT_EQ("42", format("{0:x}", 0x42));
  EXPECT_EQ("42", format("{0:x}", 0x42u));
  EXPECT_EQ("-42", format("{0:x}", -0x42));
  EXPECT_EQ("12345678", format("{0:x}", 0x12345678));
  EXPECT_EQ("90abcdef", format("{0:x}", 0x90abcdef));
  EXPECT_EQ("12345678", format("{0:X}", 0x12345678));
  EXPECT_EQ("90ABCDEF", format("{0:X}", 0x90ABCDEF));
#if FMT_USE_INT128
  EXPECT_EQ("0", format("{0:x}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", format("{0:x}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("8000000000000000",
            format("{0:x}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-8000000000000001",
            format("{0:x}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("10000000000000000",
            format("{0:x}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("7fffffffffffffffffffffffffffffff", format("{0:x}", int128_max));
  EXPECT_EQ("-80000000000000000000000000000000", format("{0:x}", int128_min));
  EXPECT_EQ("ffffffffffffffffffffffffffffffff", format("{0:x}", uint128_max));
#endif

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "-%x", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, format("{0:x}", INT_MIN));
  safe_sprintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, format("{0:x}", INT_MAX));
  safe_sprintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, format("{0:x}", UINT_MAX));
  safe_sprintf(buffer, "-%lx", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0:x}", LONG_MIN));
  safe_sprintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, format("{0:x}", LONG_MAX));
  safe_sprintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0:x}", ULONG_MAX));
}

TEST(FormatterTest, FormatOct) {
  EXPECT_EQ("0", format("{0:o}", 0));
  EXPECT_EQ("42", format("{0:o}", 042));
  EXPECT_EQ("42", format("{0:o}", 042u));
  EXPECT_EQ("-42", format("{0:o}", -042));
  EXPECT_EQ("12345670", format("{0:o}", 012345670));
#if FMT_USE_INT128
  EXPECT_EQ("0", format("{0:o}", static_cast<__int128_t>(0)));
  EXPECT_EQ("0", format("{0:o}", static_cast<__uint128_t>(0)));
  EXPECT_EQ("1000000000000000000000",
            format("{0:o}", static_cast<__int128_t>(INT64_MAX) + 1));
  EXPECT_EQ("-1000000000000000000001",
            format("{0:o}", static_cast<__int128_t>(INT64_MIN) - 1));
  EXPECT_EQ("2000000000000000000000",
            format("{0:o}", static_cast<__int128_t>(UINT64_MAX) + 1));
  EXPECT_EQ("1777777777777777777777777777777777777777777",
            format("{0:o}", int128_max));
  EXPECT_EQ("-2000000000000000000000000000000000000000000",
            format("{0:o}", int128_min));
  EXPECT_EQ("3777777777777777777777777777777777777777777",
            format("{0:o}", uint128_max));
#endif

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "-%o", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, format("{0:o}", INT_MIN));
  safe_sprintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, format("{0:o}", INT_MAX));
  safe_sprintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, format("{0:o}", UINT_MAX));
  safe_sprintf(buffer, "-%lo", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0:o}", LONG_MIN));
  safe_sprintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, format("{0:o}", LONG_MAX));
  safe_sprintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0:o}", ULONG_MAX));
}

TEST(FormatterTest, FormatIntLocale) {
  EXPECT_EQ("1234", format("{:L}", 1234));
}

struct ConvertibleToLongLong {
  operator long long() const { return 1LL << 32; }
};

TEST(FormatterTest, FormatConvertibleToLongLong) {
  EXPECT_EQ("100000000", format("{:x}", ConvertibleToLongLong()));
}

TEST(FormatterTest, FormatFloat) {
  EXPECT_EQ("392.500000", format("{0:f}", 392.5f));
}

TEST(FormatterTest, FormatDouble) {
  check_unknown_types(1.2, "eEfFgGaAnL%", "double");
  EXPECT_EQ("0.0", format("{:}", 0.0));
  EXPECT_EQ("0.000000", format("{:f}", 0.0));
  EXPECT_EQ("0", format("{:g}", 0.0));
  EXPECT_EQ("392.65", format("{:}", 392.65));
  EXPECT_EQ("392.65", format("{:g}", 392.65));
  EXPECT_EQ("392.65", format("{:G}", 392.65));
  EXPECT_EQ("392.650000", format("{:f}", 392.65));
  EXPECT_EQ("392.650000", format("{:F}", 392.65));
  EXPECT_EQ("42", format("{:L}", 42.0));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%e", 392.65);
  EXPECT_EQ(buffer, format("{0:e}", 392.65));
  safe_sprintf(buffer, "%E", 392.65);
  EXPECT_EQ(buffer, format("{0:E}", 392.65));
  EXPECT_EQ("+0000392.6", format("{0:+010.4g}", 392.65));
  safe_sprintf(buffer, "%a", -42.0);
  EXPECT_EQ(buffer, format("{:a}", -42.0));
  safe_sprintf(buffer, "%A", -42.0);
  EXPECT_EQ(buffer, format("{:A}", -42.0));
}

TEST(FormatterTest, PrecisionRounding) {
  EXPECT_EQ("0", format("{:.0f}", 0.0));
  EXPECT_EQ("0", format("{:.0f}", 0.01));
  EXPECT_EQ("0", format("{:.0f}", 0.1));
  EXPECT_EQ("0.000", format("{:.3f}", 0.00049));
  EXPECT_EQ("0.001", format("{:.3f}", 0.0005));
  EXPECT_EQ("0.001", format("{:.3f}", 0.00149));
  EXPECT_EQ("0.002", format("{:.3f}", 0.0015));
  EXPECT_EQ("1.000", format("{:.3f}", 0.9999));
  EXPECT_EQ("0.00123", format("{:.3}", 0.00123));
  EXPECT_EQ("0.1", format("{:.16g}", 0.1));
  // Trigger rounding error in Grisu by a carefully chosen number.
  auto n = 3788512123356.985352;
  char buffer[64];
  safe_sprintf(buffer, "%f", n);
  EXPECT_EQ(buffer, format("{:f}", n));
}

TEST(FormatterTest, FormatNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ("nan", format("{}", nan));
  EXPECT_EQ("+nan", format("{:+}", nan));
  EXPECT_EQ(" nan", format("{: }", nan));
  EXPECT_EQ("NAN", format("{:F}", nan));
  EXPECT_EQ("nan    ", format("{:<7}", nan));
  EXPECT_EQ("  nan  ", format("{:^7}", nan));
  EXPECT_EQ("    nan", format("{:>7}", nan));
}

TEST(FormatterTest, FormatInfinity) {
  double inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ("inf", format("{}", inf));
  EXPECT_EQ("+inf", format("{:+}", inf));
  EXPECT_EQ("-inf", format("{}", -inf));
  EXPECT_EQ(" inf", format("{: }", inf));
  EXPECT_EQ("INF", format("{:F}", inf));
  EXPECT_EQ("inf    ", format("{:<7}", inf));
  EXPECT_EQ("  inf  ", format("{:^7}", inf));
  EXPECT_EQ("    inf", format("{:>7}", inf));
}

TEST(FormatterTest, FormatLongDouble) {
  EXPECT_EQ("0.0", format("{0:}", 0.0l));
  EXPECT_EQ("0.000000", format("{0:f}", 0.0l));
  EXPECT_EQ("392.65", format("{0:}", 392.65l));
  EXPECT_EQ("392.65", format("{0:g}", 392.65l));
  EXPECT_EQ("392.65", format("{0:G}", 392.65l));
  EXPECT_EQ("392.650000", format("{0:f}", 392.65l));
  EXPECT_EQ("392.650000", format("{0:F}", 392.65l));
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%Le", 392.65l);
  EXPECT_EQ(buffer, format("{0:e}", 392.65l));
  EXPECT_EQ("+0000392.6", format("{0:+010.4g}", 392.64l));
  safe_sprintf(buffer, "%La", 3.31l);
  EXPECT_EQ(buffer, format("{:a}", 3.31l));
}

TEST(FormatterTest, FormatChar) {
  const char types[] = "cbBdoxXL";
  check_unknown_types('a', types, "char");
  EXPECT_EQ("a", format("{0}", 'a'));
  EXPECT_EQ("z", format("{0:c}", 'z'));
  EXPECT_EQ(L"a", format(L"{0}", 'a'));
  int n = 'x';
  for (const char* type = types + 1; *type; ++type) {
    std::string format_str = fmt::format("{{:{}}}", *type);
    EXPECT_EQ(fmt::format(format_str, n), fmt::format(format_str, 'x'));
  }
  EXPECT_EQ(fmt::format("{:02X}", n), fmt::format("{:02X}", 'x'));
}

TEST(FormatterTest, FormatVolatileChar) {
  volatile char c = 'x';
  EXPECT_EQ("x", format("{}", c));
}

TEST(FormatterTest, FormatUnsignedChar) {
  EXPECT_EQ("42", format("{}", static_cast<unsigned char>(42)));
  EXPECT_EQ("42", format("{}", static_cast<uint8_t>(42)));
}

TEST(FormatterTest, FormatWChar) {
  EXPECT_EQ(L"a", format(L"{0}", L'a'));
  // This shouldn't compile:
  // format("{}", L'a');
}

TEST(FormatterTest, FormatCString) {
  check_unknown_types("test", "sp", "string");
  EXPECT_EQ("test", format("{0}", "test"));
  EXPECT_EQ("test", format("{0:s}", "test"));
  char nonconst[] = "nonconst";
  EXPECT_EQ("nonconst", format("{0}", nonconst));
  EXPECT_THROW_MSG(format("{0}", static_cast<const char*>(nullptr)),
                   format_error, "string pointer is null");
}

TEST(FormatterTest, FormatSCharString) {
  signed char str[] = "test";
  EXPECT_EQ("test", format("{0:s}", str));
  const signed char* const_str = str;
  EXPECT_EQ("test", format("{0:s}", const_str));
}

TEST(FormatterTest, FormatUCharString) {
  unsigned char str[] = "test";
  EXPECT_EQ("test", format("{0:s}", str));
  const unsigned char* const_str = str;
  EXPECT_EQ("test", format("{0:s}", const_str));
  unsigned char* ptr = str;
  EXPECT_EQ("test", format("{0:s}", ptr));
}

TEST(FormatterTest, FormatPointer) {
  check_unknown_types(reinterpret_cast<void*>(0x1234), "p", "pointer");
  EXPECT_EQ("0x0", format("{0}", static_cast<void*>(nullptr)));
  EXPECT_EQ("0x1234", format("{0}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x1234", format("{0:p}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x" + std::string(sizeof(void*) * CHAR_BIT / 4, 'f'),
            format("{0}", reinterpret_cast<void*>(~uintptr_t())));
  EXPECT_EQ("0x1234", format("{}", fmt::ptr(reinterpret_cast<int*>(0x1234))));
  std::unique_ptr<int> up(new int(1));
  EXPECT_EQ(format("{}", fmt::ptr(up.get())), format("{}", fmt::ptr(up)));
  std::shared_ptr<int> sp(new int(1));
  EXPECT_EQ(format("{}", fmt::ptr(sp.get())), format("{}", fmt::ptr(sp)));
  EXPECT_EQ("0x0", format("{}", nullptr));
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", format("{0}", std::string("test")));
}

TEST(FormatterTest, FormatStringView) {
  EXPECT_EQ("test", format("{}", string_view("test")));
  EXPECT_EQ("", format("{}", string_view()));
}

#ifdef FMT_USE_STRING_VIEW
struct string_viewable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<string_viewable> : formatter<std::string_view> {
  auto format(string_viewable, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<std::string_view>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(FormatterTest, FormatStdStringView) {
  EXPECT_EQ("test", format("{}", std::string_view("test")));
  EXPECT_EQ("foo", format("{}", string_viewable()));
}

struct explicitly_convertible_to_std_string_view {
  explicit operator std::string_view() const { return "foo"; }
};

namespace fmt {
template <>
struct formatter<explicitly_convertible_to_std_string_view>
    : formatter<std::string_view> {
  auto format(const explicitly_convertible_to_std_string_view& v,
              format_context& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "'{}'", std::string_view(v));
  }
};
}  // namespace fmt

TEST(FormatterTest, FormatExplicitlyConvertibleToStdStringView) {
  EXPECT_EQ("'foo'",
            fmt::format("{}", explicitly_convertible_to_std_string_view()));
}
#endif

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VER || FMT_MSC_VER >= 1900
struct explicitly_convertible_to_wstring_view {
  explicit operator fmt::wstring_view() const { return L"foo"; }
};

TEST(FormatTest, FormatExplicitlyConvertibleToWStringView) {
  EXPECT_EQ(L"foo",
            fmt::format(L"{}", explicitly_convertible_to_wstring_view()));
}
#endif

namespace fake_qt {
class QString {
 public:
  QString(const wchar_t* s) : s_(std::make_shared<std::wstring>(s)) {}
  const wchar_t* utf16() const FMT_NOEXCEPT { return s_->data(); }
  int size() const FMT_NOEXCEPT { return static_cast<int>(s_->size()); }

 private:
  std::shared_ptr<std::wstring> s_;
};

fmt::basic_string_view<wchar_t> to_string_view(const QString& s) FMT_NOEXCEPT {
  return {s.utf16(), static_cast<size_t>(s.size())};
}
}  // namespace fake_qt

TEST(FormatTest, FormatForeignStrings) {
  using fake_qt::QString;
  EXPECT_EQ(fmt::format(QString(L"{}"), 42), L"42");
  EXPECT_EQ(fmt::format(QString(L"{}"), QString(L"42")), L"42");
}

FMT_BEGIN_NAMESPACE
template <> struct formatter<Date> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'd') ++it;
    return it;
  }

  auto format(const Date& d, format_context& ctx) -> decltype(ctx.out()) {
    format_to(ctx.out(), "{}-{}-{}", d.year(), d.month(), d.day());
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(FormatterTest, FormatCustom) {
  Date date(2012, 12, 9);
  EXPECT_THROW_MSG(fmt::format("{:s}", date), format_error,
                   "unknown format specifier");
}

class Answer {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<Answer> : formatter<int> {
  template <typename FormatContext>
  auto format(Answer, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
FMT_END_NAMESPACE

TEST(FormatterTest, CustomFormat) {
  EXPECT_EQ("42", format("{0}", Answer()));
  EXPECT_EQ("0042", format("{:04}", Answer()));
}

TEST(FormatterTest, CustomFormatTo) {
  char buf[10] = {};
  auto end =
      &*fmt::format_to(fmt::detail::make_checked(buf, 10), "{}", Answer());
  EXPECT_EQ(end, buf + 2);
  EXPECT_STREQ(buf, "42");
}

TEST(FormatterTest, WideFormatString) {
  EXPECT_EQ(L"42", format(L"{}", 42));
  EXPECT_EQ(L"4.2", format(L"{}", 4.2));
  EXPECT_EQ(L"abc", format(L"{}", L"abc"));
  EXPECT_EQ(L"z", format(L"{}", L'z'));
}

TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
            format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%", 1.234, 42, 3.13,
                   "str", reinterpret_cast<void*>(1000), 'X'));
}

TEST(FormatterTest, FormatExamples) {
  std::string message = format("The answer is {}", 42);
  EXPECT_EQ("The answer is 42", message);

  EXPECT_EQ("42", format("{}", 42));
  EXPECT_EQ("42", format(std::string("{}"), 42));

  memory_buffer out;
  format_to(out, "The answer is {}.", 42);
  EXPECT_EQ("The answer is 42.", to_string(out));

  const char* filename = "nonexistent";
  FILE* ftest = safe_fopen(filename, "r");
  if (ftest) fclose(ftest);
  int error_code = errno;
  EXPECT_TRUE(ftest == nullptr);
  EXPECT_SYSTEM_ERROR(
      {
        FILE* f = safe_fopen(filename, "r");
        if (!f)
          throw fmt::system_error(errno, "Cannot open file '{}'", filename);
        fclose(f);
      },
      error_code, "Cannot open file 'nonexistent'");
}

TEST(FormatterTest, Examples) {
  EXPECT_EQ("First, thou shalt count to three",
            format("First, thou shalt count to {0}", "three"));
  EXPECT_EQ("Bring me a shrubbery", format("Bring me a {}", "shrubbery"));
  EXPECT_EQ("From 1 to 3", format("From {} to {}", 1, 3));

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%03.2f", -1.2);
  EXPECT_EQ(buffer, format("{:03.2f}", -1.2));

  EXPECT_EQ("a, b, c", format("{0}, {1}, {2}", 'a', 'b', 'c'));
  EXPECT_EQ("a, b, c", format("{}, {}, {}", 'a', 'b', 'c'));
  EXPECT_EQ("c, b, a", format("{2}, {1}, {0}", 'a', 'b', 'c'));
  EXPECT_EQ("abracadabra", format("{0}{1}{0}", "abra", "cad"));

  EXPECT_EQ("left aligned                  ", format("{:<30}", "left aligned"));
  EXPECT_EQ("                 right aligned",
            format("{:>30}", "right aligned"));
  EXPECT_EQ("           centered           ", format("{:^30}", "centered"));
  EXPECT_EQ("***********centered***********", format("{:*^30}", "centered"));

  EXPECT_EQ("+3.140000; -3.140000", format("{:+f}; {:+f}", 3.14, -3.14));
  EXPECT_EQ(" 3.140000; -3.140000", format("{: f}; {: f}", 3.14, -3.14));
  EXPECT_EQ("3.140000; -3.140000", format("{:-f}; {:-f}", 3.14, -3.14));

  EXPECT_EQ("int: 42;  hex: 2a;  oct: 52",
            format("int: {0:d};  hex: {0:x};  oct: {0:o}", 42));
  EXPECT_EQ("int: 42;  hex: 0x2a;  oct: 052",
            format("int: {0:d};  hex: {0:#x};  oct: {0:#o}", 42));

  EXPECT_EQ("The answer is 42", format("The answer is {}", 42));
  EXPECT_THROW_MSG(format("The answer is {:d}", "forty-two"), format_error,
                   "invalid type specifier");

  EXPECT_EQ(L"Cyrillic letter \x42e", format(L"Cyrillic letter {}", L'\x42e'));

  EXPECT_WRITE(
      stdout, fmt::print("{}", std::numeric_limits<double>::infinity()), "inf");
}

TEST(FormatIntTest, Data) {
  fmt::format_int format_int(42);
  EXPECT_EQ("42", std::string(format_int.data(), format_int.size()));
}

TEST(FormatIntTest, FormatInt) {
  EXPECT_EQ("42", fmt::format_int(42).str());
  EXPECT_EQ(2u, fmt::format_int(42).size());
  EXPECT_EQ("-42", fmt::format_int(-42).str());
  EXPECT_EQ(3u, fmt::format_int(-42).size());
  EXPECT_EQ("42", fmt::format_int(42ul).str());
  EXPECT_EQ("-42", fmt::format_int(-42l).str());
  EXPECT_EQ("42", fmt::format_int(42ull).str());
  EXPECT_EQ("-42", fmt::format_int(-42ll).str());
  std::ostringstream os;
  os << max_value<int64_t>();
  EXPECT_EQ(os.str(), fmt::format_int(max_value<int64_t>()).str());
}

TEST(FormatTest, Print) {
#if FMT_USE_FCNTL
  EXPECT_WRITE(stdout, fmt::print("Don't {}!", "panic"), "Don't panic!");
  EXPECT_WRITE(stderr, fmt::print(stderr, "Don't {}!", "panic"),
               "Don't panic!");
#endif
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}

TEST(FormatTest, Variadic) {
  EXPECT_EQ("abc1", format("{}c{}", "ab", 1));
  EXPECT_EQ(L"abc1", format(L"{}c{}", L"ab", 1));
}

TEST(FormatTest, Dynamic) {
  typedef fmt::format_context ctx;
  std::vector<fmt::basic_format_arg<ctx>> args;
  args.emplace_back(fmt::detail::make_arg<ctx>(42));
  args.emplace_back(fmt::detail::make_arg<ctx>("abc1"));
  args.emplace_back(fmt::detail::make_arg<ctx>(1.5f));

  std::string result = fmt::vformat(
      "{} and {} and {}",
      fmt::basic_format_args<ctx>(args.data(), static_cast<int>(args.size())));

  EXPECT_EQ("42 and abc1 and 1.5", result);
}

TEST(FormatTest, Bytes) {
  auto s = fmt::format("{:10}", fmt::bytes("ёжик"));
  EXPECT_EQ("ёжик  ", s);
  EXPECT_EQ(10, s.size());
}

TEST(FormatTest, JoinArg) {
  using fmt::join;
  int v1[3] = {1, 2, 3};
  std::vector<float> v2;
  v2.push_back(1.2f);
  v2.push_back(3.4f);
  void* v3[2] = {&v1[0], &v1[1]};

  EXPECT_EQ("(1, 2, 3)", format("({})", join(v1, v1 + 3, ", ")));
  EXPECT_EQ("(1)", format("({})", join(v1, v1 + 1, ", ")));
  EXPECT_EQ("()", format("({})", join(v1, v1, ", ")));
  EXPECT_EQ("(001, 002, 003)", format("({:03})", join(v1, v1 + 3, ", ")));
  EXPECT_EQ("(+01.20, +03.40)",
            format("({:+06.2f})", join(v2.begin(), v2.end(), ", ")));

  EXPECT_EQ(L"(1, 2, 3)", format(L"({})", join(v1, v1 + 3, L", ")));
  EXPECT_EQ("1, 2, 3", format("{0:{1}}", join(v1, v1 + 3, ", "), 1));

  EXPECT_EQ(format("{}, {}", v3[0], v3[1]),
            format("{}", join(v3, v3 + 2, ", ")));

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 405
  EXPECT_EQ("(1, 2, 3)", format("({})", join(v1, ", ")));
  EXPECT_EQ("(+01.20, +03.40)", format("({:+06.2f})", join(v2, ", ")));
#endif
}

template <typename T> std::string str(const T& value) {
  return fmt::format("{}", value);
}

TEST(StrTest, Convert) {
  EXPECT_EQ("42", str(42));
  std::string s = str(Date(2012, 12, 9));
  EXPECT_EQ("2012-12-9", s);
}

std::string vformat_message(int id, const char* format, fmt::format_args args) {
  fmt::memory_buffer buffer;
  format_to(buffer, "[{}] ", id);
  vformat_to(buffer, format, args);
  return to_string(buffer);
}

template <typename... Args>
std::string format_message(int id, const char* format, const Args&... args) {
  auto va = fmt::make_format_args(args...);
  return vformat_message(id, format, va);
}

TEST(FormatTest, FormatMessageExample) {
  EXPECT_EQ("[42] something happened",
            format_message(42, "{} happened", "something"));
}

template <typename... Args>
void print_error(const char* file, int line, const char* format,
                 const Args&... args) {
  fmt::print("{}: {}: ", file, line);
  fmt::print(format, args...);
}

TEST(FormatTest, UnpackedArgs) {
  EXPECT_EQ("0123456789abcdefg",
            fmt::format("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5,
                        6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e', 'f', 'g'));
}

struct string_like {};
fmt::string_view to_string_view(string_like) { return "foo"; }

constexpr char with_null[3] = {'{', '}', '\0'};
constexpr char no_null[2] = {'{', '}'};

TEST(FormatTest, CompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(L"{}"), 42));
  EXPECT_EQ("foo", fmt::format(FMT_STRING("{}"), string_like()));
  (void)with_null;
  (void)no_null;
#if __cplusplus >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(with_null), 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING(no_null), 42));
#endif
#if defined(FMT_USE_STRING_VIEW) && __cplusplus >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(std::string_view("{}")), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(std::wstring_view(L"{}")), 42));
#endif
}

TEST(FormatTest, CustomFormatCompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), Answer()));
  Answer answer;
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), answer));
  char buf[10] = {};
  fmt::format_to(buf, FMT_STRING("{}"), answer);
  const Answer const_answer = Answer();
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), const_answer));
}

#if FMT_USE_USER_DEFINED_LITERALS
// Passing user-defined literals directly to EXPECT_EQ causes problems
// with macro argument stringification (#) on some versions of GCC.
// Workaround: Assing the UDL result to a variable before the macro.

using namespace fmt::literals;

TEST(LiteralsTest, Format) {
  auto udl_format = "{}c{}"_format("ab", 1);
  EXPECT_EQ(format("{}c{}", "ab", 1), udl_format);
  auto udl_format_w = L"{}c{}"_format(L"ab", 1);
  EXPECT_EQ(format(L"{}c{}", L"ab", 1), udl_format_w);
}

TEST(LiteralsTest, NamedArg) {
  auto udl_a = format("{first}{second}{first}{third}", "first"_a = "abra",
                      "second"_a = "cad", "third"_a = 99);
  EXPECT_EQ(format("{first}{second}{first}{third}", fmt::arg("first", "abra"),
                   fmt::arg("second", "cad"), fmt::arg("third", 99)),
            udl_a);
  auto udl_a_w = format(L"{first}{second}{first}{third}", L"first"_a = L"abra",
                        L"second"_a = L"cad", L"third"_a = 99);
  EXPECT_EQ(
      format(L"{first}{second}{first}{third}", fmt::arg(L"first", L"abra"),
             fmt::arg(L"second", L"cad"), fmt::arg(L"third", 99)),
      udl_a_w);
}

TEST(FormatTest, UdlTemplate) {
  EXPECT_EQ("foo", "foo"_format());
  EXPECT_EQ("        42", "{0:10}"_format(42));
}

TEST(FormatTest, UdlPassUserDefinedObjectAsLvalue) {
  Date date(2015, 10, 21);
  EXPECT_EQ("2015-10-21", "{}"_format(date));
}
#endif  // FMT_USE_USER_DEFINED_LITERALS

enum TestEnum { A };

TEST(FormatTest, Enum) { EXPECT_EQ("0", fmt::format("{}", A)); }

TEST(FormatTest, FormatterNotSpecialized) {
  static_assert(
      !fmt::has_formatter<fmt::formatter<TestEnum>, fmt::format_context>::value,
      "");
}

#if FMT_HAS_FEATURE(cxx_strong_enums)
enum big_enum : unsigned long long { big_enum_value = 5000000000ULL };

TEST(FormatTest, StrongEnum) {
  EXPECT_EQ("5000000000", fmt::format("{}", big_enum_value));
}
#endif

using buffer_iterator = fmt::format_context::iterator;

class mock_arg_formatter
    : public fmt::detail::arg_formatter_base<buffer_iterator, char> {
 private:
#if FMT_USE_INT128
  MOCK_METHOD1(call, void(__int128_t value));
#else
  MOCK_METHOD1(call, void(long long value));
#endif

 public:
  using base = fmt::detail::arg_formatter_base<buffer_iterator, char>;

  mock_arg_formatter(fmt::format_context& ctx, fmt::format_parse_context*,
                     fmt::format_specs* s = nullptr, const char* = nullptr)
      : base(ctx.out(), s, ctx.locale()) {
    EXPECT_CALL(*this, call(42));
  }

  template <typename T>
  typename std::enable_if<fmt::detail::is_integral<T>::value, iterator>::type
  operator()(T value) {
    call(value);
    return base::operator()(value);
  }

  template <typename T>
  typename std::enable_if<!fmt::detail::is_integral<T>::value, iterator>::type
  operator()(T value) {
    return base::operator()(value);
  }

  iterator operator()(fmt::basic_format_arg<fmt::format_context>::handle) {
    return base::operator()(fmt::monostate());
  }
};

static void custom_vformat(fmt::string_view format_str, fmt::format_args args) {
  fmt::memory_buffer buffer;
  fmt::detail::buffer<char>& base = buffer;
  fmt::vformat_to<mock_arg_formatter>(std::back_inserter(base), format_str,
                                      args);
}

template <typename... Args>
void custom_format(const char* format_str, const Args&... args) {
  auto va = fmt::make_format_args(args...);
  return custom_vformat(format_str, va);
}

TEST(FormatTest, CustomArgFormatter) { custom_format("{}", 42); }

TEST(FormatTest, NonNullTerminatedFormatString) {
  EXPECT_EQ("42", format(string_view("{}foo", 2), 42));
}

struct variant {
  enum { INT, STRING } type;
  explicit variant(int) : type(INT) {}
  explicit variant(const char*) : type(STRING) {}
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<variant> : dynamic_formatter<> {
  auto format(variant value, format_context& ctx) -> decltype(ctx.out()) {
    if (value.type == variant::INT) return dynamic_formatter<>::format(42, ctx);
    return dynamic_formatter<>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(FormatTest, DynamicFormatter) {
  auto num = variant(42);
  auto str = variant("foo");
  EXPECT_EQ("42", format("{:d}", num));
  EXPECT_EQ("foo", format("{:s}", str));
  EXPECT_EQ(" 42 foo ", format("{:{}} {:{}}", num, 3, str, 4));
  EXPECT_THROW_MSG(format("{0:{}}", num), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{:{0}}", num), format_error,
                   "cannot switch from automatic to manual argument indexing");
#if FMT_DEPRECATED_NUMERIC_ALIGN
  EXPECT_THROW_MSG(format("{:=}", str), format_error,
                   "format specifier requires numeric argument");
#endif
  EXPECT_THROW_MSG(format("{:+}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:-}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{: }", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:#}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:0}", str), format_error,
                   "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:.2}", num), format_error,
                   "precision not allowed for this argument type");
}

namespace adl_test {
namespace fmt {
namespace detail {
struct foo {};
template <typename, typename OutputIt> void write(OutputIt, foo) = delete;
}  // namespace detail
}  // namespace fmt
}  // namespace adl_test

FMT_BEGIN_NAMESPACE
template <>
struct formatter<adl_test::fmt::detail::foo> : formatter<std::string> {
  template <typename FormatContext>
  auto format(adl_test::fmt::detail::foo, FormatContext& ctx)
      -> decltype(ctx.out()) {
    return formatter<std::string>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(FormatTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
  EXPECT_EQ("0x1234", fmt::to_string(reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("foo", fmt::to_string(adl_test::fmt::detail::foo()));
}

TEST(FormatTest, ToWString) { EXPECT_EQ(L"42", fmt::to_wstring(42)); }

TEST(FormatTest, OutputIterators) {
  std::list<char> out;
  fmt::format_to(std::back_inserter(out), "{}", 42);
  EXPECT_EQ("42", std::string(out.begin(), out.end()));
  std::stringstream s;
  fmt::format_to(std::ostream_iterator<char>(s), "{}", 42);
  EXPECT_EQ("42", s.str());
}

TEST(FormatTest, FormattedSize) {
  EXPECT_EQ(2u, fmt::formatted_size("{}", 42));
}

TEST(FormatTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, "{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("123x", fmt::string_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, "{:s}", "foobar");
  EXPECT_EQ(6u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("foox", fmt::string_view(buffer, 4));
  buffer[0] = 'x';
  buffer[1] = 'x';
  buffer[2] = 'x';
  result = fmt::format_to_n(buffer, 3, "{}", 'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ("Axxx", fmt::string_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, "{}{} ", 'B', 'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("BC x", fmt::string_view(buffer, 4));
}

TEST(FormatTest, WideFormatToN) {
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

struct test_output_iterator {
  char* data;

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  test_output_iterator& operator++() {
    ++data;
    return *this;
  }
  test_output_iterator operator++(int) {
    auto tmp = *this;
    ++data;
    return tmp;
  }
  char& operator*() { return *data; }
};

TEST(FormatTest, FormatToNOutputIterator) {
  char buf[10] = {};
  fmt::format_to_n(test_output_iterator{buf}, 10, "{}", 42);
  EXPECT_STREQ(buf, "42");
}

#if FMT_USE_CONSTEXPR
struct test_arg_id_handler {
  enum result { NONE, EMPTY, INDEX, NAME, ERROR };
  result res = NONE;
  int index = 0;
  string_view name;

  FMT_CONSTEXPR void operator()() { res = EMPTY; }

  FMT_CONSTEXPR void operator()(int i) {
    res = INDEX;
    index = i;
  }

  FMT_CONSTEXPR void operator()(string_view n) {
    res = NAME;
    name = n;
  }

  FMT_CONSTEXPR void on_error(const char*) { res = ERROR; }
};

template <size_t N>
FMT_CONSTEXPR test_arg_id_handler parse_arg_id(const char (&s)[N]) {
  test_arg_id_handler h;
  fmt::detail::parse_arg_id(s, s + N, h);
  return h;
}

TEST(FormatTest, ConstexprParseArgID) {
  static_assert(parse_arg_id(":").res == test_arg_id_handler::EMPTY, "");
  static_assert(parse_arg_id("}").res == test_arg_id_handler::EMPTY, "");
  static_assert(parse_arg_id("42:").res == test_arg_id_handler::INDEX, "");
  static_assert(parse_arg_id("42:").index == 42, "");
  static_assert(parse_arg_id("foo:").res == test_arg_id_handler::NAME, "");
  static_assert(parse_arg_id("foo:").name.size() == 3, "");
  static_assert(parse_arg_id("!").res == test_arg_id_handler::ERROR, "");
}

struct test_format_specs_handler {
  enum Result { NONE, PLUS, MINUS, SPACE, HASH, ZERO, ERROR };
  Result res = NONE;

  fmt::align_t align = fmt::align::none;
  char fill = 0;
  int width = 0;
  fmt::detail::arg_ref<char> width_ref;
  int precision = 0;
  fmt::detail::arg_ref<char> precision_ref;
  char type = 0;

  // Workaround for MSVC2017 bug that results in "expression did not evaluate
  // to a constant" with compiler-generated copy ctor.
  FMT_CONSTEXPR test_format_specs_handler() {}
  FMT_CONSTEXPR test_format_specs_handler(
      const test_format_specs_handler& other)
      : res(other.res),
        align(other.align),
        fill(other.fill),
        width(other.width),
        width_ref(other.width_ref),
        precision(other.precision),
        precision_ref(other.precision_ref),
        type(other.type) {}

  FMT_CONSTEXPR void on_align(fmt::align_t a) { align = a; }
  FMT_CONSTEXPR void on_fill(fmt::string_view f) { fill = f[0]; }
  FMT_CONSTEXPR void on_plus() { res = PLUS; }
  FMT_CONSTEXPR void on_minus() { res = MINUS; }
  FMT_CONSTEXPR void on_space() { res = SPACE; }
  FMT_CONSTEXPR void on_hash() { res = HASH; }
  FMT_CONSTEXPR void on_zero() { res = ZERO; }

  FMT_CONSTEXPR void on_width(int w) { width = w; }
  FMT_CONSTEXPR void on_dynamic_width(fmt::detail::auto_id) {}
  FMT_CONSTEXPR void on_dynamic_width(int index) { width_ref = index; }
  FMT_CONSTEXPR void on_dynamic_width(string_view) {}

  FMT_CONSTEXPR void on_precision(int p) { precision = p; }
  FMT_CONSTEXPR void on_dynamic_precision(fmt::detail::auto_id) {}
  FMT_CONSTEXPR void on_dynamic_precision(int index) { precision_ref = index; }
  FMT_CONSTEXPR void on_dynamic_precision(string_view) {}

  FMT_CONSTEXPR void end_precision() {}
  FMT_CONSTEXPR void on_type(char t) { type = t; }
  FMT_CONSTEXPR void on_error(const char*) { res = ERROR; }
};

template <size_t N>
FMT_CONSTEXPR test_format_specs_handler parse_test_specs(const char (&s)[N]) {
  test_format_specs_handler h;
  fmt::detail::parse_format_specs(s, s + N, h);
  return h;
}

TEST(FormatTest, ConstexprParseFormatSpecs) {
  typedef test_format_specs_handler handler;
  static_assert(parse_test_specs("<").align == fmt::align::left, "");
  static_assert(parse_test_specs("*^").fill == '*', "");
  static_assert(parse_test_specs("+").res == handler::PLUS, "");
  static_assert(parse_test_specs("-").res == handler::MINUS, "");
  static_assert(parse_test_specs(" ").res == handler::SPACE, "");
  static_assert(parse_test_specs("#").res == handler::HASH, "");
  static_assert(parse_test_specs("0").res == handler::ZERO, "");
  static_assert(parse_test_specs("42").width == 42, "");
  static_assert(parse_test_specs("{42}").width_ref.val.index == 42, "");
  static_assert(parse_test_specs(".42").precision == 42, "");
  static_assert(parse_test_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(parse_test_specs("d").type == 'd', "");
  static_assert(parse_test_specs("{<").res == handler::ERROR, "");
}

struct test_parse_context {
  typedef char char_type;

  FMT_CONSTEXPR int next_arg_id() { return 11; }
  template <typename Id> FMT_CONSTEXPR void check_arg_id(Id) {}

  FMT_CONSTEXPR const char* begin() { return nullptr; }
  FMT_CONSTEXPR const char* end() { return nullptr; }

  void on_error(const char*) {}
};

struct test_context {
  using char_type = char;
  using format_arg = fmt::basic_format_arg<test_context>;
  using parse_context_type = fmt::format_parse_context;

  template <typename T> struct formatter_type {
    typedef fmt::formatter<T, char_type> type;
  };

  template <typename Id>
  FMT_CONSTEXPR fmt::basic_format_arg<test_context> arg(Id id) {
    return fmt::detail::make_arg<test_context>(id);
  }

  void on_error(const char*) {}

  FMT_CONSTEXPR test_context error_handler() { return *this; }
};

template <size_t N>
FMT_CONSTEXPR fmt::format_specs parse_specs(const char (&s)[N]) {
  auto specs = fmt::format_specs();
  auto parse_ctx = test_parse_context();
  auto ctx = test_context();
  fmt::detail::specs_handler<test_parse_context, test_context> h(
      specs, parse_ctx, ctx);
  parse_format_specs(s, s + N, h);
  return specs;
}

TEST(FormatTest, ConstexprSpecsHandler) {
  static_assert(parse_specs("<").align == fmt::align::left, "");
  static_assert(parse_specs("*^").fill[0] == '*', "");
  static_assert(parse_specs("+").sign == fmt::sign::plus, "");
  static_assert(parse_specs("-").sign == fmt::sign::minus, "");
  static_assert(parse_specs(" ").sign == fmt::sign::space, "");
  static_assert(parse_specs("#").alt, "");
  static_assert(parse_specs("0").align == fmt::align::numeric, "");
  static_assert(parse_specs("42").width == 42, "");
  static_assert(parse_specs("{}").width == 11, "");
  static_assert(parse_specs("{22}").width == 22, "");
  static_assert(parse_specs(".42").precision == 42, "");
  static_assert(parse_specs(".{}").precision == 11, "");
  static_assert(parse_specs(".{22}").precision == 22, "");
  static_assert(parse_specs("d").type == 'd', "");
}

template <size_t N>
FMT_CONSTEXPR fmt::detail::dynamic_format_specs<char> parse_dynamic_specs(
    const char (&s)[N]) {
  fmt::detail::dynamic_format_specs<char> specs;
  test_parse_context ctx{};
  fmt::detail::dynamic_specs_handler<test_parse_context> h(specs, ctx);
  parse_format_specs(s, s + N, h);
  return specs;
}

TEST(FormatTest, ConstexprDynamicSpecsHandler) {
  static_assert(parse_dynamic_specs("<").align == fmt::align::left, "");
  static_assert(parse_dynamic_specs("*^").fill[0] == '*', "");
  static_assert(parse_dynamic_specs("+").sign == fmt::sign::plus, "");
  static_assert(parse_dynamic_specs("-").sign == fmt::sign::minus, "");
  static_assert(parse_dynamic_specs(" ").sign == fmt::sign::space, "");
  static_assert(parse_dynamic_specs("#").alt, "");
  static_assert(parse_dynamic_specs("0").align == fmt::align::numeric, "");
  static_assert(parse_dynamic_specs("42").width == 42, "");
  static_assert(parse_dynamic_specs("{}").width_ref.val.index == 11, "");
  static_assert(parse_dynamic_specs("{42}").width_ref.val.index == 42, "");
  static_assert(parse_dynamic_specs(".42").precision == 42, "");
  static_assert(parse_dynamic_specs(".{}").precision_ref.val.index == 11, "");
  static_assert(parse_dynamic_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(parse_dynamic_specs("d").type == 'd', "");
}

template <size_t N>
FMT_CONSTEXPR test_format_specs_handler check_specs(const char (&s)[N]) {
  fmt::detail::specs_checker<test_format_specs_handler> checker(
      test_format_specs_handler(), fmt::detail::type::double_type);
  parse_format_specs(s, s + N, checker);
  return checker;
}

TEST(FormatTest, ConstexprSpecsChecker) {
  typedef test_format_specs_handler handler;
  static_assert(check_specs("<").align == fmt::align::left, "");
  static_assert(check_specs("*^").fill == '*', "");
  static_assert(check_specs("+").res == handler::PLUS, "");
  static_assert(check_specs("-").res == handler::MINUS, "");
  static_assert(check_specs(" ").res == handler::SPACE, "");
  static_assert(check_specs("#").res == handler::HASH, "");
  static_assert(check_specs("0").res == handler::ZERO, "");
  static_assert(check_specs("42").width == 42, "");
  static_assert(check_specs("{42}").width_ref.val.index == 42, "");
  static_assert(check_specs(".42").precision == 42, "");
  static_assert(check_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(check_specs("d").type == 'd', "");
  static_assert(check_specs("{<").res == handler::ERROR, "");
}

struct test_format_string_handler {
  FMT_CONSTEXPR void on_text(const char*, const char*) {}

  FMT_CONSTEXPR int on_arg_id() { return 0; }

  template <typename T> FMT_CONSTEXPR int on_arg_id(T) { return 0; }

  FMT_CONSTEXPR void on_replacement_field(int, const char*) {}

  FMT_CONSTEXPR const char* on_format_specs(int, const char* begin,
                                            const char*) {
    return begin;
  }

  FMT_CONSTEXPR void on_error(const char*) { error = true; }

  bool error = false;
};

template <size_t N> FMT_CONSTEXPR bool parse_string(const char (&s)[N]) {
  test_format_string_handler h;
  fmt::detail::parse_format_string<true>(fmt::string_view(s, N - 1), h);
  return !h.error;
}

TEST(FormatTest, ConstexprParseFormatString) {
  static_assert(parse_string("foo"), "");
  static_assert(!parse_string("}"), "");
  static_assert(parse_string("{}"), "");
  static_assert(parse_string("{42}"), "");
  static_assert(parse_string("{foo}"), "");
  static_assert(parse_string("{:}"), "");
}

struct test_error_handler {
  const char*& error;

  FMT_CONSTEXPR test_error_handler(const char*& err) : error(err) {}

  FMT_CONSTEXPR test_error_handler(const test_error_handler& other)
      : error(other.error) {}

  FMT_CONSTEXPR void on_error(const char* message) {
    if (!error) error = message;
  }
};

FMT_CONSTEXPR size_t len(const char* s) {
  size_t len = 0;
  while (*s++) ++len;
  return len;
}

FMT_CONSTEXPR bool equal(const char* s1, const char* s2) {
  if (!s1 || !s2) return s1 == s2;
  while (*s1 && *s1 == *s2) {
    ++s1;
    ++s2;
  }
  return *s1 == *s2;
}

template <typename... Args>
FMT_CONSTEXPR bool test_error(const char* fmt, const char* expected_error) {
  const char* actual_error = nullptr;
  string_view s(fmt, len(fmt));
  fmt::detail::format_string_checker<char, test_error_handler, Args...> checker(
      s, test_error_handler(actual_error));
  fmt::detail::parse_format_string<true>(s, checker);
  return equal(actual_error, expected_error);
}

#  define EXPECT_ERROR_NOARGS(fmt, error) \
    static_assert(test_error(fmt, error), "")
#  define EXPECT_ERROR(fmt, error, ...) \
    static_assert(test_error<__VA_ARGS__>(fmt, error), "")

TEST(FormatTest, FormatStringErrors) {
  EXPECT_ERROR_NOARGS("foo", nullptr);
  EXPECT_ERROR_NOARGS("}", "unmatched '}' in format string");
  EXPECT_ERROR("{0:s", "unknown format specifier", Date);
#  if !FMT_MSC_VER || FMT_MSC_VER >= 1916
  // This causes an detail compiler error in MSVC2017.
  EXPECT_ERROR("{:{<}", "invalid fill character '{'", int);
  EXPECT_ERROR("{:10000000000}", "number is too big", int);
  EXPECT_ERROR("{:.10000000000}", "number is too big", int);
  EXPECT_ERROR_NOARGS("{:x}", "argument not found");
#    if FMT_DEPRECATED_NUMERIC_ALIGN
  EXPECT_ERROR("{0:=5", "unknown format specifier", int);
  EXPECT_ERROR("{:=}", "format specifier requires numeric argument",
               const char*);
#    endif
  EXPECT_ERROR("{:+}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:-}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:#}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{: }", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:0}", "format specifier requires numeric argument",
               const char*);
  EXPECT_ERROR("{:+}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:-}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{: }", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:{}}", "argument not found", int);
  EXPECT_ERROR("{:.{}}", "argument not found", double);
  EXPECT_ERROR("{:.2}", "precision not allowed for this argument type", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", bool);
  EXPECT_ERROR("{:s}", "invalid type specifier", char);
  EXPECT_ERROR("{:+}", "invalid format specifier for char", char);
  EXPECT_ERROR("{:s}", "invalid type specifier", double);
  EXPECT_ERROR("{:d}", "invalid type specifier", const char*);
  EXPECT_ERROR("{:d}", "invalid type specifier", std::string);
  EXPECT_ERROR("{:s}", "invalid type specifier", void*);
#  else
  fmt::print("warning: constexpr is broken in this version of MSVC\n");
#  endif
  EXPECT_ERROR("{foo", "compile-time checks don't support named arguments",
               int);
  EXPECT_ERROR_NOARGS("{10000000000}", "number is too big");
  EXPECT_ERROR_NOARGS("{0x}", "invalid format string");
  EXPECT_ERROR_NOARGS("{-}", "invalid format string");
  EXPECT_ERROR("{:{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:.{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.x}", "missing precision specifier", int);
  EXPECT_ERROR_NOARGS("{}", "argument not found");
  EXPECT_ERROR("{1}", "argument not found", int);
  EXPECT_ERROR("{1}{}",
               "cannot switch from manual to automatic argument indexing", int,
               int);
  EXPECT_ERROR("{}{1}",
               "cannot switch from automatic to manual argument indexing", int,
               int);
}

TEST(FormatTest, VFormatTo) {
  typedef fmt::format_context context;
  fmt::basic_format_arg<context> arg = fmt::detail::make_arg<context>(42);
  fmt::basic_format_args<context> args(&arg, 1);
  std::string s;
  fmt::vformat_to(std::back_inserter(s), "{}", args);
  EXPECT_EQ("42", s);
  s.clear();
  fmt::vformat_to(std::back_inserter(s), FMT_STRING("{}"), args);
  EXPECT_EQ("42", s);

  typedef fmt::wformat_context wcontext;
  fmt::basic_format_arg<wcontext> warg = fmt::detail::make_arg<wcontext>(42);
  fmt::basic_format_args<wcontext> wargs(&warg, 1);
  std::wstring w;
  fmt::vformat_to(std::back_inserter(w), L"{}", wargs);
  EXPECT_EQ(L"42", w);
  w.clear();
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

template <typename T> static std::string FmtToString(const T& t) {
  return fmt::format(FMT_STRING("{}"), t);
}

TEST(FormatTest, FmtStringInTemplate) {
  EXPECT_EQ(FmtToString(1), "1");
  EXPECT_EQ(FmtToString(0), "0");
}

#endif  // FMT_USE_CONSTEXPR

TEST(FormatTest, EmphasisNonHeaderOnly) {
  // Ensure this compiles even if FMT_HEADER_ONLY is not defined.
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
}

TEST(FormatTest, CharTraitsIsNotAmbiguous) {
  // Test that we don't inject detail names into the std namespace.
  using namespace std;
  char_traits<char>::char_type c;
  (void)c;
#if __cplusplus >= 201103L
  std::string s;
  auto lval = begin(s);
  (void)lval;
#endif
}

struct mychar {
  int value;
  mychar() = default;

  template <typename T> mychar(T val) : value(static_cast<int>(val)) {}

  operator int() const { return value; }
};

FMT_BEGIN_NAMESPACE
template <> struct is_char<mychar> : std::true_type {};
FMT_END_NAMESPACE

TEST(FormatTest, FormatCustomChar) {
  const mychar format[] = {'{', '}', 0};
  auto result = fmt::format(format, mychar('x'));
  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], mychar('x'));
}

// Convert a char8_t string to std::string. Otherwise GTest will insist on
// inserting `char8_t` NTBS into a `char` stream which is disabled by P1423.
template <typename S> std::string from_u8str(const S& str) {
  return std::string(str.begin(), str.end());
}

TEST(FormatTest, FormatUTF8Precision) {
  using str_type = std::basic_string<fmt::detail::char8_type>;
  str_type format(reinterpret_cast<const fmt::detail::char8_type*>(u8"{:.4}"));
  str_type str(reinterpret_cast<const fmt::detail::char8_type*>(
      u8"caf\u00e9s"));  // cafés
  auto result = fmt::format(format, str);
  EXPECT_EQ(fmt::detail::count_code_points(result), 4);
  EXPECT_EQ(result.size(), 5);
  EXPECT_EQ(from_u8str(result), from_u8str(str.substr(0, 5)));
}
