
#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "fmt/async.h"
#include "gtest-extra.h"

#define TWENTY_ARGS "{} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} {} "
static const char multiple_brackets[] = TWENTY_ARGS;
static fmt::string_view get_format_string(size_t num_args) { return fmt::string_view(&multiple_brackets[(20 - num_args)*3], num_args * 3); }

namespace trivial_entry_test {
template <typename... Args>
inline void make_async_entry_test(Args&&... args) {
    std::string formatted = fmt::format(std::forward<Args>(args)...);
    // format_arg_store containing named_args are not copy-constructible, auto&& is required.
    auto&& entry = fmt::make_async_entry(std::forward<Args>(args)...);
    EXPECT_EQ(formatted, fmt::async::format(entry));
}

template <typename... Args>
inline void make_async_entry_test_args(Args&&... args) {
    make_async_entry_test(get_format_string(sizeof...(args)), std::forward<Args>(args)...);
}

void make_async_entry_and_alter(const std::string& s) {
    std::string str = s;
    std::string formatted = fmt::format("{}", str);
    auto entry = fmt::make_async_entry("{}", str);
    str.front() =  formatted.front() = '#';
    str.back() = formatted.back() = '#';
    EXPECT_EQ(formatted, fmt::format("{}", str));
    EXPECT_EQ(formatted, fmt::async::format(entry));
}
}

namespace stored_entry_test {
static char buf[1 * 1024 * 1024] = {}; // 1M should be enough for this test
static fmt::basic_async_entry<fmt::format_context>& entry = reinterpret_cast<fmt::basic_async_entry<fmt::format_context>&>(buf);

template <typename... Args>
inline void make_async_entry_test(Args&&... args) {
    std::string formatted = fmt::format(std::forward<Args>(args)...);
    // FIXME: how to test entry_size?
    size_t entry_size = fmt::store_async_entry(buf, std::forward<Args>(args)...);
    EXPECT_EQ(formatted, fmt::async::format(entry));
}

template <typename... Args>
inline void make_async_entry_test_args(Args&&... args) {
    make_async_entry_test(get_format_string(sizeof...(args)), std::forward<Args>(args)...);
}

void make_async_entry_and_alter(const std::string& s) {
    std::string str = s;
    std::string formatted = fmt::format("{}", str);
    size_t entry_size = fmt::store_async_entry(buf, "{}", str);
    str.front() = str.back() = '#';
    EXPECT_EQ(formatted, fmt::async::format(entry));
    formatted.front() = formatted.back() = '#';
    EXPECT_EQ(formatted, fmt::format("{}", str));
}
}

TEST(AsyncTest, TrivialEntry) {
    using namespace trivial_entry_test;
    // basic test
    make_async_entry_test("The answer is {}", 42);
    // index
    make_async_entry_test("The answer of {2}*{1} is {0}", 42, 6, 7);

    // named args
    using namespace fmt::literals;
    make_async_entry_test("The answer of {}*{a} is {product}", 6, "product"_a=42, "a"_a=7);

    // long arg list (>=16, as max_packed_args = 15)
    make_async_entry_test_args(short(1), (unsigned short)2, 3, 4U, 5L, 6UL, 7LL, 8ULL, 9.0F, 10.0, 11, 12, 13, 14, 15, 16, 17, 18);
    make_async_entry_test(TWENTY_ARGS "{narg}", 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,0/*ignore*/,"narg"_a="bingo");

    // this API copies only reference.
    make_async_entry_and_alter("[change me]");
}


TEST(AsyncTest, StoredEntry) {
    using namespace stored_entry_test;
    // basic test
    make_async_entry_test("The answer is {}", 42);
    // index
    make_async_entry_test("The answer of {2}*{1} is {0}", 42, 6, 7);

    // named args
    using namespace fmt::literals;
    // make_async_entry_test("The answer of {}*{a} is {product}", 6, "product"_a=42, "a"_a=7);

    // long arg list (>=16, as max_packed_args = 15)
    make_async_entry_test_args(short(1), (unsigned short)2, 3, 4U, 5L, 6UL, 7LL, 8ULL, 9.0F, 10.0, 11, 12, 13, 14, 15, 16, 17, 18);
    // make_async_entry_test(TWENTY_ARGS "{narg}", 1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,0/*ignore*/,"narg"_a="bingo");

    // this API copies only reference.
    make_async_entry_and_alter("[change me]");
}
