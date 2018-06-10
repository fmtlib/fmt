// Formatting library for C++ - utility tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "test-assert.h"

#include <cfloat>
#include <climits>
#include <cstring>
#include <functional>
#include <limits>

#if FMT_USE_TYPE_TRAITS
# include <type_traits>
#endif

#include "gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

// Check if format.h compiles with windows.h included.
#ifdef _WIN32
# include <windows.h>
#endif

#include "fmt/core.h"

#undef min
#undef max

using fmt::basic_format_arg;
using fmt::internal::basic_buffer;
using fmt::basic_memory_buffer;
using fmt::string_view;
using fmt::internal::fp;
using fmt::internal::value;

using testing::_;
using testing::Return;
using testing::StrictMock;

namespace {

struct Test {};

template <typename Context, typename T>
basic_format_arg<Context> make_arg(const T &value) {
  return fmt::internal::make_arg<Context>(value);
}
}  // namespace

FMT_BEGIN_NAMESPACE
template <typename Char>
struct formatter<Test, Char> {
  template <typename ParseContext>
  auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  typedef std::back_insert_iterator<basic_buffer<Char>> iterator;

  auto format(Test, basic_format_context<iterator, char> &ctx)
      -> decltype(ctx.out()) {
    const Char *test = "test";
    return std::copy_n(test, std::strlen(test), ctx.out());
  }
};
FMT_END_NAMESPACE

static void CheckForwarding(
    MockAllocator<int> &alloc, AllocatorRef<MockAllocator<int>> &ref) {
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
  EXPECT_EQ(nullptr, ref3.get());
  ref3 = ref;
  CheckForwarding(alloc, ref3);
}

#if FMT_USE_TYPE_TRAITS
TEST(BufferTest, Noncopyable) {
  EXPECT_FALSE(std::is_copy_constructible<basic_buffer<char> >::value);
  EXPECT_FALSE(std::is_copy_assignable<basic_buffer<char> >::value);
}

TEST(BufferTest, Nonmoveable) {
  EXPECT_FALSE(std::is_move_constructible<basic_buffer<char> >::value);
  EXPECT_FALSE(std::is_move_assignable<basic_buffer<char> >::value);
}
#endif

// A test buffer with a dummy grow method.
template <typename T>
struct TestBuffer : basic_buffer<T> {
  void grow(std::size_t capacity) { this->set(nullptr, capacity); }
};

template <typename T>
struct MockBuffer : basic_buffer<T> {
  MOCK_METHOD1(do_grow, void (std::size_t capacity));

  void grow(std::size_t capacity) {
    this->set(this->data(), capacity);
    do_grow(capacity);
  }

  MockBuffer() {}
  MockBuffer(T *data) { this->set(data, 0); }
  MockBuffer(T *data, std::size_t capacity) { this->set(data, capacity); }
};

TEST(BufferTest, Ctor) {
  {
    MockBuffer<int> buffer;
    EXPECT_EQ(nullptr, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    MockBuffer<int> buffer(&dummy);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    std::size_t capacity = std::numeric_limits<std::size_t>::max();
    MockBuffer<int> buffer(&dummy, capacity);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
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
  basic_buffer<int> *buffer = mock_buffer;
  delete buffer;
}

TEST(BufferTest, Access) {
  char data[10];
  MockBuffer<char> buffer(data, sizeof(data));
  buffer[0] = 11;
  EXPECT_EQ(11, buffer[0]);
  buffer[3] = 42;
  EXPECT_EQ(42, *(&buffer[0] + 3));
  const basic_buffer<char> &const_buffer = buffer;
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
  buffer.resize(0);
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(20u, buffer.capacity());
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
  basic_memory_buffer<char, 123> buffer;
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
}

#if FMT_USE_RVALUE_REFERENCES

typedef AllocatorRef< std::allocator<char> > TestAllocator;

static void check_move_buffer(const char *str,
                       basic_memory_buffer<char, 5, TestAllocator> &buffer) {
  std::allocator<char> *alloc = buffer.get_allocator().get();
  basic_memory_buffer<char, 5, TestAllocator> buffer2(std::move(buffer));
  // Move shouldn't destroy the inline content of the first buffer.
  EXPECT_EQ(str, std::string(&buffer[0], buffer.size()));
  EXPECT_EQ(str, std::string(&buffer2[0], buffer2.size()));
  EXPECT_EQ(5u, buffer2.capacity());
  // Move should transfer allocator.
  EXPECT_EQ(nullptr, buffer.get_allocator().get());
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

#endif  // FMT_USE_RVALUE_REFERENCES

TEST(MemoryBufferTest, Grow) {
  typedef AllocatorRef< MockAllocator<int> > Allocator;
  typedef basic_memory_buffer<int, 10, Allocator> Base;
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
  basic_memory_buffer<char, 10, TestAllocator> buffer;
  EXPECT_EQ(nullptr, buffer.get_allocator().get());
  StrictMock< MockAllocator<char> > alloc;
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
  typedef AllocatorRef< MockAllocator<char> > TestAllocator;
  StrictMock< MockAllocator<char> > alloc;
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

TEST(FixedBufferTest, Ctor) {
  char array[10] = "garbage";
  fmt::basic_fixed_buffer<char> buffer(array, sizeof(array));
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(10u, buffer.capacity());
  EXPECT_EQ(array, buffer.data());
}

TEST(FixedBufferTest, CompileTimeSizeCtor) {
  char array[10] = "garbage";
  fmt::basic_fixed_buffer<char> buffer(array);
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(10u, buffer.capacity());
  EXPECT_EQ(array, buffer.data());
}

TEST(FixedBufferTest, BufferOverflow) {
  char array[10];
  fmt::basic_fixed_buffer<char> buffer(array);
  buffer.resize(10);
  EXPECT_THROW_MSG(buffer.resize(11), std::runtime_error, "buffer overflow");
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

TEST(UtilTest, FormatArgs) {
  fmt::format_args args;
  EXPECT_FALSE(args.get(1));
}

struct custom_context {
  typedef char char_type;

  template <typename T>
  struct formatter_type {
    struct type {
      template <typename ParseContext>
      auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
      }

      const char *format(const T &, custom_context& ctx) {
        ctx.called = true;
        return nullptr;
      }
    };
  };

  bool called;

  fmt::parse_context parse_context() { return fmt::parse_context(""); }
  void advance_to(const char *) {}
};

TEST(UtilTest, MakeValueWithCustomFormatter) {
  ::Test t;
  fmt::internal::value<custom_context> arg =
    fmt::internal::make_value<custom_context>(t);
  custom_context ctx = {false};
  arg.custom.format(&t, ctx);
  EXPECT_TRUE(ctx.called);
}

FMT_BEGIN_NAMESPACE
namespace internal {

template <typename Char>
bool operator==(custom_value<Char> lhs, custom_value<Char> rhs) {
  return lhs.value == rhs.value;
}
}
FMT_END_NAMESPACE

// Use a unique result type to make sure that there are no undesirable
// conversions.
struct Result {};

template <typename T>
struct MockVisitor: fmt::internal::function<Result> {
  MockVisitor() {
    ON_CALL(*this, visit(_)).WillByDefault(Return(Result()));
  }

  MOCK_METHOD1_T(visit, Result (T value));
  MOCK_METHOD0_T(unexpected, void ());

  Result operator()(T value) { return visit(value); }

  template <typename U>
  Result operator()(U) {
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
VISIT_TYPE(long, long long);
VISIT_TYPE(unsigned long, unsigned long long);
#endif

VISIT_TYPE(float, double);

#define CHECK_ARG_(Char, expected, value) { \
  testing::StrictMock<MockVisitor<decltype(expected)>> visitor; \
  EXPECT_CALL(visitor, visit(expected)); \
  typedef std::back_insert_iterator<basic_buffer<Char>> iterator; \
  fmt::visit(visitor, \
      make_arg<fmt::basic_format_context<iterator, Char>>(value)); \
}

#define CHECK_ARG(value, typename_) { \
  typedef decltype(value) value_type; \
  typename_ VisitType<value_type>::Type expected = value; \
  CHECK_ARG_(char, expected, value) \
  CHECK_ARG_(wchar_t, expected, value) \
}

template <typename T>
class NumericArgTest : public testing::Test {};

typedef ::testing::Types<
  bool, signed char, unsigned char, signed, unsigned short,
  int, unsigned, long, unsigned long, long long, unsigned long long,
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
  CHECK_ARG(test_value<TypeParam>(), typename);
  CHECK_ARG(std::numeric_limits<TypeParam>::min(), typename);
  CHECK_ARG(std::numeric_limits<TypeParam>::max(), typename);
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

  string_view sref(str);
  CHECK_ARG_(char, sref, std::string(str));
}

TEST(UtilTest, WStringArg) {
  wchar_t str_data[] = L"test";
  wchar_t *str = str_data;
  const wchar_t *cstr = str;

  fmt::wstring_view sref(str);
  CHECK_ARG_(wchar_t, cstr, str);
  CHECK_ARG_(wchar_t, cstr, cstr);
  CHECK_ARG_(wchar_t, sref, std::wstring(str));
  CHECK_ARG_(wchar_t, sref, fmt::wstring_view(str));
}

TEST(UtilTest, PointerArg) {
  void *p = nullptr;
  const void *cp = nullptr;
  CHECK_ARG_(char, cp, p);
  CHECK_ARG_(wchar_t, cp, p);
  CHECK_ARG(cp, );
}

struct check_custom {
  Result operator()(fmt::basic_format_arg<fmt::format_context>::handle h) const {
    fmt::memory_buffer buffer;
    fmt::internal::basic_buffer<char> &base = buffer;
    fmt::format_context ctx(std::back_inserter(base), "", fmt::format_args());
    h.format(ctx);
    EXPECT_EQ("test", std::string(buffer.data(), buffer.size()));
    return Result();
  }
};

TEST(UtilTest, CustomArg) {
  ::Test test;
  typedef MockVisitor<fmt::basic_format_arg<fmt::format_context>::handle>
    visitor;
  testing::StrictMock<visitor> v;
  EXPECT_CALL(v, visit(_)).WillOnce(testing::Invoke(check_custom()));
  fmt::visit(v, make_arg<fmt::format_context>(test));
}

TEST(ArgVisitorTest, VisitInvalidArg) {
  typedef MockVisitor<fmt::monostate> Visitor;
  testing::StrictMock<Visitor> visitor;
  EXPECT_CALL(visitor, visit(_));
  fmt::basic_format_arg<fmt::format_context> arg;
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
  EXPECT_EQ(std::strlen(str), string_view(str).size());
  EXPECT_LT(std::strlen(str), sizeof(str));
}

// Check StringRef's comparison operator.
template <template <typename> class Op>
void CheckOp() {
  const char *inputs[] = {"foo", "fop", "fo"};
  std::size_t num_inputs = sizeof(inputs) / sizeof(*inputs);
  for (std::size_t i = 0; i < num_inputs; ++i) {
    for (std::size_t j = 0; j < num_inputs; ++j) {
      string_view lhs(inputs[i]), rhs(inputs[j]);
      EXPECT_EQ(Op<int>()(lhs.compare(rhs), 0), Op<string_view>()(lhs, rhs));
    }
  }
}

TEST(UtilTest, StringRefCompare) {
  EXPECT_EQ(0, string_view("foo").compare(string_view("foo")));
  EXPECT_GT(string_view("fop").compare(string_view("foo")), 0);
  EXPECT_LT(string_view("foo").compare(string_view("fop")), 0);
  EXPECT_GT(string_view("foo").compare(string_view("fo")), 0);
  EXPECT_LT(string_view("fo").compare(string_view("foo")), 0);
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
  fmt::format_system_error(message, EDOM, fmt::string_view(nullptr, max_size));
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

enum TestEnum2 {};

TEST(UtilTest, ConvertToInt) {
  EXPECT_FALSE((fmt::internal::convert_to_int<char, char>::value));
  EXPECT_FALSE((fmt::internal::convert_to_int<const char *, char>::value));
  EXPECT_TRUE((fmt::internal::convert_to_int<TestEnum2, char>::value));
}

#if FMT_USE_ENUM_BASE
enum TestEnum : char {TestValue};
TEST(UtilTest, IsEnumConvertibleToInt) {
  EXPECT_TRUE((fmt::internal::convert_to_int<TestEnum, char>::value));
}
#endif

TEST(UtilTest, ParseNonnegativeInt) {
  if (std::numeric_limits<int>::max() != static_cast<int>(static_cast<unsigned>(1) << 31)) {
    fmt::print("Skipping parse_nonnegative_int test\n");
    return;
  }
  const char *s = "10000000000";
  EXPECT_THROW_MSG(
        parse_nonnegative_int(s, fmt::internal::error_handler()),
        fmt::format_error, "number is too big");
  s = "2147483649";
  EXPECT_THROW_MSG(
        parse_nonnegative_int(s, fmt::internal::error_handler()),
        fmt::format_error, "number is too big");
}

template <bool is_iec559>
void test_construct_from_double() {
  fmt::print("warning: double is not IEC559, skipping FP tests\n");
}

template <>
void test_construct_from_double<true>() {
  auto v = fp(1.23);
  EXPECT_EQ(v.f, 0x13ae147ae147aeu);
  EXPECT_EQ(v.e, -52);
}

TEST(FPTest, ConstructFromDouble) {
  test_construct_from_double<std::numeric_limits<double>::is_iec559>();
}

TEST(FPTest, Normalize) {
  auto v = fp(0xbeef, 42);
  v.normalize();
  EXPECT_EQ(0xbeef000000000000, v.f);
  EXPECT_EQ(-6, v.e);
}

TEST(FPTest, Subtract) {
  auto v = fp(123, 1) - fp(102, 1);
  EXPECT_EQ(v.f, 21u);
  EXPECT_EQ(v.e, 1);
}

TEST(FPTest, Multiply) {
  auto v = fp(123ULL << 32, 4) * fp(56ULL << 32, 7);
  EXPECT_EQ(v.f, 123u * 56u);
  EXPECT_EQ(v.e, 4 + 7 + 64);
  v = fp(123ULL << 32, 4) * fp(567ULL << 31, 8);
  EXPECT_EQ(v.f, (123 * 567 + 1u) / 2);
  EXPECT_EQ(v.e, 4 + 8 + 64);
}

TEST(FPTest, GetCachedPower) {
  typedef std::numeric_limits<double> limits;
  for (auto exp = limits::min_exponent; exp <= limits::max_exponent; ++exp) {
    int dec_exp = 0;
    auto fp = fmt::internal::get_cached_power(exp, dec_exp);
    EXPECT_LE(exp, fp.e);
    int dec_exp_step = 8;
    EXPECT_LE(fp.e, exp + dec_exp_step * log2(10));
    EXPECT_DOUBLE_EQ(pow(10, dec_exp), ldexp(fp.f, fp.e));
  }
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
