/*
 Utility tests.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "test-assert.h"

#include <cfloat>
#include <climits>
#include <cstring>
#include <functional>
#include <limits>

#if FMT_USE_TYPE_TRAITS
# include <type_traits>
#endif

#include "gmock/gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

// Check if format.h compiles with windows.h included.
#ifdef _WIN32
# include <windows.h>
#endif

#include "fmt/format.h"

#undef min
#undef max

using fmt::basic_format_arg;
using fmt::format_arg;
using fmt::Buffer;
using fmt::StringRef;
using fmt::internal::MemoryBuffer;
using fmt::internal::value;

using testing::_;
using testing::Return;
using testing::StrictMock;

namespace {

struct Test {};

template <typename Char>
void format_value(fmt::basic_writer<Char> &w, Test,
                  fmt::basic_format_context<Char> &) {
  w << "test";
}

template <typename Context, typename T>
basic_format_arg<Context> make_arg(const T &value) {
  return fmt::internal::make_arg<Context>(value);
}
}  // namespace

void CheckForwarding(
    MockAllocator<int> &alloc, AllocatorRef< MockAllocator<int> > &ref) {
  int mem;
  // Check if value_type is properly defined.
  AllocatorRef< MockAllocator<int> >::value_type *ptr = &mem;
  // Check forwarding.
  EXPECT_CALL(alloc, allocate(42)).WillOnce(Return(ptr));
  ref.allocate(42);
  EXPECT_CALL(alloc, deallocate(ptr, 42));
  ref.deallocate(ptr, 42);
}

TEST(AllocatorTest, AllocatorRef) {
  StrictMock< MockAllocator<int> > alloc;
  typedef AllocatorRef< MockAllocator<int> > TestAllocatorRef;
  TestAllocatorRef ref(&alloc);
  // Check if AllocatorRef forwards to the underlying allocator.
  CheckForwarding(alloc, ref);
  TestAllocatorRef ref2(ref);
  CheckForwarding(alloc, ref2);
  TestAllocatorRef ref3;
  EXPECT_EQ(0, ref3.get());
  ref3 = ref;
  CheckForwarding(alloc, ref3);
}

#if FMT_USE_TYPE_TRAITS
TEST(BufferTest, Noncopyable) {
  EXPECT_FALSE(std::is_copy_constructible<Buffer<char> >::value);
  EXPECT_FALSE(std::is_copy_assignable<Buffer<char> >::value);
}

TEST(BufferTest, Nonmoveable) {
  EXPECT_FALSE(std::is_move_constructible<Buffer<char> >::value);
  EXPECT_FALSE(std::is_move_assignable<Buffer<char> >::value);
}
#endif

// A test buffer with a dummy grow method.
template <typename T>
struct TestBuffer : Buffer<T> {
  void grow(std::size_t size) { this->capacity_ = size; }
};

template <typename T>
struct MockBuffer : Buffer<T> {
  MOCK_METHOD1(do_grow, void (std::size_t size));

  void grow(std::size_t size) {
    this->capacity_ = size;
    do_grow(size);
  }

  MockBuffer() {}
  MockBuffer(T *ptr) : Buffer<T>(ptr) {}
  MockBuffer(T *ptr, std::size_t capacity) : Buffer<T>(ptr, capacity) {}
};

TEST(BufferTest, Ctor) {
  {
    MockBuffer<int> buffer;
    EXPECT_EQ(0, &buffer[0]);
    EXPECT_EQ(0u, buffer.size());
    EXPECT_EQ(0u, buffer.capacity());
  }
  {
    int dummy;
    MockBuffer<int> buffer(&dummy);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(0u, buffer.size());
    EXPECT_EQ(0u, buffer.capacity());
  }
  {
    int dummy;
    std::size_t capacity = std::numeric_limits<std::size_t>::max();
    MockBuffer<int> buffer(&dummy, capacity);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(0u, buffer.size());
    EXPECT_EQ(capacity, buffer.capacity());
  }
}

struct DyingBuffer : TestBuffer<int> {
  MOCK_METHOD0(die, void());
  ~DyingBuffer() { die(); }
};

TEST(BufferTest, VirtualDtor) {
  typedef StrictMock<DyingBuffer> StictMockBuffer;
  StictMockBuffer *mock_buffer = new StictMockBuffer();
  EXPECT_CALL(*mock_buffer, die());
  Buffer<int> *buffer = mock_buffer;
  delete buffer;
}

TEST(BufferTest, Access) {
  char data[10];
  MockBuffer<char> buffer(data, sizeof(data));
  buffer[0] = 11;
  EXPECT_EQ(11, buffer[0]);
  buffer[3] = 42;
  EXPECT_EQ(42, *(&buffer[0] + 3));
  const Buffer<char> &const_buffer = buffer;
  EXPECT_EQ(42, const_buffer[3]);
}

TEST(BufferTest, Resize) {
  char data[123];
  MockBuffer<char> buffer(data, sizeof(data));
  buffer[10] = 42;
  EXPECT_EQ(42, buffer[10]);
  buffer.resize(20);
  EXPECT_EQ(20u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  buffer.resize(5);
  EXPECT_EQ(5u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  // Check if resize calls grow.
  EXPECT_CALL(buffer, do_grow(124));
  buffer.resize(124);
  EXPECT_CALL(buffer, do_grow(200));
  buffer.resize(200);
}

TEST(BufferTest, Clear) {
  TestBuffer<char> buffer;
  buffer.resize(20);
  buffer.clear();
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(20u, buffer.capacity());
}

TEST(BufferTest, PushBack) {
  int data[15];
  MockBuffer<int> buffer(data, 10);
  buffer.push_back(11);
  EXPECT_EQ(11, buffer[0]);
  EXPECT_EQ(1u, buffer.size());
  buffer.resize(10);
  EXPECT_CALL(buffer, do_grow(11));
  buffer.push_back(22);
  EXPECT_EQ(22, buffer[10]);
  EXPECT_EQ(11u, buffer.size());
}

TEST(BufferTest, Append) {
  char data[15];
  MockBuffer<char> buffer(data, 10);
  const char *test = "test";
  buffer.append(test, test + 5);
  EXPECT_STREQ(test, &buffer[0]);
  EXPECT_EQ(5u, buffer.size());
  buffer.resize(10);
  EXPECT_CALL(buffer, do_grow(12));
  buffer.append(test, test + 2);
  EXPECT_EQ('t', buffer[10]);
  EXPECT_EQ('e', buffer[11]);
  EXPECT_EQ(12u, buffer.size());
}

TEST(BufferTest, AppendAllocatesEnoughStorage) {
  char data[19];
  MockBuffer<char> buffer(data, 10);
  const char *test = "abcdefgh";
  buffer.resize(10);
  EXPECT_CALL(buffer, do_grow(19));
  buffer.append(test, test + 9);
}

TEST(MemoryBufferTest, Ctor) {
  MemoryBuffer<char, 123> buffer;
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
}

#if FMT_USE_RVALUE_REFERENCES

typedef AllocatorRef< std::allocator<char> > TestAllocator;

void check_move_buffer(const char *str,
                       MemoryBuffer<char, 5, TestAllocator> &buffer) {
  std::allocator<char> *alloc = buffer.get_allocator().get();
  MemoryBuffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
  // Move should transfer allocator.
  EXPECT_EQ(0, buffer.get_allocator().get());
  EXPECT_EQ(alloc, buffer2.get_allocator().get());
}

TEST(MemoryBufferTest, MoveCtor) {
  std::allocator<char> alloc;
  MemoryBuffer<char, 5, TestAllocator> buffer((TestAllocator(&alloc)));
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
  MemoryBuffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testab", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 5u);
}

void check_move_assign_buffer(const char *str, MemoryBuffer<char, 5> &buffer) {
  MemoryBuffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
}

TEST(MemoryBufferTest, MoveAssignment) {
  MemoryBuffer<char, 5> buffer;
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
  MemoryBuffer<char, 5> buffer2;
  buffer2 = std::move(buffer);
  // Move should rip the guts of the first buffer.
  EXPECT_EQ(inline_buffer_ptr, &buffer[0]);
  EXPECT_EQ("testab", std::string(&buffer2[0], buffer2.size()));
  EXPECT_GT(buffer2.capacity(), 5u);
}

#endif  // FMT_USE_RVALUE_REFERENCES

TEST(MemoryBufferTest, Grow) {
  typedef AllocatorRef< MockAllocator<int> > Allocator;
  typedef MemoryBuffer<int, 10, Allocator> Base;
  MockAllocator<int> alloc;
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
  typedef AllocatorRef< MockAllocator<char> > TestAllocator;
  MemoryBuffer<char, 10, TestAllocator> buffer;
  EXPECT_EQ(0, buffer.get_allocator().get());
  StrictMock< MockAllocator<char> > alloc;
  char mem;
  {
    MemoryBuffer<char, 10, TestAllocator> buffer2((TestAllocator(&alloc)));
    EXPECT_EQ(&alloc, buffer2.get_allocator().get());
    std::size_t size = 2 * fmt::internal::INLINE_BUFFER_SIZE;
    EXPECT_CALL(alloc, allocate(size)).WillOnce(Return(&mem));
    buffer2.reserve(size);
    EXPECT_CALL(alloc, deallocate(&mem, size));
  }
}

TEST(MemoryBufferTest, ExceptionInDeallocate) {
  typedef AllocatorRef< MockAllocator<char> > TestAllocator;
  StrictMock< MockAllocator<char> > alloc;
  MemoryBuffer<char, 10, TestAllocator> buffer((TestAllocator(&alloc)));
  std::size_t size = 2 * fmt::internal::INLINE_BUFFER_SIZE;
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

TEST(UtilTest, FormatArgs) {
  fmt::format_args args;
  EXPECT_FALSE(args[1]);
}

struct CustomContext {
  typedef char char_type;
  bool called;
};

void format_value(fmt::writer &, const Test &, CustomContext &ctx) {
  ctx.called = true;
}

TEST(UtilTest, MakeValueWithCustomFormatter) {
  ::Test t;
  fmt::internal::value<CustomContext> arg(t);
  CustomContext ctx = {false};
  fmt::MemoryWriter w;
  arg.custom.format(w, &t, &ctx);
  EXPECT_TRUE(ctx.called);
}

namespace fmt {
namespace internal {

template <typename Char>
bool operator==(CustomValue<Char> lhs, CustomValue<Char> rhs) {
  return lhs.value == rhs.value;
}
}
}

template <typename T>
struct MockVisitor {
  // Use a unique result type to make sure that there are no undesirable
  // conversions.
  struct Result {};

  MockVisitor() {
    ON_CALL(*this, visit(_)).WillByDefault(Return(Result()));
  }

  MOCK_METHOD1_T(visit, Result (T value));
  MOCK_METHOD0_T(unexpected, void ());

  Result operator()(T value) { return visit(value); }

  template <typename U>
  Result operator()(U value) {
    unexpected();
    return Result();
  }
};

template <typename T>
struct VisitType { typedef T Type; };

#define VISIT_TYPE(Type_, VisitType_) \
  template <> \
  struct VisitType<Type_> { typedef VisitType_ Type; }

VISIT_TYPE(signed char, int);
VISIT_TYPE(unsigned char, unsigned);
VISIT_TYPE(short, int);
VISIT_TYPE(unsigned short, unsigned);

#if LONG_MAX == INT_MAX
VISIT_TYPE(long, int);
VISIT_TYPE(unsigned long, unsigned);
#else
VISIT_TYPE(long, fmt::LongLong);
VISIT_TYPE(unsigned long, fmt::ULongLong);
#endif

VISIT_TYPE(float, double);

#define CHECK_ARG_(Char, expected, value) { \
  testing::StrictMock<MockVisitor<decltype(expected)>> visitor; \
  EXPECT_CALL(visitor, visit(expected)); \
  fmt::visit(visitor, make_arg<fmt::basic_format_context<Char>>(value)); \
}

#define CHECK_ARG(value) { \
  typename VisitType<decltype(value)>::Type expected = value; \
  CHECK_ARG_(char, expected, value) \
  CHECK_ARG_(wchar_t, expected, value) \
}

template <typename T>
class NumericArgTest : public testing::Test {};

typedef ::testing::Types<
  bool, signed char, unsigned char, signed, unsigned short,
  int, unsigned, long, unsigned long, fmt::LongLong, fmt::ULongLong,
  float, double, long double> Types;
TYPED_TEST_CASE(NumericArgTest, Types);

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type test_value() {
  return static_cast<T>(42);
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type
    test_value() {
  return static_cast<T>(4.2);
}

TYPED_TEST(NumericArgTest, MakeAndVisit) {
  CHECK_ARG(test_value<TypeParam>());
  CHECK_ARG(std::numeric_limits<TypeParam>::min());
  CHECK_ARG(std::numeric_limits<TypeParam>::max());
}

TEST(UtilTest, CharArg) {
  CHECK_ARG_(char, 'a', 'a');
  CHECK_ARG_(wchar_t, L'a', 'a');
  CHECK_ARG_(wchar_t, L'a', L'a');
}

TEST(UtilTest, StringArg) {
  char str_data[] = "test";
  char *str = str_data;
  const char *cstr = str;
  CHECK_ARG_(char, cstr, str);
  CHECK_ARG_(wchar_t, cstr, str);
  CHECK_ARG(cstr);

  StringRef sref(str);
  CHECK_ARG_(char, sref, std::string(str));
  CHECK_ARG_(wchar_t, sref, std::string(str));
  CHECK_ARG(sref);
}

TEST(UtilTest, WStringArg) {
  wchar_t str_data[] = L"test";
  wchar_t *str = str_data;
  const wchar_t *cstr = str;

  fmt::WStringRef sref(str);
  CHECK_ARG_(wchar_t, sref, str);
  CHECK_ARG_(wchar_t, sref, cstr);
  CHECK_ARG_(wchar_t, sref, std::wstring(str));
  CHECK_ARG_(wchar_t, sref, fmt::WStringRef(str));
}

TEST(UtilTest, PointerArg) {
  void *p = 0;
  const void *cp = 0;
  CHECK_ARG_(char, cp, p);
  CHECK_ARG_(wchar_t, cp, p);
  CHECK_ARG(cp);
}

TEST(UtilTest, CustomArg) {
  ::Test test;
  typedef MockVisitor<fmt::internal::CustomValue<char>> Visitor;
  testing::StrictMock<Visitor> visitor;
  EXPECT_CALL(visitor, visit(_)).WillOnce(
        testing::Invoke([&](fmt::internal::CustomValue<char> custom) {
    EXPECT_EQ(&test, custom.value);
    fmt::MemoryWriter w;
    fmt::format_context ctx("}", fmt::format_args());
    custom.format(w, &test, &ctx);
    EXPECT_EQ("test", w.str());
    return Visitor::Result();
  }));
  fmt::visit(visitor, make_arg<fmt::format_context>(test));
}

TEST(ArgVisitorTest, VisitInvalidArg) {
  typedef MockVisitor<fmt::monostate> Visitor;
  testing::StrictMock<Visitor> visitor;
  EXPECT_CALL(visitor, visit(_));
  format_arg arg;
  visit(visitor, arg);
}

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

TEST(UtilTest, StringRef) {
  // Test that StringRef::size() returns string length, not buffer size.
  char str[100] = "some string";
  EXPECT_EQ(std::strlen(str), StringRef(str).size());
  EXPECT_LT(std::strlen(str), sizeof(str));
}

// Check StringRef's comparison operator.
template <template <typename> class Op>
void CheckOp() {
  const char *inputs[] = {"foo", "fop", "fo"};
  std::size_t num_inputs = sizeof(inputs) / sizeof(*inputs);
  for (std::size_t i = 0; i < num_inputs; ++i) {
    for (std::size_t j = 0; j < num_inputs; ++j) {
      StringRef lhs(inputs[i]), rhs(inputs[j]);
      EXPECT_EQ(Op<int>()(lhs.compare(rhs), 0), Op<StringRef>()(lhs, rhs));
    }
  }
}

TEST(UtilTest, StringRefCompare) {
  EXPECT_EQ(0, StringRef("foo").compare(StringRef("foo")));
  EXPECT_GT(StringRef("fop").compare(StringRef("foo")), 0);
  EXPECT_LT(StringRef("foo").compare(StringRef("fop")), 0);
  EXPECT_GT(StringRef("foo").compare(StringRef("fo")), 0);
  EXPECT_LT(StringRef("fo").compare(StringRef("foo")), 0);
  CheckOp<std::equal_to>();
  CheckOp<std::not_equal_to>();
  CheckOp<std::less>();
  CheckOp<std::less_equal>();
  CheckOp<std::greater>();
  CheckOp<std::greater_equal>();
}

TEST(UtilTest, CountDigits) {
  test_count_digits<uint32_t>();
  test_count_digits<uint64_t>();
}

#ifdef _WIN32
TEST(UtilTest, UTF16ToUTF8) {
  std::string s = "ёжик";
  fmt::internal::UTF16ToUTF8 u(L"\x0451\x0436\x0438\x043A");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, UTF8ToUTF16) {
  std::string s = "лошадка";
  fmt::internal::UTF8ToUTF16 u(s.c_str());
  EXPECT_EQ(L"\x043B\x043E\x0448\x0430\x0434\x043A\x0430", u.str());
  EXPECT_EQ(7, u.size());
}

template <typename Converter, typename Char>
void check_utf_conversion_error(
        const char *message,
        fmt::BasicStringRef<Char> str = fmt::BasicStringRef<Char>(0, 0)) {
  fmt::MemoryWriter out;
  fmt::internal::format_windows_error(out, ERROR_INVALID_PARAMETER, message);
  fmt::SystemError error(0, "");
  try {
    (Converter)(str);
  } catch (const fmt::SystemError &e) {
    error = e;
  }
  EXPECT_EQ(ERROR_INVALID_PARAMETER, error.error_code());
  EXPECT_EQ(out.str(), error.what());
}

TEST(UtilTest, UTF16ToUTF8Error) {
  check_utf_conversion_error<fmt::internal::UTF16ToUTF8, wchar_t>(
      "cannot convert string from UTF-16 to UTF-8");
}

TEST(UtilTest, UTF8ToUTF16Error) {
  const char *message = "cannot convert string from UTF-8 to UTF-16";
  check_utf_conversion_error<fmt::internal::UTF8ToUTF16, char>(message);
  check_utf_conversion_error<fmt::internal::UTF8ToUTF16, char>(
    message, fmt::StringRef("foo", INT_MAX + 1u));
}

TEST(UtilTest, UTF16ToUTF8Convert) {
  fmt::internal::UTF16ToUTF8 u;
  EXPECT_EQ(ERROR_INVALID_PARAMETER, u.convert(fmt::WStringRef(0, 0)));
  EXPECT_EQ(ERROR_INVALID_PARAMETER,
            u.convert(fmt::WStringRef(L"foo", INT_MAX + 1u)));
}
#endif  // _WIN32

typedef void (*FormatErrorMessage)(
        fmt::writer &out, int error_code, StringRef message);

template <typename Error>
void check_throw_error(int error_code, FormatErrorMessage format) {
  fmt::SystemError error(0, "");
  try {
    throw Error(error_code, "test {}", "error");
  } catch (const fmt::SystemError &e) {
    error = e;
  }
  fmt::MemoryWriter message;
  format(message, error_code, "test error");
  EXPECT_EQ(message.str(), error.what());
  EXPECT_EQ(error_code, error.error_code());
}

TEST(UtilTest, FormatSystemError) {
  fmt::MemoryWriter message;
  fmt::format_system_error(message, EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)), message.str());
  message.clear();
  fmt::format_system_error(
        message, EDOM, fmt::StringRef(0, std::numeric_limits<size_t>::max()));
  EXPECT_EQ(fmt::format("error {}", EDOM), message.str());
}

TEST(UtilTest, SystemError) {
  fmt::SystemError e(EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", get_system_error(EDOM)), e.what());
  EXPECT_EQ(EDOM, e.error_code());
  check_throw_error<fmt::SystemError>(EDOM, fmt::format_system_error);
}

TEST(UtilTest, ReportSystemError) {
  fmt::MemoryWriter out;
  fmt::format_system_error(out, EDOM, "test error");
  out << '\n';
  EXPECT_WRITE(stderr, fmt::report_system_error(EDOM, "test error"), out.str());
}

#ifdef _WIN32

TEST(UtilTest, FormatWindowsError) {
  LPWSTR message = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      ERROR_FILE_EXISTS, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, 0);
  fmt::internal::UTF16ToUTF8 utf8_message(message);
  LocalFree(message);
  fmt::MemoryWriter actual_message;
  fmt::internal::format_windows_error(
      actual_message, ERROR_FILE_EXISTS, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
      actual_message.str());
  actual_message.clear();
  fmt::internal::format_windows_error(
        actual_message, ERROR_FILE_EXISTS,
        fmt::StringRef(0, std::numeric_limits<size_t>::max()));
  EXPECT_EQ(fmt::format("error {}", ERROR_FILE_EXISTS), actual_message.str());
}

TEST(UtilTest, FormatLongWindowsError) {
  LPWSTR message = 0;
  // this error code is not available on all Windows platforms and
  // Windows SDKs, so do not fail the test if the error string cannot
  // be retrieved.
  const int provisioning_not_allowed = 0x80284013L /*TBS_E_PROVISIONING_NOT_ALLOWED*/;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      provisioning_not_allowed, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, 0) == 0) {
    return;
  }
  fmt::internal::UTF16ToUTF8 utf8_message(message);
  LocalFree(message);
  fmt::MemoryWriter actual_message;
  fmt::internal::format_windows_error(
      actual_message, provisioning_not_allowed, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
      actual_message.str());
}

TEST(UtilTest, WindowsError) {
  check_throw_error<fmt::WindowsError>(
      ERROR_FILE_EXISTS, fmt::internal::format_windows_error);
}

TEST(UtilTest, ReportWindowsError) {
  fmt::MemoryWriter out;
  fmt::internal::format_windows_error(out, ERROR_FILE_EXISTS, "test error");
  out << '\n';
  EXPECT_WRITE(stderr,
      fmt::report_windows_error(ERROR_FILE_EXISTS, "test error"), out.str());
}

#endif  // _WIN32

enum TestEnum2 {};

TEST(UtilTest, ConvertToInt) {
  EXPECT_TRUE(fmt::internal::ConvertToInt<char>::enable_conversion);
  EXPECT_FALSE(fmt::internal::ConvertToInt<const char *>::enable_conversion);
  EXPECT_TRUE(fmt::internal::ConvertToInt<TestEnum2>::value);
}

#if FMT_USE_ENUM_BASE
enum TestEnum : char {TestValue};
TEST(UtilTest, IsEnumConvertibleToInt) {
  EXPECT_TRUE(fmt::internal::ConvertToInt<TestEnum>::enable_conversion);
}
#endif

template <typename T>
bool check_enable_if(
    typename fmt::internal::EnableIf<sizeof(T) == sizeof(int), T>::type *) {
  return true;
}

template <typename T>
bool check_enable_if(
    typename fmt::internal::EnableIf<sizeof(T) != sizeof(int), T>::type *) {
  return false;
}

TEST(UtilTest, EnableIf) {
  int i = 0;
  EXPECT_TRUE(check_enable_if<int>(&i));
  char c = 0;
  EXPECT_FALSE(check_enable_if<char>(&c));
}

TEST(UtilTest, Conditional) {
  int i = 0;
  fmt::internal::Conditional<true, int, char>::type *pi = &i;
  (void)pi;
  char c = 0;
  fmt::internal::Conditional<false, int, char>::type *pc = &c;
  (void)pc;
}

struct TestLConv {
  char *thousands_sep;
};

struct EmptyLConv {};

TEST(UtilTest, ThousandsSep) {
  char foo[] = "foo";
  TestLConv lc = {foo};
  EXPECT_EQ("foo", fmt::internal::thousands_sep(&lc).to_string());
  EmptyLConv empty_lc;
  EXPECT_EQ("", fmt::internal::thousands_sep(&empty_lc));
}
