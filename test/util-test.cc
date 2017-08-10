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

#undef max

using fmt::StringRef;
using fmt::internal::Arg;
using fmt::Buffer;
using fmt::internal::MemoryBuffer;

using testing::Return;
using testing::StrictMock;

namespace {

struct Test {};

template <typename Char>
void format_arg(fmt::BasicFormatter<Char> &f, const Char *, Test) {
  f.writer() << "test";
}

template <typename Char, typename T>
Arg make_arg(const T &value) {
  typedef fmt::internal::MakeValue< fmt::BasicFormatter<Char> > MakeValue;
  Arg arg = MakeValue(value);
  arg.type = static_cast<Arg::Type>(MakeValue::type(value));
  return arg;
}
}  // namespace

void CheckForwarding(
    MockAllocator<int> &alloc, AllocatorRef< MockAllocator<int> > &ref) {
  int mem;
  // Check if value_type is properly defined.
  AllocatorRef< MockAllocator<int> >::value_type *ptr = &mem;
  // Check forwarding.
  EXPECT_CALL(alloc, allocate(42, 0)).WillOnce(Return(ptr));
  ref.allocate(42, 0);
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
  EXPECT_CALL(alloc, allocate(20, 0)).WillOnce(Return(mem));
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
    EXPECT_CALL(alloc, allocate(size, 0)).WillOnce(Return(&mem));
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
    EXPECT_CALL(alloc, allocate(size, 0)).WillOnce(Return(&mem[0]));
    buffer.resize(size);
    std::fill(&buffer[0], &buffer[0] + size, 'x');
  }
  std::vector<char> mem2(2 * size);
  {
    EXPECT_CALL(alloc, allocate(2 * size, 0)).WillOnce(Return(&mem2[0]));
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

template <Arg::Type>
struct ArgInfo;

#define ARG_INFO(type_code, Type, field) \
  template <> \
  struct ArgInfo<Arg::type_code> { \
    static Type get(const Arg &arg) { return arg.field; } \
  }

ARG_INFO(INT, int, int_value);
ARG_INFO(UINT, unsigned, uint_value);
ARG_INFO(LONG_LONG, fmt::LongLong, long_long_value);
ARG_INFO(ULONG_LONG, fmt::ULongLong, ulong_long_value);
ARG_INFO(BOOL, int, int_value);
ARG_INFO(CHAR, int, int_value);
ARG_INFO(DOUBLE, double, double_value);
ARG_INFO(LONG_DOUBLE, long double, long_double_value);
ARG_INFO(CSTRING, const char *, string.value);
ARG_INFO(STRING, const char *, string.value);
ARG_INFO(WSTRING, const wchar_t *, wstring.value);
ARG_INFO(POINTER, const void *, pointer);
ARG_INFO(CUSTOM, Arg::CustomValue, custom);

#define CHECK_ARG_INFO(Type, field, value) { \
  Arg arg = Arg(); \
  arg.field = value; \
  EXPECT_EQ(value, ArgInfo<Arg::Type>::get(arg)); \
}

TEST(ArgTest, ArgInfo) {
  CHECK_ARG_INFO(INT, int_value, 42);
  CHECK_ARG_INFO(UINT, uint_value, 42u);
  CHECK_ARG_INFO(LONG_LONG, long_long_value, 42);
  CHECK_ARG_INFO(ULONG_LONG, ulong_long_value, 42u);
  CHECK_ARG_INFO(DOUBLE, double_value, 4.2);
  CHECK_ARG_INFO(LONG_DOUBLE, long_double_value, 4.2);
  CHECK_ARG_INFO(CHAR, int_value, 'x');
  const char STR[] = "abc";
  CHECK_ARG_INFO(CSTRING, string.value, STR);
  const wchar_t WSTR[] = L"abc";
  CHECK_ARG_INFO(WSTRING, wstring.value, WSTR);
  int p = 0;
  CHECK_ARG_INFO(POINTER, pointer, &p);
  Arg arg = Arg();
  arg.custom.value = &p;
  EXPECT_EQ(&p, ArgInfo<Arg::CUSTOM>::get(arg).value);
}

#define EXPECT_ARG_(Char, type_code, MakeArgType, ExpectedType, value) { \
  MakeArgType input = static_cast<MakeArgType>(value); \
  Arg arg = make_arg<Char>(input); \
  EXPECT_EQ(Arg::type_code, arg.type); \
  ExpectedType expected_value = static_cast<ExpectedType>(value); \
  EXPECT_EQ(expected_value, ArgInfo<Arg::type_code>::get(arg)); \
}

#define EXPECT_ARG(type_code, Type, value) \
  EXPECT_ARG_(char, type_code, Type, Type, value)

#define EXPECT_ARGW(type_code, Type, value) \
  EXPECT_ARG_(wchar_t, type_code, Type, Type, value)

TEST(ArgTest, MakeArg) {
  // Test bool.
  EXPECT_ARG_(char, BOOL, bool, int, true);
  EXPECT_ARG_(wchar_t, BOOL, bool, int, true);

  // Test char.
  EXPECT_ARG(CHAR, char, 'a');
  EXPECT_ARG(CHAR, char, CHAR_MIN);
  EXPECT_ARG(CHAR, char, CHAR_MAX);

  // Test wchar_t.
  EXPECT_ARGW(CHAR, wchar_t, L'a');
  EXPECT_ARGW(CHAR, wchar_t, WCHAR_MIN);
  EXPECT_ARGW(CHAR, wchar_t, WCHAR_MAX);

  // Test signed/unsigned char.
  EXPECT_ARG(INT, signed char, 42);
  EXPECT_ARG(INT, signed char, SCHAR_MIN);
  EXPECT_ARG(INT, signed char, SCHAR_MAX);
  EXPECT_ARG(UINT, unsigned char, 42);
  EXPECT_ARG(UINT, unsigned char, UCHAR_MAX );

  // Test short.
  EXPECT_ARG(INT, short, 42);
  EXPECT_ARG(INT, short, SHRT_MIN);
  EXPECT_ARG(INT, short, SHRT_MAX);
  EXPECT_ARG(UINT, unsigned short, 42);
  EXPECT_ARG(UINT, unsigned short, USHRT_MAX);

  // Test int.
  EXPECT_ARG(INT, int, 42);
  EXPECT_ARG(INT, int, INT_MIN);
  EXPECT_ARG(INT, int, INT_MAX);
  EXPECT_ARG(UINT, unsigned, 42);
  EXPECT_ARG(UINT, unsigned, UINT_MAX);

  // Test long.
#if LONG_MAX == INT_MAX
# define LONG INT
# define ULONG UINT
# define long_value int_value
# define ulong_value uint_value
#else
# define LONG LONG_LONG
# define ULONG ULONG_LONG
# define long_value long_long_value
# define ulong_value ulong_long_value
#endif
  EXPECT_ARG(LONG, long, 42);
  EXPECT_ARG(LONG, long, LONG_MIN);
  EXPECT_ARG(LONG, long, LONG_MAX);
  EXPECT_ARG(ULONG, unsigned long, 42);
  EXPECT_ARG(ULONG, unsigned long, ULONG_MAX);

  // Test long long.
  EXPECT_ARG(LONG_LONG, fmt::LongLong, 42);
  EXPECT_ARG(LONG_LONG, fmt::LongLong, LLONG_MIN);
  EXPECT_ARG(LONG_LONG, fmt::LongLong, LLONG_MAX);
  EXPECT_ARG(ULONG_LONG, fmt::ULongLong, 42);
  EXPECT_ARG(ULONG_LONG, fmt::ULongLong, ULLONG_MAX);

  // Test float.
  EXPECT_ARG(DOUBLE, float, 4.2);
  EXPECT_ARG(DOUBLE, float, FLT_MIN);
  EXPECT_ARG(DOUBLE, float, FLT_MAX);

  // Test double.
  EXPECT_ARG(DOUBLE, double, 4.2);
  EXPECT_ARG(DOUBLE, double, DBL_MIN);
  EXPECT_ARG(DOUBLE, double, DBL_MAX);

  // Test long double.
  EXPECT_ARG(LONG_DOUBLE, long double, 4.2);
  EXPECT_ARG(LONG_DOUBLE, long double, LDBL_MIN);
  EXPECT_ARG(LONG_DOUBLE, long double, LDBL_MAX);

  // Test string.
  char STR[] = "test";
  EXPECT_ARG(CSTRING, char*, STR);
  EXPECT_ARG(CSTRING, const char*, STR);
  EXPECT_ARG(STRING, std::string, STR);
  EXPECT_ARG(STRING, fmt::StringRef, STR);

  // Test wide string.
  wchar_t WSTR[] = L"test";
  EXPECT_ARGW(WSTRING, wchar_t*, WSTR);
  EXPECT_ARGW(WSTRING, const wchar_t*, WSTR);
  EXPECT_ARGW(WSTRING, std::wstring, WSTR);
  EXPECT_ARGW(WSTRING, fmt::WStringRef, WSTR);

  int n = 42;
  EXPECT_ARG(POINTER, void*, &n);
  EXPECT_ARG(POINTER, const void*, &n);

  ::Test t;
  Arg arg = make_arg<char>(t);
  EXPECT_EQ(fmt::internal::Arg::CUSTOM, arg.type);
  EXPECT_EQ(&t, arg.custom.value);
  fmt::MemoryWriter w;
  fmt::BasicFormatter<char> formatter(fmt::ArgList(), w);
  const char *s = "}";
  arg.custom.format(&formatter, &t, &s);
  EXPECT_EQ("test", w.str());
}

TEST(UtilTest, ArgList) {
  fmt::ArgList args;
  EXPECT_EQ(Arg::NONE, args[1].type);
}

struct CustomFormatter {
  typedef char Char;
};

void format_arg(CustomFormatter &, const char *&s, const Test &) {
  s = "custom_format";
}

TEST(UtilTest, MakeValueWithCustomFormatter) {
  ::Test t;
  Arg arg = fmt::internal::MakeValue<CustomFormatter>(t);
  CustomFormatter formatter;
  const char *s = "";
  arg.custom.format(&formatter, &t, &s);
  EXPECT_STREQ("custom_format", s);
}

struct Result {
  Arg arg;

  Result() : arg(make_arg<char>(0xdeadbeef)) {}

  template <typename T>
  Result(const T& value) : arg(make_arg<char>(value)) {}
  Result(const wchar_t *s) : arg(make_arg<wchar_t>(s)) {}
};

struct TestVisitor : fmt::ArgVisitor<TestVisitor, Result> {
  Result visit_int(int value) { return value; }
  Result visit_uint(unsigned value) { return value; }
  Result visit_long_long(fmt::LongLong value) { return value; }
  Result visit_ulong_long(fmt::ULongLong value) { return value; }
  Result visit_double(double value) { return value; }
  Result visit_long_double(long double value) { return value; }
  Result visit_char(int value) { return static_cast<char>(value); }
  Result visit_cstring(const char *s) { return s; }
  Result visit_string(fmt::internal::Arg::StringValue<char> s) {
    return s.value;
  }
  Result visit_wstring(fmt::internal::Arg::StringValue<wchar_t> s) {
    return s.value;
  }
  Result visit_pointer(const void *p) { return p; }
  Result visit_custom(fmt::internal::Arg::CustomValue c) {
    return *static_cast<const ::Test*>(c.value);
  }
};

#define EXPECT_RESULT_(Char, type_code, value) { \
  Arg arg = make_arg<Char>(value); \
  Result result = TestVisitor().visit(arg); \
  EXPECT_EQ(Arg::type_code, result.arg.type); \
  EXPECT_EQ(value, ArgInfo<Arg::type_code>::get(result.arg)); \
}

#define EXPECT_RESULT(type_code, value) \
  EXPECT_RESULT_(char, type_code, value)
#define EXPECT_RESULTW(type_code, value) \
  EXPECT_RESULT_(wchar_t, type_code, value)

TEST(ArgVisitorTest, VisitAll) {
  EXPECT_RESULT(INT, 42);
  EXPECT_RESULT(UINT, 42u);
  EXPECT_RESULT(LONG_LONG, 42ll);
  EXPECT_RESULT(ULONG_LONG, 42ull);
  EXPECT_RESULT(DOUBLE, 4.2);
  EXPECT_RESULT(LONG_DOUBLE, 4.2l);
  EXPECT_RESULT(CHAR, 'x');
  const char STR[] = "abc";
  EXPECT_RESULT(CSTRING, STR);
  const wchar_t WSTR[] = L"abc";
  EXPECT_RESULTW(WSTRING, WSTR);
  const void *p = STR;
  EXPECT_RESULT(POINTER, p);
  ::Test t;
  Result result = TestVisitor().visit(make_arg<char>(t));
  EXPECT_EQ(Arg::CUSTOM, result.arg.type);
  EXPECT_EQ(&t, result.arg.custom.value);
}

struct TestAnyVisitor : fmt::ArgVisitor<TestAnyVisitor, Result> {
  template <typename T>
  Result visit_any_int(T value) { return value; }

  template <typename T>
  Result visit_any_double(T value) { return value; }
};

#undef EXPECT_RESULT
#define EXPECT_RESULT(type_code, value) { \
  Result result = TestAnyVisitor().visit(make_arg<char>(value)); \
  EXPECT_EQ(Arg::type_code, result.arg.type); \
  EXPECT_EQ(value, ArgInfo<Arg::type_code>::get(result.arg)); \
}

TEST(ArgVisitorTest, VisitAny) {
  EXPECT_RESULT(INT, 42);
  EXPECT_RESULT(UINT, 42u);
  EXPECT_RESULT(LONG_LONG, 42ll);
  EXPECT_RESULT(ULONG_LONG, 42ull);
  EXPECT_RESULT(DOUBLE, 4.2);
  EXPECT_RESULT(LONG_DOUBLE, 4.2l);
}

struct TestUnhandledVisitor :
    fmt::ArgVisitor<TestUnhandledVisitor, const char *> {
  const char *visit_unhandled_arg() { return "test"; }
};

#define EXPECT_UNHANDLED(value) \
  EXPECT_STREQ("test", TestUnhandledVisitor().visit(make_arg<wchar_t>(value)));

TEST(ArgVisitorTest, VisitUnhandledArg) {
  EXPECT_UNHANDLED(42);
  EXPECT_UNHANDLED(42u);
  EXPECT_UNHANDLED(42ll);
  EXPECT_UNHANDLED(42ull);
  EXPECT_UNHANDLED(4.2);
  EXPECT_UNHANDLED(4.2l);
  EXPECT_UNHANDLED('x');
  const char STR[] = "abc";
  EXPECT_UNHANDLED(STR);
  const wchar_t WSTR[] = L"abc";
  EXPECT_UNHANDLED(WSTR);
  const void *p = STR;
  EXPECT_UNHANDLED(p);
  EXPECT_UNHANDLED(::Test());
}

TEST(ArgVisitorTest, VisitInvalidArg) {
  Arg arg = Arg();
  arg.type = static_cast<Arg::Type>(Arg::NONE);
  EXPECT_ASSERT(TestVisitor().visit(arg), "invalid argument type");
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
        fmt::Writer &out, int error_code, StringRef message);

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

  // Check if std::allocator throws on allocating max size_t / 2 chars.
  size_t max_size = std::numeric_limits<size_t>::max() / 2;
  bool throws_on_alloc = false;
  try {
    std::allocator<char> alloc;
    alloc.deallocate(alloc.allocate(max_size), max_size);
  } catch (std::bad_alloc) {
    throws_on_alloc = true;
  }
  if (!throws_on_alloc) {
    fmt::print("warning: std::allocator allocates {} chars", max_size);
    return;
  }
  fmt::format_system_error(message, EDOM, fmt::StringRef(0, max_size));
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
