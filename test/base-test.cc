// Formatting library for C++ - core tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

// clang-format off
#include "test-assert.h"
// clang-format on

#include "fmt/base.h"

#include <climits>      // INT_MAX
#include <cstring>      // std::strlen
#include <functional>   // std::equal_to
#include <iterator>     // std::back_insert_iterator, std::distance
#include <limits>       // std::numeric_limits
#include <string>       // std::string
#include <type_traits>  // std::is_same

#include "gmock/gmock.h"

using fmt::string_view;
using fmt::detail::buffer;

using testing::_;
using testing::Invoke;
using testing::Return;

#ifdef FMT_FORMAT_H_
#  error core-test includes format.h
#endif

fmt::appender copy(fmt::string_view s, fmt::appender out) {
  for (char c : s) *out++ = c;
  return out;
}

TEST(string_view_test, value_type) {
  static_assert(std::is_same<string_view::value_type, char>::value, "");
}

TEST(string_view_test, ctor) {
  EXPECT_STREQ("abc", fmt::string_view("abc").data());
  EXPECT_EQ(3u, fmt::string_view("abc").size());

  EXPECT_STREQ("defg", fmt::string_view(std::string("defg")).data());
  EXPECT_EQ(4u, fmt::string_view(std::string("defg")).size());
}

TEST(string_view_test, length) {
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

TEST(string_view_test, compare) {
  EXPECT_EQ(string_view("foo").compare(string_view("foo")), 0);
  EXPECT_GT(string_view("fop").compare(string_view("foo")), 0);
  EXPECT_LT(string_view("foo").compare(string_view("fop")), 0);
  EXPECT_GT(string_view("foo").compare(string_view("fo")), 0);
  EXPECT_LT(string_view("fo").compare(string_view("foo")), 0);

  EXPECT_TRUE(string_view("foo").starts_with('f'));
  EXPECT_FALSE(string_view("foo").starts_with('o'));
  EXPECT_FALSE(string_view().starts_with('o'));

  EXPECT_TRUE(string_view("foo").starts_with("fo"));
  EXPECT_TRUE(string_view("foo").starts_with("foo"));
  EXPECT_FALSE(string_view("foo").starts_with("fooo"));
  EXPECT_FALSE(string_view().starts_with("fooo"));

  check_op<std::equal_to>();
  check_op<std::not_equal_to>();
  check_op<std::less>();
  check_op<std::less_equal>();
  check_op<std::greater>();
  check_op<std::greater_equal>();
}

TEST(base_test, is_output_iterator) {
  EXPECT_TRUE((fmt::detail::is_output_iterator<char*, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<const char*, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<std::string, char>::value));
  EXPECT_TRUE(
      (fmt::detail::is_output_iterator<std::back_insert_iterator<std::string>,
                                       char>::value));
  EXPECT_TRUE(
      (fmt::detail::is_output_iterator<std::string::iterator, char>::value));
  EXPECT_FALSE((fmt::detail::is_output_iterator<std::string::const_iterator,
                                                char>::value));
}

TEST(base_test, is_back_insert_iterator) {
  EXPECT_TRUE(fmt::detail::is_back_insert_iterator<
              std::back_insert_iterator<std::string>>::value);
  EXPECT_FALSE(fmt::detail::is_back_insert_iterator<
               std::front_insert_iterator<std::string>>::value);
}

TEST(base_test, buffer_appender) {
#ifdef __cpp_lib_ranges
  static_assert(std::output_iterator<fmt::appender, char>);
#endif
}

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 470
TEST(buffer_test, noncopyable) {
  EXPECT_FALSE(std::is_copy_constructible<buffer<char>>::value);
#  if !FMT_MSC_VERSION
  // std::is_copy_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_copy_assignable<buffer<char>>::value);
#  endif
}

TEST(buffer_test, nonmoveable) {
  EXPECT_FALSE(std::is_move_constructible<buffer<char>>::value);
#  if !FMT_MSC_VERSION
  // std::is_move_assignable is broken in MSVC2013.
  EXPECT_FALSE(std::is_move_assignable<buffer<char>>::value);
#  endif
}
#endif

TEST(buffer_test, indestructible) {
  static_assert(!std::is_destructible<fmt::detail::buffer<int>>(),
                "buffer's destructor is protected");
}

template <typename T> struct mock_buffer final : buffer<T> {
  MOCK_METHOD(size_t, do_grow, (size_t));

  static void grow(buffer<T>& buf, size_t capacity) {
    auto& self = static_cast<mock_buffer&>(buf);
    self.set(buf.data(), self.do_grow(capacity));
  }

  mock_buffer(T* data = nullptr, size_t buf_capacity = 0) : buffer<T>(grow) {
    this->set(data, buf_capacity);
    ON_CALL(*this, do_grow(_)).WillByDefault(Invoke([](size_t capacity) {
      return capacity;
    }));
  }
};

TEST(buffer_test, ctor) {
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

TEST(buffer_test, access) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  buffer[0] = 11;
  EXPECT_EQ(11, buffer[0]);
  buffer[3] = 42;
  EXPECT_EQ(42, *(&buffer[0] + 3));
  const fmt::detail::buffer<char>& const_buffer = buffer;
  EXPECT_EQ(42, const_buffer[3]);
}

TEST(buffer_test, try_resize) {
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

TEST(buffer_test, try_resize_partial) {
  char data[10];
  mock_buffer<char> buffer(data, sizeof(data));
  EXPECT_CALL(buffer, do_grow(20)).WillOnce(Return(15));
  buffer.try_resize(20);
  EXPECT_EQ(buffer.capacity(), 15);
  EXPECT_EQ(buffer.size(), 15);
}

TEST(buffer_test, clear) {
  mock_buffer<char> buffer;
  EXPECT_CALL(buffer, do_grow(20));
  buffer.try_resize(20);
  buffer.try_resize(0);
  EXPECT_EQ(static_cast<size_t>(0), buffer.size());
  EXPECT_EQ(20u, buffer.capacity());
}

TEST(buffer_test, append) {
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

TEST(buffer_test, append_partial) {
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

TEST(buffer_test, append_allocates_enough_storage) {
  char data[19];
  mock_buffer<char> buffer(data, 10);
  auto test = "abcdefgh";
  buffer.try_resize(10);
  EXPECT_CALL(buffer, do_grow(19));
  buffer.append(test, test + 9);
}

struct custom_context {
  using char_type = char;
  using parse_context_type = fmt::format_parse_context;

  bool called = false;

  template <typename T> struct formatter_type {
    FMT_CONSTEXPR auto parse(fmt::format_parse_context& ctx)
        -> decltype(ctx.begin()) {
      return ctx.begin();
    }

    const char* format(const T&, custom_context& ctx) const {
      ctx.called = true;
      return nullptr;
    }
  };

  void advance_to(const char*) {}
};

struct test_struct {};

FMT_BEGIN_NAMESPACE
template <typename Char> struct formatter<test_struct, Char> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(test_struct, format_context& ctx) const -> decltype(ctx.out()) {
    return copy("test", ctx.out());
  }
};
FMT_END_NAMESPACE

TEST(arg_test, format_args) {
  auto args = fmt::format_args();
  EXPECT_FALSE(args.get(1));
}

TEST(arg_test, make_value_with_custom_context) {
  auto t = test_struct();
  auto arg = fmt::detail::value<custom_context>(
      fmt::detail::arg_mapper<custom_context>().map(t));
  auto ctx = custom_context();
  auto parse_ctx = fmt::format_parse_context("");
  arg.custom.format(&t, parse_ctx, ctx);
  EXPECT_TRUE(ctx.called);
}

// Use a unique result type to make sure that there are no undesirable
// conversions.
struct test_result {};

template <typename T> struct mock_visitor {
  template <typename U> struct result {
    using type = test_result;
  };

  mock_visitor() {
    ON_CALL(*this, visit(_)).WillByDefault(Return(test_result()));
  }

  MOCK_METHOD(test_result, visit, (T));
  MOCK_METHOD(void, unexpected, ());

  auto operator()(T value) -> test_result { return visit(value); }

  template <typename U> auto operator()(U) -> test_result {
    unexpected();
    return test_result();
  }
};

template <typename T> struct visit_type {
  using type = T;
};

#define VISIT_TYPE(type_, visit_type_)   \
  template <> struct visit_type<type_> { \
    using type = visit_type_;            \
  }

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

#define CHECK_ARG(Char, expected, value)                                  \
  {                                                                       \
    testing::StrictMock<mock_visitor<decltype(expected)>> visitor;        \
    EXPECT_CALL(visitor, visit(expected));                                \
    using iterator = fmt::basic_appender<Char>;                           \
    auto var = value;                                                     \
    fmt::detail::make_arg<fmt::basic_format_context<iterator, Char>>(var) \
        .visit(visitor);                                                  \
  }

#define CHECK_ARG_SIMPLE(value)                             \
  {                                                         \
    using value_type = decltype(value);                     \
    typename visit_type<value_type>::type expected = value; \
    CHECK_ARG(char, expected, value)                        \
  }

template <typename T> class numeric_arg_test : public testing::Test {};

using test_types =
    testing::Types<bool, signed char, unsigned char, short, unsigned short, int,
                   unsigned, long, unsigned long, long long, unsigned long long,
                   float, double, long double>;
TYPED_TEST_SUITE(numeric_arg_test, test_types);

template <typename T, fmt::enable_if_t<std::is_integral<T>::value, int> = 0>
auto test_value() -> T {
  return static_cast<T>(42);
}

template <typename T,
          fmt::enable_if_t<std::is_floating_point<T>::value, int> = 0>
auto test_value() -> T {
  return static_cast<T>(4.2);
}

TYPED_TEST(numeric_arg_test, make_and_visit) {
  CHECK_ARG_SIMPLE(test_value<TypeParam>());
  CHECK_ARG_SIMPLE(std::numeric_limits<TypeParam>::min());
  CHECK_ARG_SIMPLE(std::numeric_limits<TypeParam>::max());
}

TEST(arg_test, char_arg) { CHECK_ARG(char, 'a', 'a'); }

TEST(arg_test, string_arg) {
  char str_data[] = "test";
  char* str = str_data;
  const char* cstr = str;
  CHECK_ARG(char, cstr, str);

  auto sv = fmt::string_view(str);
  CHECK_ARG(char, sv, std::string(str));
}

TEST(arg_test, pointer_arg) {
  void* p = nullptr;
  const void* cp = nullptr;
  CHECK_ARG(char, cp, p);
  CHECK_ARG_SIMPLE(cp);
}

struct check_custom {
  auto operator()(fmt::basic_format_arg<fmt::format_context>::handle h) const
      -> test_result {
    struct test_buffer final : fmt::detail::buffer<char> {
      char data[10];
      test_buffer()
          : fmt::detail::buffer<char>([](buffer<char>&, size_t) {}, data, 0,
                                      10) {}
    } buffer;
    auto parse_ctx = fmt::format_parse_context("");
    auto ctx = fmt::format_context(fmt::appender(buffer), fmt::format_args());
    h.format(parse_ctx, ctx);
    EXPECT_EQ("test", std::string(buffer.data, buffer.size()));
    return test_result();
  }
};

TEST(arg_test, custom_arg) {
  auto test = test_struct();
  using visitor =
      mock_visitor<fmt::basic_format_arg<fmt::format_context>::handle>;
  auto&& v = testing::StrictMock<visitor>();
  EXPECT_CALL(v, visit(_)).WillOnce(Invoke(check_custom()));
  fmt::detail::make_arg<fmt::format_context>(test).visit(v);
}

TEST(arg_test, visit_invalid_arg) {
  auto&& visitor = testing::StrictMock<mock_visitor<fmt::monostate>>();
  EXPECT_CALL(visitor, visit(_));
  fmt::basic_format_arg<fmt::format_context>().visit(visitor);
}

#if FMT_USE_CONSTEXPR

enum class arg_id_result { none, empty, index, name };
struct test_arg_id_handler {
  arg_id_result res = arg_id_result::none;
  int index = 0;
  string_view name;

  constexpr void on_auto() { res = arg_id_result::empty; }

  constexpr void on_index(int i) {
    res = arg_id_result::index;
    index = i;
  }

  constexpr void on_name(string_view n) {
    res = arg_id_result::name;
    name = n;
  }
};

template <size_t N>
constexpr test_arg_id_handler parse_arg_id(const char (&s)[N]) {
  auto h = test_arg_id_handler();
  fmt::detail::parse_arg_id(s, s + N, h);
  return h;
}

TEST(base_test, constexpr_parse_arg_id) {
  static_assert(parse_arg_id(":").res == arg_id_result::empty, "");
  static_assert(parse_arg_id("}").res == arg_id_result::empty, "");
  static_assert(parse_arg_id("42:").res == arg_id_result::index, "");
  static_assert(parse_arg_id("42:").index == 42, "");
  static_assert(parse_arg_id("foo:").res == arg_id_result::name, "");
  static_assert(parse_arg_id("foo:").name.size() == 3, "");
}

template <size_t N> constexpr auto parse_test_specs(const char (&s)[N]) {
  auto ctx = fmt::detail::compile_parse_context<char>(fmt::string_view(s, N),
                                                      43, nullptr);
  auto specs = fmt::detail::dynamic_format_specs<>();
  fmt::detail::parse_format_specs(s, s + N - 1, specs, ctx,
                                  fmt::detail::type::float_type);
  return specs;
}

TEST(base_test, constexpr_parse_format_specs) {
  static_assert(parse_test_specs("<").align == fmt::align::left, "");
  static_assert(parse_test_specs("*^").fill.get<char>() == '*', "");
  static_assert(parse_test_specs("+").sign == fmt::sign::plus, "");
  static_assert(parse_test_specs("-").sign == fmt::sign::minus, "");
  static_assert(parse_test_specs(" ").sign == fmt::sign::space, "");
  static_assert(parse_test_specs("#").alt, "");
  static_assert(parse_test_specs("0").align == fmt::align::numeric, "");
  static_assert(parse_test_specs("L").localized, "");
  static_assert(parse_test_specs("42").width == 42, "");
  static_assert(parse_test_specs("{42}").width_ref.val.index == 42, "");
  static_assert(parse_test_specs(".42").precision == 42, "");
  static_assert(parse_test_specs(".{42}").precision_ref.val.index == 42, "");
  static_assert(parse_test_specs("f").type == fmt::presentation_type::fixed,
                "");
}

struct test_format_string_handler {
  constexpr void on_text(const char*, const char*) {}

  constexpr auto on_arg_id() -> int { return 0; }

  template <typename T> constexpr auto on_arg_id(T) -> int { return 0; }

  constexpr void on_replacement_field(int, const char*) {}

  constexpr auto on_format_specs(int, const char* begin, const char*) -> const
      char* {
    return begin;
  }

  constexpr void on_error(const char*) { error = true; }

  bool error = false;
};

template <size_t N> constexpr bool parse_string(const char (&s)[N]) {
  auto h = test_format_string_handler();
  fmt::detail::parse_format_string<true>(fmt::string_view(s, N - 1), h);
  return !h.error;
}

TEST(base_test, constexpr_parse_format_string) {
  static_assert(parse_string("foo"), "");
  static_assert(!parse_string("}"), "");
  static_assert(parse_string("{}"), "");
  static_assert(parse_string("{42}"), "");
  static_assert(parse_string("{foo}"), "");
  static_assert(parse_string("{:}"), "");
}
#endif  // FMT_USE_CONSTEXPR

struct enabled_formatter {};
struct enabled_ptr_formatter {};
struct disabled_formatter {};
struct disabled_formatter_convertible {
  operator int() const { return 42; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<enabled_formatter> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(enabled_formatter, format_context& ctx) const
      -> decltype(ctx.out()) {
    return ctx.out();
  }
};

template <> struct formatter<enabled_ptr_formatter*> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }
  auto format(enabled_ptr_formatter*, format_context& ctx) const
      -> decltype(ctx.out()) {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(base_test, has_formatter) {
  using fmt::has_formatter;
  using context = fmt::format_context;
  static_assert(has_formatter<enabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter, context>::value, "");
  static_assert(!has_formatter<disabled_formatter_convertible, context>::value,
                "");
}

struct const_formattable {};
struct nonconst_formattable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<const_formattable> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(const const_formattable&, format_context& ctx) const
      -> decltype(ctx.out()) {
    return copy("test", ctx.out());
  }
};

template <> struct formatter<nonconst_formattable> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(nonconst_formattable&, format_context& ctx) const
      -> decltype(ctx.out()) {
    return copy("test", ctx.out());
  }
};
FMT_END_NAMESPACE

struct convertible_to_pointer {
  operator const int*() const { return nullptr; }
};

struct convertible_to_pointer_formattable {
  operator const int*() const { return nullptr; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<convertible_to_pointer_formattable> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(convertible_to_pointer_formattable, format_context& ctx) const
      -> decltype(ctx.out()) {
    return copy("test", ctx.out());
  }
};
FMT_END_NAMESPACE

enum class unformattable_scoped_enum {};

TEST(base_test, is_formattable) {
  static_assert(!fmt::is_formattable<wchar_t>::value, "");
#ifdef __cpp_char8_t
  static_assert(!fmt::is_formattable<char8_t>::value, "");
#endif
  static_assert(!fmt::is_formattable<char16_t>::value, "");
  static_assert(!fmt::is_formattable<char32_t>::value, "");
  static_assert(!fmt::is_formattable<signed char*>::value, "");
  static_assert(!fmt::is_formattable<unsigned char*>::value, "");
  static_assert(!fmt::is_formattable<const signed char*>::value, "");
  static_assert(!fmt::is_formattable<const unsigned char*>::value, "");
  static_assert(!fmt::is_formattable<const wchar_t*>::value, "");
  static_assert(!fmt::is_formattable<const wchar_t[3]>::value, "");
  static_assert(!fmt::is_formattable<fmt::basic_string_view<wchar_t>>::value,
                "");
  static_assert(fmt::is_formattable<enabled_formatter>::value, "");
  static_assert(!fmt::is_formattable<enabled_ptr_formatter*>::value, "");
  static_assert(!fmt::is_formattable<disabled_formatter>::value, "");
  static_assert(!fmt::is_formattable<disabled_formatter_convertible>::value,
                "");

  static_assert(fmt::is_formattable<const_formattable&>::value, "");
  static_assert(fmt::is_formattable<const const_formattable&>::value, "");

  static_assert(fmt::is_formattable<nonconst_formattable&>::value, "");
#if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1910
  static_assert(!fmt::is_formattable<const nonconst_formattable&>::value, "");
#endif

  static_assert(!fmt::is_formattable<convertible_to_pointer>::value, "");
  const auto f = convertible_to_pointer_formattable();
  auto str = std::string();
  fmt::format_to(std::back_inserter(str), "{}", f);
  EXPECT_EQ(str, "test");

  static_assert(!fmt::is_formattable<void (*)()>::value, "");

  struct s;
  static_assert(!fmt::is_formattable<int(s::*)>::value, "");
  static_assert(!fmt::is_formattable<int (s::*)()>::value, "");
  static_assert(!fmt::is_formattable<unformattable_scoped_enum>::value, "");
  static_assert(!fmt::is_formattable<unformattable_scoped_enum>::value, "");
}

#if FMT_USE_CONCEPTS
TEST(base_test, formattable) {
  static_assert(fmt::formattable<char>);
  static_assert(fmt::formattable<char&>);
  static_assert(fmt::formattable<char&&>);
  static_assert(fmt::formattable<const char>);
  static_assert(fmt::formattable<const char&>);
  static_assert(fmt::formattable<const char&&>);
  static_assert(fmt::formattable<int>);
  static_assert(!fmt::formattable<wchar_t>);
}
#endif

TEST(base_test, format_to) {
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{}", 42);
  EXPECT_EQ(s, "42");
}

TEST(base_test, format_to_array) {
  char buffer[4];
  auto result = fmt::format_to(buffer, "{}", 12345);
  EXPECT_EQ(4, std::distance(&buffer[0], result.out));
  EXPECT_TRUE(result.truncated);
  EXPECT_EQ(buffer + 4, result.out);
  EXPECT_EQ("1234", fmt::string_view(buffer, 4));

  char* out = nullptr;
  EXPECT_THROW(out = result, std::runtime_error);
  (void)out;

  result = fmt::format_to(buffer, "{:s}", "foobar");
  EXPECT_EQ(4, std::distance(&buffer[0], result.out));
  EXPECT_TRUE(result.truncated);
  EXPECT_EQ(buffer + 4, result.out);
  EXPECT_EQ("foob", fmt::string_view(buffer, 4));

  buffer[0] = 'x';
  buffer[1] = 'x';
  buffer[2] = 'x';
  buffer[3] = 'x';
  result = fmt::format_to(buffer, "{}", 'A');
  EXPECT_EQ(1, std::distance(&buffer[0], result.out));
  EXPECT_FALSE(result.truncated);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ("Axxx", fmt::string_view(buffer, 4));

  result = fmt::format_to(buffer, "{}{} ", 'B', 'C');
  EXPECT_EQ(3, std::distance(&buffer[0], result.out));
  EXPECT_FALSE(result.truncated);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("BC x", fmt::string_view(buffer, 4));

  result = fmt::format_to(buffer, "{}", "ABCDE");
  EXPECT_EQ(4, std::distance(&buffer[0], result.out));
  EXPECT_TRUE(result.truncated);
  EXPECT_EQ("ABCD", fmt::string_view(buffer, 4));

  result = fmt::format_to(buffer, "{}", std::string(1000, '*'));
  EXPECT_EQ(4, std::distance(&buffer[0], result.out));
  EXPECT_TRUE(result.truncated);
  EXPECT_EQ("****", fmt::string_view(buffer, 4));
}

#ifdef __cpp_lib_byte
TEST(base_test, format_byte) {
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{}", std::byte(42));
  EXPECT_EQ(s, "42");
}
#endif

// Test that check is not found by ADL.
template <typename T> void check(T);
TEST(base_test, adl_check) {
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{}", test_struct());
  EXPECT_EQ(s, "test");
}

struct implicitly_convertible_to_string_view {
  operator fmt::string_view() const { return "foo"; }
};

TEST(base_test, no_implicit_conversion_to_string_view) {
  EXPECT_FALSE(
      fmt::is_formattable<implicitly_convertible_to_string_view>::value);
}

#ifdef FMT_USE_STRING_VIEW
struct implicitly_convertible_to_std_string_view {
  operator std::string_view() const { return "foo"; }
};

TEST(base_test, no_implicit_conversion_to_std_string_view) {
  EXPECT_FALSE(
      fmt::is_formattable<implicitly_convertible_to_std_string_view>::value);
}
#endif

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VERSION || FMT_MSC_VERSION >= 1900
struct explicitly_convertible_to_string_view {
  explicit operator fmt::string_view() const { return "foo"; }
};

TEST(base_test, format_explicitly_convertible_to_string_view) {
  // Types explicitly convertible to string_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !fmt::is_formattable<explicitly_convertible_to_string_view>::value, "");
}

#  ifdef FMT_USE_STRING_VIEW
struct explicitly_convertible_to_std_string_view {
  explicit operator std::string_view() const { return "foo"; }
};

TEST(base_test, format_explicitly_convertible_to_std_string_view) {
  // Types explicitly convertible to string_view are not formattable by
  // default because it may introduce ODR violations.
  static_assert(
      !fmt::is_formattable<explicitly_convertible_to_std_string_view>::value,
      "");
}
#  endif
#endif

TEST(base_test, has_const_formatter) {
  EXPECT_TRUE((fmt::detail::has_const_formatter<const_formattable,
                                                fmt::format_context>()));
  EXPECT_FALSE((fmt::detail::has_const_formatter<nonconst_formattable,
                                                 fmt::format_context>()));
}

TEST(base_test, format_nonconst) {
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{}", nonconst_formattable());
  EXPECT_EQ(s, "test");
}

TEST(base_test, throw_in_buffer_dtor) {
  enum { buffer_size = 256 };

  struct throwing_iterator {
    int& count;

    auto operator=(char) -> throwing_iterator& {
      if (++count > buffer_size) throw std::exception();
      return *this;
    }
    auto operator*() -> throwing_iterator& { return *this; }
    auto operator++() -> throwing_iterator& { return *this; }
    auto operator++(int) -> throwing_iterator { return *this; }
  };

  try {
    int count = 0;
    fmt::format_to(throwing_iterator{count}, fmt::runtime("{:{}}{"), "",
                   buffer_size + 1);
  } catch (const std::exception&) {
  }
}

struct its_a_trap {
  template <typename T> operator T() const {
    auto v = T();
    v.x = 42;
    return v;
  }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<its_a_trap> {
  FMT_CONSTEXPR auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  auto format(its_a_trap, format_context& ctx) const
      -> decltype(ctx.out()) const {
    auto out = ctx.out();
    *out++ = 'x';
    return out;
  }
};
FMT_END_NAMESPACE

TEST(base_test, trappy_conversion) {
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{}", its_a_trap());
  EXPECT_EQ(s, "x");
}
