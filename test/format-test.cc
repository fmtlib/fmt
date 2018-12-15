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
#include <string>
#include <stdint.h>

// Check if fmt/format.h compiles with windows.h included before it.
#ifdef _WIN32
# include <windows.h>
#endif

#include "fmt/format.h"
#include "gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

#undef ERROR
#undef min
#undef max

using std::size_t;

using fmt::basic_memory_buffer;
using fmt::basic_writer;
using fmt::format;
using fmt::format_error;
using fmt::string_view;
using fmt::memory_buffer;
using fmt::wmemory_buffer;

using testing::Return;
using testing::StrictMock;

namespace {

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 408
template <typename Char, typename T>
bool check_enabled_formatter() {
  static_assert(
        std::is_default_constructible<fmt::formatter<T, Char>>::value, "");
  return true;
}

template <typename Char, typename... T>
void check_enabled_formatters() {
  auto dummy = {check_enabled_formatter<Char, T>()...};
  (void)dummy;
}

TEST(FormatterTest, TestFormattersEnabled) {
  check_enabled_formatters<char,
      bool, char, signed char, unsigned char, short, unsigned short,
      int, unsigned, long, unsigned long, long long, unsigned long long,
      float, double, long double, void*, const void*,
      char*, const char*, std::string>();
  check_enabled_formatters<wchar_t,
      bool, wchar_t, signed char, unsigned char, short, unsigned short,
      int, unsigned, long, unsigned long, long long, unsigned long long,
      float, double, long double, void*, const void*,
      wchar_t*, const wchar_t*, std::wstring>();
#if FMT_USE_NULLPTR
  check_enabled_formatters<char, std::nullptr_t>();
  check_enabled_formatters<wchar_t, std::nullptr_t>();
#endif
}
#endif

// Format value using the standard library.
template <typename Char, typename T>
void std_format(const T &value, std::basic_string<Char> &result) {
  std::basic_ostringstream<Char> os;
  os << value;
  result = os.str();
}

#ifdef __MINGW32__
// Workaround a bug in formatting long double in MinGW.
void std_format(long double value, std::string &result) {
  char buffer[100];
  safe_sprintf(buffer, "%Lg", value);
  result = buffer;
}
void std_format(long double value, std::wstring &result) {
  wchar_t buffer[100];
  swprintf(buffer, L"%Lg", value);
  result = buffer;
}
#endif

// Checks if writing value to BasicWriter<Char> produces the same result
// as writing it to std::basic_ostringstream<Char>.
template <typename Char, typename T>
::testing::AssertionResult check_write(const T &value, const char *type) {
  fmt::basic_memory_buffer<Char> buffer;
  typedef fmt::back_insert_range<fmt::internal::basic_buffer<Char>> range;
  fmt::basic_writer<range> writer(buffer);
  writer.write(value);
  std::basic_string<Char> actual = to_string(buffer);
  std::basic_string<Char> expected;
  std_format(value, expected);
  if (expected == actual)
    return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
      << "Value of: (Writer<" << type << ">() << value).str()\n"
      << "  Actual: " << actual << "\n"
      << "Expected: " << expected << "\n";
}

struct AnyWriteChecker {
  template <typename T>
  ::testing::AssertionResult operator()(const char *, const T &value) const {
    ::testing::AssertionResult result = check_write<char>(value, "char");
    return result ? check_write<wchar_t>(value, "wchar_t") : result;
  }
};

template <typename Char>
struct WriteChecker {
  template <typename T>
  ::testing::AssertionResult operator()(const char *, const T &value) const {
    return check_write<Char>(value, "char");
  }
};

// Checks if writing value to BasicWriter produces the same result
// as writing it to std::ostringstream both for char and wchar_t.
#define CHECK_WRITE(value) EXPECT_PRED_FORMAT1(AnyWriteChecker(), value)

#define CHECK_WRITE_CHAR(value) \
  EXPECT_PRED_FORMAT1(WriteChecker<char>(), value)
#define CHECK_WRITE_WCHAR(value) \
  EXPECT_PRED_FORMAT1(WriteChecker<wchar_t>(), value)
}  // namespace

// Tests fmt::internal::count_digits for integer type Int.
template <typename Int>
void test_count_digits() {
  for (Int i = 0; i < 10; ++i)
    EXPECT_EQ(1u, fmt::internal::count_digits(i));
  for (Int i = 1, n = 1,
      end = std::numeric_limits<Int>::max() / 10; n <= end; ++i) {
    n *= 10;
    EXPECT_EQ(i, fmt::internal::count_digits(n - 1));
    EXPECT_EQ(i + 1, fmt::internal::count_digits(n));
  }
}

TEST(UtilTest, CountDigits) {
  test_count_digits<uint32_t>();
  test_count_digits<uint64_t>();
}

struct uint32_pair {
  uint32_t u[2];
};

TEST(UtilTest, BitCast) {
  auto s = fmt::internal::bit_cast<uint32_pair>(uint64_t{42});
  EXPECT_EQ(fmt::internal::bit_cast<uint64_t>(s), 42ull);
  s = fmt::internal::bit_cast<uint32_pair>(uint64_t(~0ull));
  EXPECT_EQ(fmt::internal::bit_cast<uint64_t>(s), ~0ull);
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
  if (std::numeric_limits<int>::max() !=
      static_cast<int>(static_cast<unsigned>(1) << 31)) {
    fmt::print("Skipping parse_nonnegative_int test\n");
    return;
  }
  fmt::string_view s = "10000000000";
  auto begin = s.begin(), end = s.end();
  EXPECT_THROW_MSG(
        parse_nonnegative_int(begin, end, fmt::internal::error_handler()),
        fmt::format_error, "number is too big");
  s = "2147483649";
  begin = s.begin();
  end = s.end();
  EXPECT_THROW_MSG(
        parse_nonnegative_int(begin, end, fmt::internal::error_handler()),
        fmt::format_error, "number is too big");
}

TEST(IteratorTest, CountingIterator) {
  fmt::internal::counting_iterator<char> it;
  auto prev = it++;
  EXPECT_EQ(prev.count(), 0);
  EXPECT_EQ(it.count(), 1);
}

TEST(IteratorTest, TruncatingIterator) {
  char *p = FMT_NULL;
  fmt::internal::truncating_iterator<char*> it(p, 3);
  auto prev = it++;
  EXPECT_EQ(prev.base(), p);
  EXPECT_EQ(it.base(), p + 1);
}

TEST(IteratorTest, TruncatingBackInserter) {
  std::string buffer;
  auto bi = std::back_inserter(buffer);
  fmt::internal::truncating_iterator<decltype(bi)> it(bi, 2);
  *it++ = '4';
  *it++ = '2';
  *it++ = '1';
  EXPECT_EQ(buffer.size(), 2);
  EXPECT_EQ(buffer, "42");
}

TEST(IteratorTest, IsOutputIterator) {
  EXPECT_TRUE(fmt::internal::is_output_iterator<char*>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<const char*>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<std::string>::value);
  EXPECT_TRUE(fmt::internal::is_output_iterator<
              std::back_insert_iterator<std::string>>::value);
  EXPECT_TRUE(fmt::internal::is_output_iterator<
              std::string::iterator>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<
               std::string::const_iterator>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<std::list<char>>::value);
  EXPECT_TRUE(fmt::internal::is_output_iterator<
              std::list<char>::iterator>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<
               std::list<char>::const_iterator>::value);
  EXPECT_FALSE(fmt::internal::is_output_iterator<uint32_pair>::value);
}

TEST(MemoryBufferTest, Ctor) {
  basic_memory_buffer<char, 123> buffer;
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
}

static void check_forwarding(
    mock_allocator<int> &alloc, allocator_ref<mock_allocator<int>> &ref) {
  int mem;
  // Check if value_type is properly defined.
  allocator_ref< mock_allocator<int> >::value_type *ptr = &mem;
  // Check forwarding.
  EXPECT_CALL(alloc, allocate(42)).WillOnce(testing::Return(ptr));
  ref.allocate(42);
  EXPECT_CALL(alloc, deallocate(ptr, 42));
  ref.deallocate(ptr, 42);
}

TEST(AllocatorTest, allocator_ref) {
  StrictMock< mock_allocator<int> > alloc;
  typedef allocator_ref< mock_allocator<int> > test_allocator_ref;
  test_allocator_ref ref(&alloc);
  // Check if allocator_ref forwards to the underlying allocator.
  check_forwarding(alloc, ref);
  test_allocator_ref ref2(ref);
  check_forwarding(alloc, ref2);
  test_allocator_ref ref3;
  EXPECT_EQ(FMT_NULL, ref3.get());
  ref3 = ref;
  check_forwarding(alloc, ref3);
}

typedef allocator_ref< std::allocator<char> > TestAllocator;

static void check_move_buffer(const char *str,
                       basic_memory_buffer<char, 5, TestAllocator> &buffer) {
  std::allocator<char> *alloc = buffer.get_allocator().get();
  basic_memory_buffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
  // Move should transfer allocator.
  EXPECT_EQ(FMT_NULL, buffer.get_allocator().get());
  EXPECT_EQ(alloc, buffer2.get_allocator().get());
}

TEST(MemoryBufferTest, MoveCtor) {
  std::allocator<char> alloc;
  basic_memory_buffer<char, 5, TestAllocator> buffer((TestAllocator(&alloc)));
  const char test[] = "test";
  buffer.append(test, test + 4);
  check_move_buffer("test", buffer);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  buffer.push_back('a');
  check_move_buffer("testa", buffer);
  const char *inline_buffer_ptr = &buffer[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  buffer.push_back('b');
  basic_memory_buffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testab", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 5u);
}

static void check_move_assign_buffer(
    const char *str, basic_memory_buffer<char, 5> &buffer) {
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
  const char *inline_buffer_ptr = &buffer[0];
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
  typedef allocator_ref< mock_allocator<int> > Allocator;
  typedef basic_memory_buffer<int, 10, Allocator> Base;
  mock_allocator<int> alloc;
  struct TestMemoryBuffer : Base {
    TestMemoryBuffer(Allocator alloc) : Base(alloc) {}
    void grow(std::size_t size) { Base::grow(size); }
  } buffer((Allocator(&alloc)));
  buffer.resize(7);
  using fmt::internal::to_unsigned;
  for (int i = 0; i < 7; ++i)
    buffer[to_unsigned(i)] = i * i;
  EXPECT_EQ(10u, buffer.capacity());
  int mem[20];
  mem[7] = 0xdead;
  EXPECT_CALL(alloc, allocate(20)).WillOnce(Return(mem));
  buffer.grow(20);
  EXPECT_EQ(20u, buffer.capacity());
  // Check if size elements have been copied
  for (int i = 0; i < 7; ++i)
    EXPECT_EQ(i * i, buffer[to_unsigned(i)]);
  // and no more than that.
  EXPECT_EQ(0xdead, buffer[7]);
  EXPECT_CALL(alloc, deallocate(mem, 20));
}

TEST(MemoryBufferTest, Allocator) {
  typedef allocator_ref< mock_allocator<char> > TestAllocator;
  basic_memory_buffer<char, 10, TestAllocator> buffer;
  EXPECT_EQ(FMT_NULL, buffer.get_allocator().get());
  StrictMock< mock_allocator<char> > alloc;
  char mem;
  {
    basic_memory_buffer<char, 10, TestAllocator> buffer2((TestAllocator(&alloc)));
    EXPECT_EQ(&alloc, buffer2.get_allocator().get());
    std::size_t size = 2 * fmt::inline_buffer_size;
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem));
    buffer2.reserve(size);
    EXPECT_CALL(alloc, deallocate(&mem, size));
  }
}

TEST(MemoryBufferTest, ExceptionInDeallocate) {
  typedef allocator_ref< mock_allocator<char> > TestAllocator;
  StrictMock< mock_allocator<char> > alloc;
  basic_memory_buffer<char, 10, TestAllocator> buffer((TestAllocator(&alloc)));
  std::size_t size = 2 * fmt::inline_buffer_size;
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
    for (std::size_t i = 0; i < size; ++i)
      EXPECT_EQ('x', buffer[i]);
  }
  EXPECT_CALL(alloc, deallocate(&mem2[0], 2 * size));
}

#ifdef _WIN32
TEST(UtilTest, UTF16ToUTF8) {
  std::string s = "ёжик";
  fmt::internal::utf16_to_utf8 u(L"\x0451\x0436\x0438\x043A");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, UTF16ToUTF8EmptyString) {
  std::string s = "";
  fmt::internal::utf16_to_utf8 u(L"");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, UTF8ToUTF16) {
  std::string s = "лошадка";
  fmt::internal::utf8_to_utf16 u(s.c_str());
  EXPECT_EQ(L"\x043B\x043E\x0448\x0430\x0434\x043A\x0430", u.str());
  EXPECT_EQ(7, u.size());
}

TEST(UtilTest, UTF8ToUTF16EmptyString) {
  std::string s = "";
  fmt::internal::utf8_to_utf16 u(s.c_str());
  EXPECT_EQ(L"", u.str());
  EXPECT_EQ(s.size(), u.size());
}

template <typename Converter, typename Char>
void check_utf_conversion_error(
        const char *message,
        fmt::basic_string_view<Char> str = fmt::basic_string_view<Char>(0, 1)) {
  fmt::memory_buffer out;
  fmt::internal::format_windows_error(out, ERROR_INVALID_PARAMETER, message);
  fmt::system_error error(0, "");
  try {
    (Converter)(str);
  } catch (const fmt::system_error &e) {
    error = e;
  }
  EXPECT_EQ(ERROR_INVALID_PARAMETER, error.error_code());
  EXPECT_EQ(fmt::to_string(out), error.what());
}

TEST(UtilTest, UTF16ToUTF8Error) {
  check_utf_conversion_error<fmt::internal::utf16_to_utf8, wchar_t>(
      "cannot convert string from UTF-16 to UTF-8");
}

TEST(UtilTest, UTF8ToUTF16Error) {
  const char *message = "cannot convert string from UTF-8 to UTF-16";
  check_utf_conversion_error<fmt::internal::utf8_to_utf16, char>(message);
  check_utf_conversion_error<fmt::internal::utf8_to_utf16, char>(
    message, fmt::string_view("foo", INT_MAX + 1u));
}

TEST(UtilTest, UTF16ToUTF8Convert) {
  fmt::internal::utf16_to_utf8 u;
  EXPECT_EQ(ERROR_INVALID_PARAMETER, u.convert(fmt::wstring_view(0, 1)));
  EXPECT_EQ(ERROR_INVALID_PARAMETER,
            u.convert(fmt::wstring_view(L"foo", INT_MAX + 1u)));
}
#endif  // _WIN32

typedef void (*FormatErrorMessage)(
        fmt::internal::buffer &out, int error_code, string_view message);

template <typename Error>
void check_throw_error(int error_code, FormatErrorMessage format) {
  fmt::system_error error(0, "");
  try {
    throw Error(error_code, "test {}", "error");
  } catch (const fmt::system_error &e) {
    error = e;
  }
  fmt::memory_buffer message;
  format(message, error_code, "test error");
  EXPECT_EQ(to_string(message), error.what());
  EXPECT_EQ(error_code, error.error_code());
}

TEST(UtilTest, FormatSystemError) {
  fmt::memory_buffer message;
  fmt::format_system_error(message, EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)),
            to_string(message));
  message = fmt::memory_buffer();

  // Check if std::allocator throws on allocating max size_t / 2 chars.
  size_t max_size = std::numeric_limits<size_t>::max() / 2;
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
  fmt::format_system_error(message, EDOM, fmt::string_view(FMT_NULL, max_size));
  EXPECT_EQ(fmt::format("error {}", EDOM), to_string(message));
}

TEST(UtilTest, SystemError) {
  fmt::system_error e(EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)), e.what());
  EXPECT_EQ(EDOM, e.error_code());
  check_throw_error<fmt::system_error>(EDOM, fmt::format_system_error);
}

TEST(UtilTest, ReportSystemError) {
  fmt::memory_buffer out;
  fmt::format_system_error(out, EDOM, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr, fmt::report_system_error(EDOM, "test error"),
               to_string(out));
}

#ifdef _WIN32

TEST(UtilTest, FormatWindowsError) {
  LPWSTR message = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      ERROR_FILE_EXISTS, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, 0);
  fmt::internal::utf16_to_utf8 utf8_message(message);
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::internal::format_windows_error(
      actual_message, ERROR_FILE_EXISTS, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
      fmt::to_string(actual_message));
  actual_message.resize(0);
  fmt::internal::format_windows_error(
        actual_message, ERROR_FILE_EXISTS,
        fmt::string_view(0, std::numeric_limits<size_t>::max()));
  EXPECT_EQ(fmt::format("error {}", ERROR_FILE_EXISTS),
            fmt::to_string(actual_message));
}

TEST(UtilTest, FormatLongWindowsError) {
  LPWSTR message = 0;
  // this error code is not available on all Windows platforms and
  // Windows SDKs, so do not fail the test if the error string cannot
  // be retrieved.
  const int provisioning_not_allowed = 0x80284013L /*TBS_E_PROVISIONING_NOT_ALLOWED*/;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      static_cast<DWORD>(provisioning_not_allowed),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, 0) == 0) {
    return;
  }
  fmt::internal::utf16_to_utf8 utf8_message(message);
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::internal::format_windows_error(
      actual_message, provisioning_not_allowed, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
      fmt::to_string(actual_message));
}

TEST(UtilTest, WindowsError) {
  check_throw_error<fmt::windows_error>(
      ERROR_FILE_EXISTS, fmt::internal::format_windows_error);
}

TEST(UtilTest, ReportWindowsError) {
  fmt::memory_buffer out;
  fmt::internal::format_windows_error(out, ERROR_FILE_EXISTS, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr,
      fmt::report_windows_error(ERROR_FILE_EXISTS, "test error"),
               fmt::to_string(out));
}

#endif  // _WIN32

TEST(StringViewTest, Ctor) {
  EXPECT_STREQ("abc", string_view("abc").data());
  EXPECT_EQ(3u, string_view("abc").size());

  EXPECT_STREQ("defg", string_view(std::string("defg")).data());
  EXPECT_EQ(4u, string_view(std::string("defg")).size());
}

TEST(WriterTest, Data) {
  memory_buffer buf;
  fmt::writer w(buf);
  w.write(42);
  EXPECT_EQ("42", to_string(buf));
}

TEST(WriterTest, WriteInt) {
  CHECK_WRITE(42);
  CHECK_WRITE(-42);
  CHECK_WRITE(static_cast<short>(12));
  CHECK_WRITE(34u);
  CHECK_WRITE(std::numeric_limits<int>::min());
  CHECK_WRITE(std::numeric_limits<int>::max());
  CHECK_WRITE(std::numeric_limits<unsigned>::max());
}

TEST(WriterTest, WriteLong) {
  CHECK_WRITE(56l);
  CHECK_WRITE(78ul);
  CHECK_WRITE(std::numeric_limits<long>::min());
  CHECK_WRITE(std::numeric_limits<long>::max());
  CHECK_WRITE(std::numeric_limits<unsigned long>::max());
}

TEST(WriterTest, WriteLongLong) {
  CHECK_WRITE(56ll);
  CHECK_WRITE(78ull);
  CHECK_WRITE(std::numeric_limits<long long>::min());
  CHECK_WRITE(std::numeric_limits<long long>::max());
  CHECK_WRITE(std::numeric_limits<unsigned long long>::max());
}

TEST(WriterTest, WriteDouble) {
  CHECK_WRITE(4.2);
  CHECK_WRITE(-4.2);
  CHECK_WRITE(std::numeric_limits<double>::min());
  CHECK_WRITE(std::numeric_limits<double>::max());
}

TEST(WriterTest, WriteLongDouble) {
  CHECK_WRITE(4.2l);
  CHECK_WRITE_CHAR(-4.2l);
  std::wstring str;
  std_format(4.2l, str);
  if (str[0] != '-')
    CHECK_WRITE_WCHAR(-4.2l);
  else
    fmt::print("warning: long double formatting with std::swprintf is broken");
  CHECK_WRITE(std::numeric_limits<long double>::min());
  CHECK_WRITE(std::numeric_limits<long double>::max());
}

TEST(WriterTest, WriteDoubleAtBufferBoundary) {
  memory_buffer buf;
  fmt::writer writer(buf);
  for (int i = 0; i < 100; ++i)
    writer.write(1.23456789);
}

TEST(WriterTest, WriteDoubleWithFilledBuffer) {
  memory_buffer buf;
  fmt::writer writer(buf);
  // Fill the buffer.
  for (int i = 0; i < fmt::inline_buffer_size; ++i)
    writer.write(' ');
  writer.write(1.2);
  fmt::string_view sv(buf.data(), buf.size());
  sv.remove_prefix(fmt::inline_buffer_size);
  EXPECT_EQ("1.2", sv);
}

TEST(WriterTest, WriteChar) {
  CHECK_WRITE('a');
}

TEST(WriterTest, WriteWideChar) {
  CHECK_WRITE_WCHAR(L'a');
}

TEST(WriterTest, WriteString) {
  CHECK_WRITE_CHAR("abc");
  CHECK_WRITE_WCHAR("abc");
  // The following line shouldn't compile:
  //std::declval<fmt::basic_writer<fmt::buffer>>().write(L"abc");
}

TEST(WriterTest, WriteWideString) {
  CHECK_WRITE_WCHAR(L"abc");
  // The following line shouldn't compile:
  //std::declval<fmt::basic_writer<fmt::wbuffer>>().write("abc");
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

TEST(FormatToTest, FormatToNonbackInsertIteratorWithSignAndNumericAlignment) {
  char buffer[16] = {};
  fmt::format_to(fmt::internal::make_checked(buffer, 16), "{: =+}", 42.0);
  EXPECT_STREQ("+42", buffer);
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

TEST(FormatterTest, NoArgs) {
  EXPECT_EQ("test", format("test"));
}

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
  EXPECT_THROW_MSG(format("{0}"), format_error, "argument index out of range");
  EXPECT_THROW_MSG(format("{00}", 42), format_error, "invalid format string");

  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{%u", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), format_error, "invalid format string");
  safe_sprintf(format_str, "{%u}", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), format_error,
      "argument index out of range");

  safe_sprintf(format_str, "{%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), format_error, "number is too big");
  safe_sprintf(format_str, "{%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), format_error, "number is too big");
}

template <int N>
struct TestFormat {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args &... args) {
    return TestFormat<N - 1>::format(format_str, N - 1, args...);
  }
};

template <>
struct TestFormat<0> {
  template <typename... Args>
  static std::string format(fmt::string_view format_str, const Args &... args) {
    return fmt::format(format_str, args...);
  }
};

TEST(FormatterTest, ManyArgs) {
  EXPECT_EQ("19", TestFormat<20>::format("{19}"));
  EXPECT_THROW_MSG(TestFormat<20>::format("{20}"),
                   format_error, "argument index out of range");
  EXPECT_THROW_MSG(TestFormat<21>::format("{21}"),
                   format_error, "argument index out of range");
  enum { max_packed_args = fmt::internal::max_packed_args };
  std::string format_str = fmt::format("{{{}}}", max_packed_args + 1);
  EXPECT_THROW_MSG(TestFormat<max_packed_args>::format(format_str),
                   format_error, "argument index out of range");
}

TEST(FormatterTest, NamedArg) {
  EXPECT_EQ("1/a/A", format("{_1}/{a_}/{A_}", fmt::arg("a_", 'a'),
                            fmt::arg("A_", "A"), fmt::arg("_1", 1)));
  EXPECT_THROW_MSG(format("{a}"), format_error, "argument not found");
  EXPECT_EQ(" -42", format("{0:{width}}", -42, fmt::arg("width", 4)));
  EXPECT_EQ("st", format("{0:.{precision}}", "str", fmt::arg("precision", 2)));
  EXPECT_EQ("1 2", format("{} {two}", 1, fmt::arg("two", 2)));
  EXPECT_EQ("42", format("{c}",
        fmt::arg("a", 0), fmt::arg("b", 0), fmt::arg("c", 42), fmt::arg("d", 0),
        fmt::arg("e", 0), fmt::arg("f", 0), fmt::arg("g", 0), fmt::arg("h", 0),
        fmt::arg("i", 0), fmt::arg("j", 0), fmt::arg("k", 0), fmt::arg("l", 0),
        fmt::arg("m", 0), fmt::arg("n", 0), fmt::arg("o", 0), fmt::arg("p", 0)));
}

TEST(FormatterTest, AutoArgIndex) {
  EXPECT_EQ("abc", format("{}{}{}", 'a', 'b', 'c'));
  EXPECT_THROW_MSG(format("{0}{}", 'a', 'b'),
      format_error, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{}{0}", 'a', 'b'),
      format_error, "cannot switch from automatic to manual argument indexing");
  EXPECT_EQ("1.2", format("{:.{}}", 1.2345, 2));
  EXPECT_THROW_MSG(format("{0}:.{}", 1.2345, 2),
      format_error, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{:.{0}}", 1.2345, 2),
      format_error, "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(format("{}"), format_error, "argument index out of range");
}

TEST(FormatterTest, EmptySpecs) {
  EXPECT_EQ("42", format("{0:}", 42));
}

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
  EXPECT_EQ("-42  ", format("{0:<5}", -42.0));
  EXPECT_EQ("-42  ", format("{0:<5}", -42.0l));
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
  EXPECT_EQ("  -42", format("{0:>5}", -42.0));
  EXPECT_EQ("  -42", format("{0:>5}", -42.0l));
  EXPECT_EQ("    c", format("{0:>5}", 'c'));
  EXPECT_EQ("  abc", format("{0:>5}", "abc"));
  EXPECT_EQ("  0xface", format("{0:>8}", reinterpret_cast<void*>(0xface)));
}

TEST(FormatterTest, NumericAlign) {
  EXPECT_EQ("  42", format("{0:=4}", 42));
  EXPECT_EQ("+ 42", format("{0:=+4}", 42));
  EXPECT_EQ("  42", format("{0:=4o}", 042));
  EXPECT_EQ("+ 42", format("{0:=+4o}", 042));
  EXPECT_EQ("  42", format("{0:=4x}", 0x42));
  EXPECT_EQ("+ 42", format("{0:=+4x}", 0x42));
  EXPECT_EQ("-  42", format("{0:=5}", -42));
  EXPECT_EQ("   42", format("{0:=5}", 42u));
  EXPECT_EQ("-  42", format("{0:=5}", -42l));
  EXPECT_EQ("   42", format("{0:=5}", 42ul));
  EXPECT_EQ("-  42", format("{0:=5}", -42ll));
  EXPECT_EQ("   42", format("{0:=5}", 42ull));
  EXPECT_EQ("-  42", format("{0:=5}", -42.0));
  EXPECT_EQ("-  42", format("{0:=5}", -42.0l));
  EXPECT_THROW_MSG(format("{0:=5", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:=5}", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:=5}", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:=8}", reinterpret_cast<void*>(0xface)),
      format_error, "format specifier requires numeric argument");
  EXPECT_EQ(" 1", fmt::format("{:= }", 1.0));
}

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
  EXPECT_EQ(" -42  ", format("{0:^6}", -42.0));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42.0l));
  EXPECT_EQ("  c  ", format("{0:^5}", 'c'));
  EXPECT_EQ(" abc  ", format("{0:^6}", "abc"));
  EXPECT_EQ(" 0xface ", format("{0:^8}", reinterpret_cast<void*>(0xface)));
}

TEST(FormatterTest, Fill) {
  EXPECT_THROW_MSG(format("{0:{<5}", 'c'),
      format_error, "invalid fill character '{'");
  EXPECT_THROW_MSG(format("{0:{<5}}", 'c'),
      format_error, "invalid fill character '{'");
  EXPECT_EQ("**42", format("{0:*>4}", 42));
  EXPECT_EQ("**-42", format("{0:*>5}", -42));
  EXPECT_EQ("***42", format("{0:*>5}", 42u));
  EXPECT_EQ("**-42", format("{0:*>5}", -42l));
  EXPECT_EQ("***42", format("{0:*>5}", 42ul));
  EXPECT_EQ("**-42", format("{0:*>5}", -42ll));
  EXPECT_EQ("***42", format("{0:*>5}", 42ull));
  EXPECT_EQ("**-42", format("{0:*>5}", -42.0));
  EXPECT_EQ("**-42", format("{0:*>5}", -42.0l));
  EXPECT_EQ("c****", format("{0:*<5}", 'c'));
  EXPECT_EQ("abc**", format("{0:*<5}", "abc"));
  EXPECT_EQ("**0xface", format("{0:*>8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("foo=", format("{:}=", "foo"));
  EXPECT_EQ(std::string("\0\0\0*", 4), format(string_view("{:\0>4}", 6), '*'));
}

TEST(FormatterTest, PlusSign) {
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_EQ("-42", format("{0:+}", -42));
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_THROW_MSG(format("{0:+}", 42u),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42l));
  EXPECT_THROW_MSG(format("{0:+}", 42ul),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42ll));
  EXPECT_THROW_MSG(format("{0:+}", 42ull),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42.0));
  EXPECT_EQ("+42", format("{0:+}", 42.0l));
  EXPECT_THROW_MSG(format("{0:+", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:+}", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:+}", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:+}", reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(FormatterTest, MinusSign) {
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_EQ("-42", format("{0:-}", -42));
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_THROW_MSG(format("{0:-}", 42u),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42l));
  EXPECT_THROW_MSG(format("{0:-}", 42ul),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42ll));
  EXPECT_THROW_MSG(format("{0:-}", 42ull),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42.0));
  EXPECT_EQ("42", format("{0:-}", 42.0l));
  EXPECT_THROW_MSG(format("{0:-", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:-}", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:-}", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(FormatterTest, SpaceSign) {
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_EQ("-42", format("{0: }", -42));
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_THROW_MSG(format("{0: }", 42u),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42l));
  EXPECT_THROW_MSG(format("{0: }", 42ul),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42ll));
  EXPECT_THROW_MSG(format("{0: }", 42ull),
      format_error, "format specifier requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42.0));
  EXPECT_EQ(" 42", format("{0: }", 42.0l));
  EXPECT_THROW_MSG(format("{0: ", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0: }", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0: }", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
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

  if (FMT_USE_GRISU)
    EXPECT_EQ("-42.0", format("{0:#}", -42.0));
  else
    EXPECT_EQ("-42.0000", format("{0:#}", -42.0));
  EXPECT_EQ("-42.0000", format("{0:#}", -42.0l));
  EXPECT_THROW_MSG(format("{0:#", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:#}", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:#}", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", format("{0:0}", 42));
  EXPECT_EQ("-0042", format("{0:05}", -42));
  EXPECT_EQ("00042", format("{0:05}", 42u));
  EXPECT_EQ("-0042", format("{0:05}", -42l));
  EXPECT_EQ("00042", format("{0:05}", 42ul));
  EXPECT_EQ("-0042", format("{0:05}", -42ll));
  EXPECT_EQ("00042", format("{0:05}", 42ull));
  EXPECT_EQ("-0042", format("{0:05}", -42.0));
  EXPECT_EQ("-0042", format("{0:05}", -42.0l));
  EXPECT_THROW_MSG(format("{0:0", 'c'),
      format_error, "missing '}' in format string");
  EXPECT_THROW_MSG(format("{0:05}", 'c'),
      format_error, "invalid format specifier for char");
  EXPECT_THROW_MSG(format("{0:05}", "abc"),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", reinterpret_cast<void*>(0x42)),
      format_error, "format specifier requires numeric argument");
}

TEST(FormatterTest, Width) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:%u", UINT_MAX);
  increment(format_str + 3);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  std::size_t size = std::strlen(format_str);
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
}

TEST(FormatterTest, RuntimeWidth) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:{%u", UINT_MAX);
  increment(format_str + 4);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:{", 0),
      format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:{}", 0),
      format_error, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{0:{?}}", 0),
      format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:{1}}", 0),
      format_error, "argument index out of range");

  EXPECT_THROW_MSG(format("{0:{0:}}", 0),
      format_error, "invalid format string");

  EXPECT_THROW_MSG(format("{0:{1}}", 0, -1),
      format_error, "negative width");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, (INT_MAX + 1u)),
      format_error, "number is too big");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, -1l),
      format_error, "negative width");
  if (fmt::internal::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(format("{0:{1}}", 0, (value + 1)),
        format_error, "number is too big");
  }
  EXPECT_THROW_MSG(format("{0:{1}}", 0, (INT_MAX + 1ul)),
      format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:{1}}", 0, '0'),
      format_error, "width is not integer");
  EXPECT_THROW_MSG(format("{0:{1}}", 0, 0.0),
      format_error, "width is not integer");

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
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  safe_sprintf(format_str, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  safe_sprintf(format_str, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:.", 0),
      format_error, "missing precision specifier");
  EXPECT_THROW_MSG(format("{0:.}", 0),
      format_error, "missing precision specifier");

  EXPECT_THROW_MSG(format("{0:.2", 0),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42u),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42u),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42l),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42l),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ul),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ul),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ll),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ll),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2}", 42ull),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ull),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:3.0}", 'x'),
      format_error, "precision not allowed for this argument type");
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345));
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345l));

  EXPECT_THROW_MSG(format("{0:.2}", reinterpret_cast<void*>(0xcafe)),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.2f}", reinterpret_cast<void*>(0xcafe)),
      format_error, "precision not allowed for this argument type");

  EXPECT_EQ("st", format("{0:.2}", "str"));
}

TEST(FormatterTest, RuntimePrecision) {
  char format_str[BUFFER_SIZE];
  safe_sprintf(format_str, "{0:.{%u", UINT_MAX);
  increment(format_str + 5);
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:.{", 0),
      format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:.{}", 0),
      format_error, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{0:.{?}}", 0),
      format_error, "invalid format string");
  EXPECT_THROW_MSG(format("{0:.{1}", 0, 0),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0),
      format_error, "argument index out of range");

  EXPECT_THROW_MSG(format("{0:.{0:}}", 0),
      format_error, "invalid format string");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1),
      format_error, "negative precision");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1u)),
      format_error, "number is too big");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1l),
      format_error, "negative precision");
  if (fmt::internal::const_check(sizeof(long) > sizeof(int))) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(format("{0:.{1}}", 0, (value + 1)),
        format_error, "number is too big");
  }
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1ul)),
      format_error, "number is too big");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, '0'),
      format_error, "precision is not integer");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, 0.0),
      format_error, "precision is not integer");

  EXPECT_THROW_MSG(format("{0:.{1}}", 42, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42u, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42u, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42l, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42l, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ul, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ul, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ll, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ll, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ull, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ull, 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:3.{1}}", 'x', 0),
      format_error, "precision not allowed for this argument type");
  EXPECT_EQ("1.2", format("{0:.{1}}", 1.2345, 2));
  EXPECT_EQ("1.2", format("{1:.{0}}", 2, 1.2345l));

  EXPECT_THROW_MSG(format("{0:.{1}}", reinterpret_cast<void*>(0xcafe), 2),
      format_error, "precision not allowed for this argument type");
  EXPECT_THROW_MSG(format("{0:.{1}f}", reinterpret_cast<void*>(0xcafe), 2),
      format_error, "precision not allowed for this argument type");

  EXPECT_EQ("st", format("{0:.{1}}", "str", 2));
}

template <typename T>
void check_unknown_types(const T &value, const char *types, const char *) {
  char format_str[BUFFER_SIZE];
  const char *special = ".0123456789}";
  for (int i = CHAR_MIN; i <= CHAR_MAX; ++i) {
    char c = static_cast<char>(i);
    if (std::strchr(types, c) || std::strchr(special, c) || !c) continue;
    safe_sprintf(format_str, "{0:10%c}", c);
    const char *message = "invalid type specifier";
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
  EXPECT_THROW_MSG(format("{0:v", 42),
      format_error, "missing '}' in format string");
  check_unknown_types(42, "bBdoxXn", "integer");
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
            format("{0:b}", std::numeric_limits<uint32_t>::max()));
}

TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", format("{0}", 0));
  EXPECT_EQ("42", format("{0}", 42));
  EXPECT_EQ("42", format("{0:d}", 42));
  EXPECT_EQ("42", format("{0}", 42u));
  EXPECT_EQ("-42", format("{0}", -42));
  EXPECT_EQ("12345", format("{0}", 12345));
  EXPECT_EQ("67890", format("{0}", 67890));
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
  EXPECT_EQ("123", format("{:n}", 123));
  EXPECT_EQ("1,234", format("{:n}", 1234));
  EXPECT_EQ("1,234,567", format("{:n}", 1234567));
  EXPECT_EQ("4,294,967,295",
            format("{:n}", std::numeric_limits<uint32_t>::max()));
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
  check_unknown_types(1.2, "eEfFgGaA", "double");
  EXPECT_EQ("0", format("{:}", 0.0));
  EXPECT_EQ("0.000000", format("{:f}", 0.0));
  EXPECT_EQ("0", format("{:g}", 0.0));
  EXPECT_EQ("392.65", format("{:}", 392.65));
  EXPECT_EQ("392.65", format("{:g}", 392.65));
  EXPECT_EQ("392.65", format("{:G}", 392.65));
  EXPECT_EQ("392.650000", format("{:f}", 392.65));
  EXPECT_EQ("392.650000", format("{:F}", 392.65));
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

TEST(FormatterTest, FormatDoubleBigPrecision) {
  // sprintf with big precision is broken in MSVC2013, so only test on Grisu.
  if (FMT_USE_GRISU)
    EXPECT_EQ(format("0.{:0<1000}", ""), format("{:.1000f}", 0.0));
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
  EXPECT_EQ("0", format("{0:}", 0.0l));
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
}

TEST(FormatterTest, FormatChar) {
  const char types[] = "cbBdoxXn";
  check_unknown_types('a', types, "char");
  EXPECT_EQ("a", format("{0}", 'a'));
  EXPECT_EQ("z", format("{0:c}", 'z'));
  EXPECT_EQ(L"a", format(L"{0}", 'a'));
  int n = 'x';
  for (const char *type = types + 1; *type; ++type) {
    std::string format_str = fmt::format("{{:{}}}", *type);
    EXPECT_EQ(fmt::format(format_str, n), fmt::format(format_str, 'x'));
  }
  EXPECT_EQ(fmt::format("{:02X}", n), fmt::format("{:02X}", 'x'));
}

TEST(FormatterTest, FormatUnsignedChar) {
  EXPECT_EQ("42", format("{}", static_cast<unsigned char>(42)));
  EXPECT_EQ("42", format("{}", static_cast<uint8_t>(42)));
}

TEST(FormatterTest, FormatWChar) {
  EXPECT_EQ(L"a", format(L"{0}", L'a'));
  // This shouldn't compile:
  //format("{}", L'a');
}

TEST(FormatterTest, FormatCString) {
  check_unknown_types("test", "sp", "string");
  EXPECT_EQ("test", format("{0}", "test"));
  EXPECT_EQ("test", format("{0:s}", "test"));
  char nonconst[] = "nonconst";
  EXPECT_EQ("nonconst", format("{0}", nonconst));
  EXPECT_THROW_MSG(format("{0}", static_cast<const char*>(FMT_NULL)),
      format_error, "string pointer is null");
}

TEST(FormatterTest, FormatSCharString) {
  signed char str[] = "test";
  EXPECT_EQ("test", format("{0:s}", str));
  const signed char *const_str = str;
  EXPECT_EQ("test", format("{0:s}", const_str));
}

TEST(FormatterTest, FormatUCharString) {
  unsigned char str[] = "test";
  EXPECT_EQ("test", format("{0:s}", str));
  const unsigned char *const_str = str;
  EXPECT_EQ("test", format("{0:s}", const_str));
  unsigned char *ptr = str;
  EXPECT_EQ("test", format("{0:s}", ptr));
}

TEST(FormatterTest, FormatPointer) {
  check_unknown_types(reinterpret_cast<void*>(0x1234), "p", "pointer");
  EXPECT_EQ("0x0", format("{0}", static_cast<void*>(FMT_NULL)));
  EXPECT_EQ("0x1234", format("{0}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x1234", format("{0:p}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x" + std::string(sizeof(void*) * CHAR_BIT / 4, 'f'),
      format("{0}", reinterpret_cast<void*>(~uintptr_t())));
  EXPECT_EQ("0x1234", format("{}", fmt::ptr(reinterpret_cast<int*>(0x1234))));
#if FMT_USE_NULLPTR
  EXPECT_EQ("0x0", format("{}", FMT_NULL));
#endif
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", format("{0}", std::string("test")));
}

TEST(FormatterTest, FormatStringView) {
  EXPECT_EQ("test", format("{}", string_view("test")));
  EXPECT_EQ("", format("{}", string_view()));
}

#ifdef FMT_USE_STD_STRING_VIEW
TEST(FormatterTest, FormatStdStringView) {
  EXPECT_EQ("test", format("{0}", std::string_view("test")));
}
#endif

FMT_BEGIN_NAMESPACE
template <>
struct formatter<Date> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (*it == 'd')
      ++it;
    return it;
  }

  auto format(const Date &d, format_context &ctx) -> decltype(ctx.out()) {
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
template <>
struct formatter<Answer> : formatter<int> {
  template <typename FormatContext>
  auto format(Answer, FormatContext &ctx) -> decltype(ctx.out()) {
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
  auto end = &*fmt::format_to(
        fmt::internal::make_checked(buf, 10), "{}", Answer());
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
      format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%",
          1.234, 42, 3.13, "str", reinterpret_cast<void*>(1000), 'X'));
}

TEST(FormatterTest, FormatExamples) {
  std::string message = format("The answer is {}", 42);
  EXPECT_EQ("The answer is 42", message);

  EXPECT_EQ("42", format("{}", 42));
  EXPECT_EQ("42", format(std::string("{}"), 42));

  memory_buffer out;
  format_to(out, "The answer is {}.", 42);
  EXPECT_EQ("The answer is 42.", to_string(out));

  const char *filename = "nonexistent";
  FILE *ftest = safe_fopen(filename, "r");
  if (ftest) fclose(ftest);
  int error_code = errno;
  EXPECT_TRUE(ftest == FMT_NULL);
  EXPECT_SYSTEM_ERROR({
    FILE *f = safe_fopen(filename, "r");
    if (!f)
      throw fmt::system_error(errno, "Cannot open file '{}'", filename);
    fclose(f);
  }, error_code, "Cannot open file 'nonexistent'");
}

TEST(FormatterTest, Examples) {
  EXPECT_EQ("First, thou shalt count to three",
      format("First, thou shalt count to {0}", "three"));
  EXPECT_EQ("Bring me a shrubbery",
      format("Bring me a {}", "shrubbery"));
  EXPECT_EQ("From 1 to 3", format("From {} to {}", 1, 3));

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%03.2f", -1.2);
  EXPECT_EQ(buffer, format("{:03.2f}", -1.2));

  EXPECT_EQ("a, b, c", format("{0}, {1}, {2}", 'a', 'b', 'c'));
  EXPECT_EQ("a, b, c", format("{}, {}, {}", 'a', 'b', 'c'));
  EXPECT_EQ("c, b, a", format("{2}, {1}, {0}", 'a', 'b', 'c'));
  EXPECT_EQ("abracadabra", format("{0}{1}{0}", "abra", "cad"));

  EXPECT_EQ("left aligned                  ",
      format("{:<30}", "left aligned"));
  EXPECT_EQ("                 right aligned",
      format("{:>30}", "right aligned"));
  EXPECT_EQ("           centered           ",
      format("{:^30}", "centered"));
  EXPECT_EQ("***********centered***********",
      format("{:*^30}", "centered"));

  EXPECT_EQ("+3.140000; -3.140000",
      format("{:+f}; {:+f}", 3.14, -3.14));
  EXPECT_EQ(" 3.140000; -3.140000",
      format("{: f}; {: f}", 3.14, -3.14));
  EXPECT_EQ("3.140000; -3.140000",
      format("{:-f}; {:-f}", 3.14, -3.14));

  EXPECT_EQ("int: 42;  hex: 2a;  oct: 52",
      format("int: {0:d};  hex: {0:x};  oct: {0:o}", 42));
  EXPECT_EQ("int: 42;  hex: 0x2a;  oct: 052",
      format("int: {0:d};  hex: {0:#x};  oct: {0:#o}", 42));

  EXPECT_EQ("The answer is 42", format("The answer is {}", 42));
  EXPECT_THROW_MSG(
    format("The answer is {:d}", "forty-two"), format_error,
    "invalid type specifier");

  EXPECT_EQ(L"Cyrillic letter \x42e",
    format(L"Cyrillic letter {}", L'\x42e'));

  EXPECT_WRITE(stdout,
      fmt::print("{}", std::numeric_limits<double>::infinity()), "inf");
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
  os << std::numeric_limits<int64_t>::max();
  EXPECT_EQ(os.str(),
            fmt::format_int(std::numeric_limits<int64_t>::max()).str());
}

template <typename T>
std::string format_decimal(T value) {
  char buffer[10];
  char *ptr = buffer;
  fmt::format_decimal(ptr, value);
  return std::string(buffer, ptr);
}

TEST(FormatIntTest, FormatDec) {
  EXPECT_EQ("-42", format_decimal(static_cast<signed char>(-42)));
  EXPECT_EQ("-42", format_decimal(static_cast<short>(-42)));
  std::ostringstream os;
  os << std::numeric_limits<unsigned short>::max();
  EXPECT_EQ(os.str(),
            format_decimal(std::numeric_limits<unsigned short>::max()));
  EXPECT_EQ("1", format_decimal(1));
  EXPECT_EQ("-1", format_decimal(-1));
  EXPECT_EQ("42", format_decimal(42));
  EXPECT_EQ("-42", format_decimal(-42));
  EXPECT_EQ("42", format_decimal(42l));
  EXPECT_EQ("42", format_decimal(42ul));
  EXPECT_EQ("42", format_decimal(42ll));
  EXPECT_EQ("42", format_decimal(42ull));
}

TEST(FormatTest, Print) {
#if FMT_USE_FILE_DESCRIPTORS
  EXPECT_WRITE(stdout, fmt::print("Don't {}!", "panic"), "Don't panic!");
  EXPECT_WRITE(stderr,
      fmt::print(stderr, "Don't {}!", "panic"), "Don't panic!");
#endif
}

TEST(FormatTest, Variadic) {
  EXPECT_EQ("abc1", format("{}c{}", "ab", 1));
  EXPECT_EQ(L"abc1", format(L"{}c{}", L"ab", 1));
}

TEST(FormatTest, Dynamic) {
  typedef fmt::format_context ctx;
  std::vector<fmt::basic_format_arg<ctx>> args;
  args.emplace_back(fmt::internal::make_arg<ctx>(42));
  args.emplace_back(fmt::internal::make_arg<ctx>("abc1"));
  args.emplace_back(fmt::internal::make_arg<ctx>(1.2f));

  std::string result = fmt::vformat("{} and {} and {}",
                                    fmt::basic_format_args<ctx>(
                                        args.data(),
                                        static_cast<unsigned>(args.size())));

  EXPECT_EQ("42 and abc1 and 1.2", result);
}

TEST(FormatTest, JoinArg) {
  using fmt::join;
  int v1[3] = { 1, 2, 3 };
  std::vector<float> v2;
  v2.push_back(1.2f);
  v2.push_back(3.4f);
  void *v3[2] = { &v1[0], &v1[1] };

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

#if FMT_USE_TRAILING_RETURN && (!FMT_GCC_VERSION || FMT_GCC_VERSION >= 405)
  EXPECT_EQ("(1, 2, 3)", format("({})", join(v1, ", ")));
  EXPECT_EQ("(+01.20, +03.40)", format("({:+06.2f})", join(v2, ", ")));
#endif
}

template <typename T>
std::string str(const T &value) {
  return fmt::format("{}", value);
}

TEST(StrTest, Convert) {
  EXPECT_EQ("42", str(42));
  std::string s = str(Date(2012, 12, 9));
  EXPECT_EQ("2012-12-9", s);
}

std::string vformat_message(int id, const char *format, fmt::format_args args) {
  fmt::memory_buffer buffer;
  format_to(buffer, "[{}] ", id);
  vformat_to(buffer, format, args);
  return to_string(buffer);
}

template <typename... Args>
std::string format_message(int id, const char *format, const Args & ... args) {
  auto va = fmt::make_format_args(args...);
  return vformat_message(id, format, va);
}

TEST(FormatTest, FormatMessageExample) {
  EXPECT_EQ("[42] something happened",
      format_message(42, "{} happened", "something"));
}

template<typename... Args>
void print_error(const char *file, int line, const char *format,
                 const Args & ... args) {
  fmt::print("{}: {}: ", file, line);
  fmt::print(format, args...);
}

TEST(FormatTest, UnpackedArgs) {
  EXPECT_EQ("0123456789abcdefg",
            fmt::format("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}",
                        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e',
                        'f', 'g'));
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
  auto udl_a = format("{first}{second}{first}{third}",
                      "first"_a="abra", "second"_a="cad", "third"_a=99);
  EXPECT_EQ(format("{first}{second}{first}{third}",
                   fmt::arg("first", "abra"), fmt::arg("second", "cad"),
                   fmt::arg("third", 99)),
            udl_a);
  auto udl_a_w = format(L"{first}{second}{first}{third}",
                        L"first"_a=L"abra", L"second"_a=L"cad", L"third"_a=99);
  EXPECT_EQ(format(L"{first}{second}{first}{third}",
                   fmt::arg(L"first", L"abra"), fmt::arg(L"second", L"cad"),
                   fmt::arg(L"third", 99)),
            udl_a_w);
}

TEST(FormatTest, UdlTemplate) {
  EXPECT_EQ("foo", "foo"_format());
  EXPECT_EQ("        42", "{0:10}"_format(42));
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(L"{}"), 42));
}
#endif // FMT_USE_USER_DEFINED_LITERALS

enum TestEnum { A };

TEST(FormatTest, Enum) {
  EXPECT_EQ("0", fmt::format("{}", A));
}

TEST(FormatTest, EnumFormatterUnambiguous) {
  fmt::formatter<TestEnum> f;
  ASSERT_GE(sizeof(f), 0); // use f to avoid compiler warning
}

#if FMT_HAS_FEATURE(cxx_strong_enums)
enum TestFixedEnum : short { B };

TEST(FormatTest, FixedEnum) {
  EXPECT_EQ("0", fmt::format("{}", B));
}
#endif

typedef fmt::back_insert_range<fmt::internal::buffer> buffer_range;

class mock_arg_formatter:
    public fmt::internal::function<
      fmt::internal::arg_formatter_base<buffer_range>::iterator>,
    public fmt::internal::arg_formatter_base<buffer_range> {
 private:
  MOCK_METHOD1(call, void (long long value));

 public:
  typedef fmt::internal::arg_formatter_base<buffer_range> base;
  typedef buffer_range range;

  mock_arg_formatter(fmt::format_context &ctx, fmt::format_specs *s = FMT_NULL)
    : base(fmt::internal::get_container(ctx.out()), s, ctx.locale()) {
    EXPECT_CALL(*this, call(42));
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, iterator>::type
      operator()(T value) {
    call(value);
    return base::operator()(value);
  }

  template <typename T>
  typename std::enable_if<!std::is_integral<T>::value, iterator>::type
      operator()(T value) {
    return base::operator()(value);
  }

  iterator operator()(fmt::basic_format_arg<fmt::format_context>::handle) {
    return base::operator()(fmt::monostate());
  }
};

static void custom_vformat(fmt::string_view format_str, fmt::format_args args) {
  fmt::memory_buffer buffer;
  fmt::vformat_to<mock_arg_formatter>(buffer, format_str, args);
}

template <typename... Args>
void custom_format(const char *format_str, const Args & ... args) {
  auto va = fmt::make_format_args(args...);
  return custom_vformat(format_str, va);
}

TEST(FormatTest, CustomArgFormatter) {
  custom_format("{}", 42);
}

TEST(FormatTest, NonNullTerminatedFormatString) {
  EXPECT_EQ("42", format(string_view("{}foo", 2), 42));
}

struct variant {
  enum {INT, STRING} type;
  explicit variant(int) : type(INT) {}
  explicit variant(const char *) : type(STRING) {}
};

FMT_BEGIN_NAMESPACE
template <>
struct formatter<variant> : dynamic_formatter<> {
  auto format(variant value, format_context& ctx) -> decltype(ctx.out()) {
    if (value.type == variant::INT)
      return dynamic_formatter<>::format(42, ctx);
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
  EXPECT_THROW_MSG(format("{0:{}}", num),
      format_error, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{:{0}}", num),
      format_error, "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(format("{:=}", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:+}", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:-}", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{: }", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:#}", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:0}", str),
      format_error, "format specifier requires numeric argument");
  EXPECT_THROW_MSG(format("{:.2}", num),
      format_error, "precision not allowed for this argument type");
}

TEST(FormatTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
  EXPECT_EQ("0x1234", fmt::to_string(reinterpret_cast<void*>(0x1234)));
}

TEST(FormatTest, ToWString) {
  EXPECT_EQ(L"42", fmt::to_wstring(42));
}

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
}

TEST(FormatTest, WideFormatToN) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, L"{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
}

#if FMT_USE_CONSTEXPR
struct test_arg_id_handler {
  enum result { NONE, EMPTY, INDEX, NAME, ERROR };
  result res = NONE;
  unsigned index = 0;
  string_view name;

  FMT_CONSTEXPR void operator()() { res = EMPTY; }

  FMT_CONSTEXPR void operator()(unsigned i) {
    res = INDEX;
    index = i;
  }

  FMT_CONSTEXPR void operator()(string_view n) {
    res = NAME;
    name = n;
  }

  FMT_CONSTEXPR void on_error(const char *) { res = ERROR; }
};

template <size_t N>
FMT_CONSTEXPR test_arg_id_handler parse_arg_id(const char (&s)[N]) {
  test_arg_id_handler h;
  fmt::internal::parse_arg_id(s, s + N, h);
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

  fmt::alignment align_ = fmt::ALIGN_DEFAULT;
  char fill = 0;
  unsigned width = 0;
  fmt::internal::arg_ref<char> width_ref;
  unsigned precision = 0;
  fmt::internal::arg_ref<char> precision_ref;
  char type = 0;

  // Workaround for MSVC2017 bug that results in "expression did not evaluate
  // to a constant" with compiler-generated copy ctor.
  FMT_CONSTEXPR test_format_specs_handler() {}
  FMT_CONSTEXPR test_format_specs_handler(const test_format_specs_handler &other)
  : res(other.res), align_(other.align_), fill(other.fill),
    width(other.width), width_ref(other.width_ref),
    precision(other.precision), precision_ref(other.precision_ref),
    type(other.type) {}

  FMT_CONSTEXPR void on_align(fmt::alignment a) { align_ = a; }
  FMT_CONSTEXPR void on_fill(char f) { fill = f; }
  FMT_CONSTEXPR void on_plus() { res = PLUS; }
  FMT_CONSTEXPR void on_minus() { res = MINUS; }
  FMT_CONSTEXPR void on_space() { res = SPACE; }
  FMT_CONSTEXPR void on_hash() { res = HASH; }
  FMT_CONSTEXPR void on_zero() { res = ZERO; }

  FMT_CONSTEXPR void on_width(unsigned w) { width = w; }
  FMT_CONSTEXPR void on_dynamic_width(fmt::internal::auto_id) {}
  FMT_CONSTEXPR void on_dynamic_width(unsigned index) { width_ref = index; }
  FMT_CONSTEXPR void on_dynamic_width(string_view) {}

  FMT_CONSTEXPR void on_precision(unsigned p) { precision = p; }
  FMT_CONSTEXPR void on_dynamic_precision(fmt::internal::auto_id) {}
  FMT_CONSTEXPR void on_dynamic_precision(unsigned index) {
    precision_ref = index;
  }
  FMT_CONSTEXPR void on_dynamic_precision(string_view) {}

  FMT_CONSTEXPR void end_precision() {}
  FMT_CONSTEXPR void on_type(char t) { type = t; }
  FMT_CONSTEXPR void on_error(const char *) { res = ERROR; }
};

template <size_t N>
FMT_CONSTEXPR test_format_specs_handler parse_test_specs(const char (&s)[N]) {
  test_format_specs_handler h;
  fmt::internal::parse_format_specs(s, s + N, h);
  return h;
}

TEST(FormatTest, ConstexprParseFormatSpecs) {
  typedef test_format_specs_handler handler;
  static_assert(parse_test_specs("<").align_ == fmt::ALIGN_LEFT, "");
  static_assert(parse_test_specs("*^").fill == '*', "");
  static_assert(parse_test_specs("+").res == handler::PLUS, "");
  static_assert(parse_test_specs("-").res == handler::MINUS, "");
  static_assert(parse_test_specs(" ").res == handler::SPACE, "");
  static_assert(parse_test_specs("#").res == handler::HASH, "");
  static_assert(parse_test_specs("0").res == handler::ZERO, "");
  static_assert(parse_test_specs("42").width == 42, "");
  static_assert(parse_test_specs("{42}").width_ref.index == 42, "");
  static_assert(parse_test_specs(".42").precision == 42, "");
  static_assert(parse_test_specs(".{42}").precision_ref.index == 42, "");
  static_assert(parse_test_specs("d").type == 'd', "");
  static_assert(parse_test_specs("{<").res == handler::ERROR, "");
}

struct test_context {
  typedef char char_type;

  FMT_CONSTEXPR fmt::basic_format_arg<test_context> next_arg() {
    return fmt::internal::make_arg<test_context>(11);
  }

  template <typename Id>
  FMT_CONSTEXPR fmt::basic_format_arg<test_context> get_arg(Id) {
    return fmt::internal::make_arg<test_context>(22);
  }

  template <typename Id>
  FMT_CONSTEXPR void check_arg_id(Id) {}

  FMT_CONSTEXPR unsigned next_arg_id() { return 33; }

  void on_error(const char *) {}

  FMT_CONSTEXPR test_context &parse_context() { return *this; }
  FMT_CONSTEXPR test_context error_handler() { return *this; }
};

template <size_t N>
FMT_CONSTEXPR fmt::format_specs parse_specs(const char (&s)[N]) {
  fmt::format_specs specs;
  test_context ctx{};
  fmt::internal::specs_handler<test_context> h(specs, ctx);
  parse_format_specs(s, s + N, h);
  return specs;
}

TEST(FormatTest, ConstexprSpecsHandler) {
  static_assert(parse_specs("<").align() == fmt::ALIGN_LEFT, "");
  static_assert(parse_specs("*^").fill() == '*', "");
  static_assert(parse_specs("+").has(fmt::PLUS_FLAG), "");
  static_assert(parse_specs("-").has(fmt::MINUS_FLAG), "");
  static_assert(parse_specs(" ").has(fmt::SIGN_FLAG), "");
  static_assert(parse_specs("#").has(fmt::HASH_FLAG), "");
  static_assert(parse_specs("0").align() == fmt::ALIGN_NUMERIC, "");
  static_assert(parse_specs("42").width() == 42, "");
  static_assert(parse_specs("{}").width() == 11, "");
  static_assert(parse_specs("{0}").width() == 22, "");
  static_assert(parse_specs(".42").precision == 42, "");
  static_assert(parse_specs(".{}").precision == 11, "");
  static_assert(parse_specs(".{0}").precision == 22, "");
  static_assert(parse_specs("d").type == 'd', "");
}

template <size_t N>
FMT_CONSTEXPR fmt::internal::dynamic_format_specs<char>
    parse_dynamic_specs(const char (&s)[N]) {
  fmt::internal::dynamic_format_specs<char> specs;
  test_context ctx{};
  fmt::internal::dynamic_specs_handler<test_context> h(specs, ctx);
  parse_format_specs(s, s + N, h);
  return specs;
}

TEST(FormatTest, ConstexprDynamicSpecsHandler) {
  static_assert(parse_dynamic_specs("<").align() == fmt::ALIGN_LEFT, "");
  static_assert(parse_dynamic_specs("*^").fill() == '*', "");
  static_assert(parse_dynamic_specs("+").has(fmt::PLUS_FLAG), "");
  static_assert(parse_dynamic_specs("-").has(fmt::MINUS_FLAG), "");
  static_assert(parse_dynamic_specs(" ").has(fmt::SIGN_FLAG), "");
  static_assert(parse_dynamic_specs("#").has(fmt::HASH_FLAG), "");
  static_assert(parse_dynamic_specs("0").align() == fmt::ALIGN_NUMERIC, "");
  static_assert(parse_dynamic_specs("42").width() == 42, "");
  static_assert(parse_dynamic_specs("{}").width_ref.index == 33, "");
  static_assert(parse_dynamic_specs("{42}").width_ref.index == 42, "");
  static_assert(parse_dynamic_specs(".42").precision == 42, "");
  static_assert(parse_dynamic_specs(".{}").precision_ref.index == 33, "");
  static_assert(parse_dynamic_specs(".{42}").precision_ref.index == 42, "");
  static_assert(parse_dynamic_specs("d").type == 'd', "");
}

template <size_t N>
FMT_CONSTEXPR test_format_specs_handler check_specs(const char (&s)[N]) {
  fmt::internal::specs_checker<test_format_specs_handler>
      checker(test_format_specs_handler(), fmt::internal::double_type);
  parse_format_specs(s, s + N, checker);
  return checker;
}

TEST(FormatTest, ConstexprSpecsChecker) {
  typedef test_format_specs_handler handler;
  static_assert(check_specs("<").align_ == fmt::ALIGN_LEFT, "");
  static_assert(check_specs("*^").fill == '*', "");
  static_assert(check_specs("+").res == handler::PLUS, "");
  static_assert(check_specs("-").res == handler::MINUS, "");
  static_assert(check_specs(" ").res == handler::SPACE, "");
  static_assert(check_specs("#").res == handler::HASH, "");
  static_assert(check_specs("0").res == handler::ZERO, "");
  static_assert(check_specs("42").width == 42, "");
  static_assert(check_specs("{42}").width_ref.index == 42, "");
  static_assert(check_specs(".42").precision == 42, "");
  static_assert(check_specs(".{42}").precision_ref.index == 42, "");
  static_assert(check_specs("d").type == 'd', "");
  static_assert(check_specs("{<").res == handler::ERROR, "");
}

struct test_format_string_handler {
  FMT_CONSTEXPR void on_text(const char *, const char *) {}

  FMT_CONSTEXPR void on_arg_id() {}

  template <typename T>
  FMT_CONSTEXPR void on_arg_id(T) {}

  FMT_CONSTEXPR void on_replacement_field(const char *) {}

  FMT_CONSTEXPR const char *on_format_specs(const char *begin, const char*) {
    return begin;
  }

  FMT_CONSTEXPR void on_error(const char *) { error = true; }

  bool error = false;
};

template <size_t N>
FMT_CONSTEXPR bool parse_string(const char (&s)[N]) {
  test_format_string_handler h;
  fmt::internal::parse_format_string<true>(fmt::string_view(s, N - 1), h);
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
  const char *&error;

  FMT_CONSTEXPR test_error_handler(const char *&err): error(err) {}

  FMT_CONSTEXPR test_error_handler(const test_error_handler &other)
    : error(other.error) {}

  FMT_CONSTEXPR void on_error(const char *message) {
    if (!error)
      error = message;
  }
};

FMT_CONSTEXPR size_t len(const char *s) {
  size_t len = 0;
  while (*s++)
    ++len;
  return len;
}

FMT_CONSTEXPR bool equal(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return s1 == s2;
  while (*s1 && *s1 == *s2) {
    ++s1;
    ++s2;
  }
  return *s1 == *s2;
}

template <typename... Args>
FMT_CONSTEXPR bool test_error(const char *fmt, const char *expected_error) {
  const char *actual_error = FMT_NULL;
  fmt::internal::do_check_format_string<char, test_error_handler, Args...>(
        string_view(fmt, len(fmt)), test_error_handler(actual_error));
  return equal(actual_error, expected_error);
}

#define EXPECT_ERROR_NOARGS(fmt, error) \
  static_assert(test_error(fmt, error), "")
#define EXPECT_ERROR(fmt, error, ...) \
  static_assert(test_error<__VA_ARGS__>(fmt, error), "")

TEST(FormatTest, FormatStringErrors) {
  EXPECT_ERROR_NOARGS("foo", FMT_NULL);
  EXPECT_ERROR_NOARGS("}", "unmatched '}' in format string");
  EXPECT_ERROR("{0:s", "unknown format specifier", Date);
#ifndef _MSC_VER
  // This causes an internal compiler error in MSVC2017.
  EXPECT_ERROR("{0:=5", "unknown format specifier", int);
  EXPECT_ERROR("{:{<}", "invalid fill character '{'", int);
  EXPECT_ERROR("{:10000000000}", "number is too big", int);
  EXPECT_ERROR("{:.10000000000}", "number is too big", int);
  EXPECT_ERROR_NOARGS("{:x}", "argument index out of range");
  EXPECT_ERROR("{:=}", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{:+}", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{:-}", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{:#}", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{: }", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{:0}", "format specifier requires numeric argument",
               const char *);
  EXPECT_ERROR("{:+}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:-}", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{: }", "format specifier requires signed argument", unsigned);
  EXPECT_ERROR("{:.2}", "precision not allowed for this argument type", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", int);
  EXPECT_ERROR("{:s}", "invalid type specifier", bool);
  EXPECT_ERROR("{:s}", "invalid type specifier", char);
  EXPECT_ERROR("{:+}", "invalid format specifier for char", char);
  EXPECT_ERROR("{:s}", "invalid type specifier", double);
  EXPECT_ERROR("{:d}", "invalid type specifier", const char *);
  EXPECT_ERROR("{:d}", "invalid type specifier", std::string);
  EXPECT_ERROR("{:s}", "invalid type specifier", void *);
#endif
  EXPECT_ERROR("{foo", "missing '}' in format string", int);
  EXPECT_ERROR_NOARGS("{10000000000}", "number is too big");
  EXPECT_ERROR_NOARGS("{0x}", "invalid format string");
  EXPECT_ERROR_NOARGS("{-}", "invalid format string");
  EXPECT_ERROR("{:{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.{0x}}", "invalid format string", int);
  EXPECT_ERROR("{:.{-}}", "invalid format string", int);
  EXPECT_ERROR("{:.x}", "missing precision specifier", int);
  EXPECT_ERROR_NOARGS("{}", "argument index out of range");
  EXPECT_ERROR("{1}", "argument index out of range", int);
  EXPECT_ERROR("{1}{}",
               "cannot switch from manual to automatic argument indexing",
               int, int);
  EXPECT_ERROR("{}{1}",
               "cannot switch from automatic to manual argument indexing",
               int, int);
}

TEST(FormatTest, VFormatTo) {
  typedef fmt::format_context context;
  fmt::basic_format_arg<context> arg = fmt::internal::make_arg<context>(42);
  fmt::basic_format_args<context> args(&arg, 1);
  std::string s;
  fmt::vformat_to(std::back_inserter(s), "{}", args);
  EXPECT_EQ("42", s);
  s.clear();
  fmt::vformat_to(std::back_inserter(s), FMT_STRING("{}"), args);
  EXPECT_EQ("42", s);

  typedef fmt::wformat_context wcontext;
  fmt::basic_format_arg<wcontext> warg = fmt::internal::make_arg<wcontext>(42);
  fmt::basic_format_args<wcontext> wargs(&warg, 1);
  std::wstring w;
  fmt::vformat_to(std::back_inserter(w), L"{}", wargs);
  EXPECT_EQ(L"42", w);
  w.clear();
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

#endif  // FMT_USE_CONSTEXPR

TEST(FormatTest, ConstructU8StringViewFromCString) {
  fmt::u8string_view s("ab");
  EXPECT_EQ(s.size(), 2u);
  const fmt::char8_t *data = s.data();
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[1], 'b');
}

TEST(FormatTest, ConstructU8StringViewFromDataAndSize) {
  fmt::u8string_view s("foobar", 3);
  EXPECT_EQ(s.size(), 3u);
  const fmt::char8_t *data = s.data();
  EXPECT_EQ(data[0], 'f');
  EXPECT_EQ(data[1], 'o');
  EXPECT_EQ(data[2], 'o');
}

#if FMT_USE_USER_DEFINED_LITERALS
TEST(FormatTest, U8StringViewLiteral) {
  using namespace fmt::literals;
  fmt::u8string_view s = "ab"_u;
  EXPECT_EQ(s.size(), 2u);
  const fmt::char8_t *data = s.data();
  EXPECT_EQ(data[0], 'a');
  EXPECT_EQ(data[1], 'b');
  EXPECT_EQ(format("{:*^5}"_u, "🤡"_u), "**🤡**"_u);
}
#endif

TEST(FormatTest, FormatU8String) {
  EXPECT_EQ(format(fmt::u8string_view("{}"), 42), fmt::u8string_view("42"));
}
