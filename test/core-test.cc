// Formatting library for C++ - core tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <algorithm>
#include <climits>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

#include "gmock.h"
#include "test-assert.h"

// Check if fmt/core.h compiles with windows.h included before it.
#ifdef _WIN32
#  include <windows.h>
#endif

#include "fmt/args.h"
#if defined(FMT_COMPILE_TIME_CHECKS) && FMT_COMPILE_TIME_CHECKS
#  include "fmt/format.h"
#endif

#undef min
#undef max

using fmt::basic_format_arg;
using fmt::string_view;
using fmt::detail::buffer;
using fmt::detail::make_arg;
using fmt::detail::value;

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

struct test_struct {};

FMT_BEGIN_NAMESPACE
template <typename Char> struct formatter<test_struct, Char> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(test_struct, format_context& ctx) -> decltype(ctx.out()) {
    const Char* test = "test";
    return std::copy_n(test, std::strlen(test), ctx.out());
  }
};
FMT_END_NAMESPACE

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 470
TEST(BufferTest, Noncopyable) {
  EXPECT_FALSE(std::is_copy_constructible<buffer<char>>::value);
#  if !FMT_MSC_VER
  // std::is_copy_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_copy_assignable<buffer<char>>::value);
#  endif
}

TEST(BufferTest, Nonmoveable) {
  EXPECT_FALSE(std::is_move_constructible<buffer<char>>::value);
#  if !FMT_MSC_VER
  // std::is_move_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_move_assignable<buffer<char>>::value);
#  endif
}
#endif

TEST(BufferTest, Indestructible) {
  static_assert(!std::is_destructible<fmt::detail::buffer<int>>(),
                "buffer's destructor is protected");
}

template <typename T> struct mock_buffer final : buffer<T> {
  MOCK_METHOD1(do_grow, size_t(size_t capacity));

  void grow(size_t capacity) { this->set(this->data(), do_grow(capacity)); }

  mock_buffer(T* data = nullptr, size_t buf_capacity = 0) {
    this->set(data, buf_capacity);
    ON_CALL(*this, do_grow(_)).WillByDefault(Invoke([](size_t capacity) {
      return capacity;
    }));
  }
};

TEST(BufferTest, Ctor) {
  {
    mock_buffer<int> buffer;
    EXPECT_EQ(nullptr, buffer.data());
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    mock_buffer<int> buffer(&dummy);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(static_cast<size_t>(0), buffer.capacity());
  }
  {
    int dummy;
    size_t capacity = std::numeric_limits<size_t>::max();
    mock_buffer<int> buffer(&dummy, capacity);
    EXPECT_EQ(&dummy, &buffer[0]);
    EXPECT_EQ(static_cast<size_t>(0), buffer.size());
    EXPECT_EQ(capacity, buffer.capacity());
  }
}

TEST(BufferTest, Access) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  buffer[0] = 11;
  EXPECT_EQ(11, buffer[0]);
  buffer[3] = 42;
  EXPECT_EQ(42, *(&buffer[0] + 3));
  const fmt::detail::buffer<char>& const_buffer = buffer;
  EXPECT_EQ(42, const_buffer[3]);
}

TEST(BufferTest, TryResize) {
  char data[123];
  mock_buffer<char> buffer(data, sizeof(data));
  buffer[10] = 42;
  EXPECT_EQ(42, buffer[10]);
  buffer.try_resize(20);
  EXPECT_EQ(20u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  buffer.try_resize(5);
  EXPECT_EQ(5u, buffer.size());
  EXPECT_EQ(123u, buffer.capacity());
  EXPECT_EQ(42, buffer[10]);
  // Check if try_resize calls grow.
  EXPECT_CALL(buffer, do_grow(124));
  buffer.try_resize(124);
  EXPECT_CALL(buffer, do_grow(200));
  buffer.try_resize(200);
}

TEST(BufferTest, TryResizePartial) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  EXPECT_CALL(buffer, do_grow(20)).WillOnce(Return(15));
  buffer.try_resize(20);
  EXPECT_EQ(buffer.capacity(), 15);
  EXPECT_EQ(buffer.size(), 15);
}

TEST(BufferTest, Clear) {
  mock_buffer<char> buffer;
  EXPECT_CALL(buffer, do_grow(20));
  buffer.try_resize(20);
  buffer.try_resize(0);
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(20u, buffer.capacity());
}

TEST(BufferTest, Append) {
  char data[15];
  mock_buffer<char> buffer(data, 10);
  auto test = "test";
  buffer.append(test, test + 5);
  EXPECT_STREQ(test, &buffer[0]);
  EXPECT_EQ(5u, buffer.size());
  buffer.try_resize(10);
  EXPECT_CALL(buffer, do_grow(12));
  buffer.append(test, test + 2);
  EXPECT_EQ('t', buffer[10]);
  EXPECT_EQ('e', buffer[11]);
  EXPECT_EQ(12u, buffer.size());
}

TEST(BufferTest, AppendPartial) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  testing::InSequence seq;
  EXPECT_CALL(buffer, do_grow(15)).WillOnce(Return(10));
  EXPECT_CALL(buffer, do_grow(15)).WillOnce(Invoke([&buffer](size_t) {
    EXPECT_EQ(fmt::string_view(buffer.data(), buffer.size()), "0123456789");
    buffer.clear();
    return 10;
  }));
  auto test = "0123456789abcde";
  buffer.append(test, test + 15);
}

TEST(BufferTest, AppendAllocatesEnoughStorage) {
  char data[19];
  mock_buffer<char> buffer(data, 10);
  auto test = "abcdefgh";
  buffer.try_resize(10);
  EXPECT_CALL(buffer, do_grow(19));
  buffer.append(test, test + 9);
}

TEST(ArgTest, FormatArgs) {
  auto args = fmt::format_args();
  EXPECT_FALSE(args.get(1));
}

struct custom_context {
  using char_type = char;
  using parse_context_type = fmt::format_parse_context;

  template <typename T> struct formatter_type {
    template <typename ParseContext>
    auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
      return ctx.begin();
    }

    const char* format(const T&, custom_context& ctx) {
      ctx.called = true;
      return nullptr;
    }
  };

  bool called;
  fmt::format_parse_context parse_ctx;

  fmt::format_parse_context& parse_context() { return parse_ctx; }
  void advance_to(const char*) {}
};

TEST(ArgTest, MakeValueWithCustomContext) {
  auto t = test_struct();
  fmt::detail::value<custom_context> arg(
      fmt::detail::arg_mapper<custom_context>().map(t));
  custom_context ctx = {false, fmt::format_parse_context("")};
  arg.custom.format(&t, ctx.parse_context(), ctx);
  EXPECT_TRUE(ctx.called);
}

FMT_BEGIN_NAMESPACE
namespace detail {
template <typename Char>
bool operator==(custom_value<Char> lhs, custom_value<Char> rhs) {
  return lhs.value == rhs.value;
}
}  // namespace detail
FMT_END_NAMESPACE

// Use a unique result type to make sure that there are no undesirable
// conversions.
struct test_result {};

template <typename T> struct mock_visitor {
  template <typename U> struct result { using type = test_result; };

  mock_visitor() {
    ON_CALL(*this, visit(_)).WillByDefault(Return(test_result()));
  }

  MOCK_METHOD1_T(visit, test_result(T value));
  MOCK_METHOD0_T(unexpected, void());

  test_result operator()(T value) { return visit(value); }

  template <typename U> test_result operator()(U) {
    unexpected();
    return test_result();
  }
};

template <typename T> struct visit_type { using type = T; };

#define VISIT_TYPE(type_, visit_type_) \
  template <> struct visit_type<type_> { using type = visit_type_; }

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

#define CHECK_ARG_(Char, expected, value)                                     \
  {                                                                           \
    testing::StrictMock<mock_visitor<decltype(expected)>> visitor;            \
    EXPECT_CALL(visitor, visit(expected));                                    \
    using iterator = std::back_insert_iterator<buffer<Char>>;                 \
    fmt::visit_format_arg(                                                    \
        visitor, make_arg<fmt::basic_format_context<iterator, Char>>(value)); \
  }

#define CHECK_ARG(value, typename_)                          \
  {                                                          \
    using value_type = decltype(value);                      \
    typename_ visit_type<value_type>::type expected = value; \
    CHECK_ARG_(char, expected, value)                        \
    CHECK_ARG_(wchar_t, expected, value)                     \
  }

template <typename T> class NumericArgTest : public testing::Test {};

using types =
    ::testing::Types<bool, signed char, unsigned char, signed, unsigned short,
                     int, unsigned, long, unsigned long, long long,
                     unsigned long long, float, double, long double>;
TYPED_TEST_CASE(NumericArgTest, types);

template <typename T>
fmt::enable_if_t<std::is_integral<T>::value, T> test_value() {
  return static_cast<T>(42);
}

template <typename T>
fmt::enable_if_t<std::is_floating_point<T>::value, T> test_value() {
  return static_cast<T>(4.2);
}

TYPED_TEST(NumericArgTest, MakeAndVisit) {
  CHECK_ARG(test_value<TypeParam>(), typename);
  CHECK_ARG(std::numeric_limits<TypeParam>::min(), typename);
  CHECK_ARG(std::numeric_limits<TypeParam>::max(), typename);
}

TEST(ArgTest, CharArg) {
  CHECK_ARG_(char, 'a', 'a');
  CHECK_ARG_(wchar_t, L'a', 'a');
  CHECK_ARG_(wchar_t, L'a', L'a');
}

TEST(ArgTest, StringArg) {
  char str_data[] = "test";
  char* str = str_data;
  const char* cstr = str;
  CHECK_ARG_(char, cstr, str);

  auto sref = string_view(str);
  CHECK_ARG_(char, sref, std::string(str));
}

TEST(ArgTest, WStringArg) {
  wchar_t str_data[] = L"test";
  wchar_t* str = str_data;
  const wchar_t* cstr = str;

  fmt::wstring_view sref(str);
  CHECK_ARG_(wchar_t, cstr, str);
  CHECK_ARG_(wchar_t, cstr, cstr);
  CHECK_ARG_(wchar_t, sref, std::wstring(str));
  CHECK_ARG_(wchar_t, sref, fmt::wstring_view(str));
}

TEST(ArgTest, PointerArg) {
  void* p = nullptr;
  const void* cp = nullptr;
  CHECK_ARG_(char, cp, p);
  CHECK_ARG_(wchar_t, cp, p);
  CHECK_ARG(cp, );
}

struct check_custom {
  test_result operator()(
      fmt::basic_format_arg<fmt::format_context>::handle h) const {
    struct test_buffer final : fmt::detail::buffer<char> {
      char data[10];
      test_buffer() : fmt::detail::buffer<char>(data, 0, 10) {}
      void grow(size_t) {}
    } buffer;
    fmt::format_parse_context parse_ctx("");
    fmt::format_context ctx{fmt::detail::buffer_appender<char>(buffer),
                            fmt::format_args()};
    h.format(parse_ctx, ctx);
    EXPECT_EQ("test", std::string(buffer.data, buffer.size()));
    return test_result();
  }
};

TEST(ArgTest, CustomArg) {
  test_struct test;
  using visitor =
      mock_visitor<fmt::basic_format_arg<fmt::format_context>::handle>;
  testing::StrictMock<visitor> v;
  EXPECT_CALL(v, visit(_)).WillOnce(Invoke(check_custom()));
  fmt::visit_format_arg(v, make_arg<fmt::format_context>(test));
}

TEST(ArgTest, VisitInvalidArg) {
  testing::StrictMock<mock_visitor<fmt::monostate>> visitor;
  EXPECT_CALL(visitor, visit(_));
  fmt::basic_format_arg<fmt::format_context> arg;
  fmt::visit_format_arg(visitor, arg);
}

TEST(FormatDynArgsTest, Basic) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(42);
  store.push_back("abc1");
  store.push_back(1.5f);
  EXPECT_EQ("42 and abc1 and 1.5", fmt::vformat("{} and {} and {}", store));
}

TEST(FormatDynArgsTest, StringsAndRefs) {
  // Unfortunately the tests are compiled with old ABI so strings use COW.
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  char str[] = "1234567890";
  store.push_back(str);
  store.push_back(std::cref(str));
  store.push_back(fmt::string_view{str});
  str[0] = 'X';

  std::string result = fmt::vformat("{} and {} and {}", store);
  EXPECT_EQ("1234567890 and X234567890 and X234567890", result);
}

struct custom_type {
  int i = 0;
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<custom_type> {
  auto parse(format_parse_context& ctx) const -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const custom_type& p, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "cust={}", p.i);
  }
};
FMT_END_NAMESPACE

TEST(FormatDynArgsTest, CustomFormat) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  custom_type c{};
  store.push_back(c);
  ++c.i;
  store.push_back(c);
  ++c.i;
  store.push_back(std::cref(c));
  ++c.i;
  std::string result = fmt::vformat("{} and {} and {}", store);
  EXPECT_EQ("cust=0 and cust=1 and cust=3", result);
}

TEST(FormatDynArgsTest, NamedInt) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(fmt::arg("a1", 42));
  EXPECT_EQ("42", fmt::vformat("{a1}", store));
}

TEST(FormatDynArgsTest, NamedStrings) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  char str[]{"1234567890"};
  store.push_back(fmt::arg("a1", str));
  store.push_back(fmt::arg("a2", std::cref(str)));
  str[0] = 'X';
  EXPECT_EQ("1234567890 and X234567890", fmt::vformat("{a1} and {a2}", store));
}

TEST(FormatDynArgsTest, NamedArgByRef) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;

  // Note: fmt::arg() constructs an object which holds a reference
  // to its value. It's not an aggregate, so it doesn't extend the
  // reference lifetime. As a result, it's a very bad idea passing temporary
  // as a named argument value. Only GCC with optimization level >0
  // complains about this.
  //
  // A real life usecase is when you have both name and value alive
  // guarantee their lifetime and thus don't want them to be copied into
  // storages.
  int a1_val{42};
  auto a1 = fmt::arg("a1_", a1_val);
  store.push_back("abc");
  store.push_back(1.5f);
  store.push_back(std::cref(a1));

  std::string result = fmt::vformat("{a1_} and {} and {} and {}", store);
  EXPECT_EQ("42 and abc and 1.5 and 42", result);
}

TEST(FormatDynArgsTest, NamedCustomFormat) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  custom_type c{};
  store.push_back(fmt::arg("c1", c));
  ++c.i;
  store.push_back(fmt::arg("c2", c));
  ++c.i;
  store.push_back(fmt::arg("c_ref", std::cref(c)));
  ++c.i;
  std::string result = fmt::vformat("{c1} and {c2} and {c_ref}", store);
  EXPECT_EQ("cust=0 and cust=1 and cust=3", result);
}

TEST(FormatDynArgsTest, Clear) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(42);

  std::string result = fmt::vformat("{}", store);
  EXPECT_EQ("42", result);

  store.push_back(43);
  result = fmt::vformat("{} and {}", store);
  EXPECT_EQ("42 and 43", result);

  store.clear();
  store.push_back(44);
  result = fmt::vformat("{}", store);
  EXPECT_EQ("44", result);
}

TEST(FormatDynArgsTest, Reserve) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.reserve(2, 1);
  store.push_back(1.5f);
  store.push_back(fmt::arg("a1", 42));
  std::string result = fmt::vformat("{a1} and {}", store);
  EXPECT_EQ("42 and 1.5", result);
}

struct copy_throwable {
  copy_throwable() {}
  copy_throwable(const copy_throwable&) { throw "deal with it"; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<copy_throwable> {
  auto parse(format_parse_context& ctx) const -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(copy_throwable, format_context& ctx) -> decltype(ctx.out()) {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(FormatDynArgsTest, ThrowOnCopy) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(std::string("foo"));
  try {
    store.push_back(copy_throwable());
  } catch (...) {
  }
  EXPECT_EQ(fmt::vformat("{}", store), "foo");
}

TEST(StringViewTest, ValueType) {
  static_assert(std::is_same<string_view::value_type, char>::value, "");
}

TEST(StringViewTest, Length) {
  // Test that string_view::size() returns string length, not buffer size.
  char str[100] = "some string";
  EXPECT_EQ(std::strlen(str), string_view(str).size());
  EXPECT_LT(std::strlen(str), sizeof(str));
}

// Check string_view's comparison operator.
template <template <typename> class Op> void check_op() {
  const char* inputs[] = {"foo", "fop", "fo"};
  size_t num_inputs = sizeof(inputs) / sizeof(*inputs);
  for (size_t i = 0; i < num_inputs; ++i) {
    for (size_t j = 0; j < num_inputs; ++j) {
      string_view lhs(inputs[i]), rhs(inputs[j]);
      EXPECT_EQ(Op<int>()(lhs.compare(rhs), 0), Op<string_view>()(lhs, rhs));
    }
  }
}

TEST(StringViewTest, Compare) {
  EXPECT_EQ(string_view("foo").compare(string_view("foo")), 0);
  EXPECT_GT(string_view("fop").compare(string_view("foo")), 0);
  EXPECT_LT(string_view("foo").compare(string_view("fop")), 0);
  EXPECT_GT(string_view("foo").compare(string_view("fo")), 0);
  EXPECT_LT(string_view("fo").compare(string_view("foo")), 0);
  check_op<std::equal_to>();
  check_op<std::not_equal_to>();
  check_op<std::less>();
  check_op<std::less_equal>();
  check_op<std::greater>();
  check_op<std::greater_equal>();
}

struct enabled_formatter {};
struct disabled_formatter {};
struct disabled_formatter_convertible {
  operator int() const { return 42; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<enabled_formatter> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(enabled_formatter, format_context& ctx) -> decltype(ctx.out()) {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(CoreTest, HasFormatter) {
  using fmt::has_formatter;
  using context = fmt::format_context;
  static_assert(has_formatter<enabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter_convertible, context>::value,
                "");
}

struct convertible_to_int {
  operator int() const { return 42; }
};

struct convertible_to_c_string {
  operator const char*() const { return "foo"; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<convertible_to_int> {
  auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(convertible_to_int, format_context& ctx) -> decltype(ctx.out()) {
    return std::copy_n("foo", 3, ctx.out());
  }
};

template <> struct formatter<convertible_to_c_string> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(convertible_to_c_string, format_context& ctx)
      -> decltype(ctx.out()) {
    return std::copy_n("bar", 3, ctx.out());
  }
};
FMT_END_NAMESPACE

TEST(CoreTest, FormatterOverridesImplicitConversion) {
  EXPECT_EQ(fmt::format("{}", convertible_to_int()), "foo");
  EXPECT_EQ(fmt::format("{}", convertible_to_c_string()), "bar");
}

namespace my_ns {
template <typename Char> class my_string {
 private:
  std::basic_string<Char> s_;

 public:
  my_string(const Char* s) : s_(s) {}
  const Char* data() const FMT_NOEXCEPT { return s_.data(); }
  size_t length() const FMT_NOEXCEPT { return s_.size(); }
  operator const Char*() const { return s_.c_str(); }
};

template <typename Char>
inline fmt::basic_string_view<Char> to_string_view(const my_string<Char>& s)
    FMT_NOEXCEPT {
  return {s.data(), s.length()};
}

struct non_string {};
}  // namespace my_ns

template <typename T> class IsStringTest : public testing::Test {};

typedef ::testing::Types<char, wchar_t, char16_t, char32_t> StringCharTypes;
TYPED_TEST_CASE(IsStringTest, StringCharTypes);

namespace {
template <typename Char>
struct derived_from_string_view : fmt::basic_string_view<Char> {};
}  // namespace

TYPED_TEST(IsStringTest, IsString) {
  EXPECT_TRUE(fmt::detail::is_string<TypeParam*>::value);
  EXPECT_TRUE(fmt::detail::is_string<const TypeParam*>::value);
  EXPECT_TRUE(fmt::detail::is_string<TypeParam[2]>::value);
  EXPECT_TRUE(fmt::detail::is_string<const TypeParam[2]>::value);
  EXPECT_TRUE(fmt::detail::is_string<std::basic_string<TypeParam>>::value);
  EXPECT_TRUE(fmt::detail::is_string<fmt::basic_string_view<TypeParam>>::value);
  EXPECT_TRUE(
      fmt::detail::is_string<derived_from_string_view<TypeParam>>::value);
  using fmt_string_view = fmt::detail::std_string_view<TypeParam>;
  EXPECT_TRUE(std::is_empty<fmt_string_view>::value !=
              fmt::detail::is_string<fmt_string_view>::value);
  EXPECT_TRUE(fmt::detail::is_string<my_ns::my_string<TypeParam>>::value);
  EXPECT_FALSE(fmt::detail::is_string<my_ns::non_string>::value);
}

TEST(CoreTest, Format) {
  EXPECT_EQ(fmt::format("{}", 42), "42");
}

TEST(CoreTest, FormatTo) {
  std::string s;
  fmt::format_to(std::back_inserter(s), "{}", 42);
  EXPECT_EQ(s, "42");
}

TEST(CoreTest, ToStringViewForeignStrings) {
  using namespace my_ns;
  EXPECT_EQ(to_string_view(my_string<char>("42")), "42");
  fmt::detail::type type =
      fmt::detail::mapped_type_constant<my_string<char>,
                                        fmt::format_context>::value;
  EXPECT_EQ(type, fmt::detail::type::string_type);
}

TEST(CoreTest, FormatForeignStrings) {
  using namespace my_ns;
  EXPECT_EQ(fmt::format(my_string<char>("{}"), 42), "42");
}

struct implicitly_convertible_to_string {
  operator std::string() const { return "foo"; }
};

struct implicitly_convertible_to_string_view {
  operator fmt::string_view() const { return "foo"; }
};

TEST(CoreTest, FormatImplicitlyConvertibleToStringView) {
  EXPECT_EQ("foo", fmt::format("{}", implicitly_convertible_to_string_view()));
}

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VER || FMT_MSC_VER >= 1900
struct explicitly_convertible_to_string_view {
  explicit operator fmt::string_view() const { return "foo"; }
};

TEST(CoreTest, FormatExplicitlyConvertibleToStringView) {
  EXPECT_EQ("foo", fmt::format("{}", explicitly_convertible_to_string_view()));
}

#  ifdef FMT_USE_STRING_VIEW
struct explicitly_convertible_to_std_string_view {
  explicit operator std::string_view() const { return "foo"; }
};

TEST(CoreTest, FormatExplicitlyConvertibleToStdStringView) {
  EXPECT_EQ("foo",
            fmt::format("{}", explicitly_convertible_to_std_string_view()));
}
#  endif
#endif

struct disabled_rvalue_conversion {
  operator const char*() const& { return "foo"; }
  operator const char*() & { return "foo"; }
  operator const char*() const&& = delete;
  operator const char*() && = delete;
};

TEST(CoreTest, DisabledRValueConversion) {
  EXPECT_EQ("foo", fmt::format("{}", disabled_rvalue_conversion()));
}
