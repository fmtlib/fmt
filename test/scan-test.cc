// Formatting library for C++ - scanning API proof of concept
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <climits>

#include "fmt/format.h"
#include "gmock.h"
#include "gtest-extra.h"

FMT_BEGIN_NAMESPACE
namespace internal {
enum class scan_type {
  none_type,
  int_type,
  uint_type,
  long_long_type,
  ulong_long_type,
  string_type,
  string_view_type
};

struct scan_arg {
  scan_type arg_type;
  union {
    int* int_value;
    unsigned* uint_value;
    long long* long_long_value;
    unsigned long long* ulong_long_value;
    std::string* string;
    fmt::string_view* string_view;
    // TODO: more types
  };

  scan_arg() : arg_type(scan_type::none_type) {}
  scan_arg(int& value) : arg_type(scan_type::int_type), int_value(&value) {}
  scan_arg(unsigned& value)
      : arg_type(scan_type::uint_type), uint_value(&value) {}
  scan_arg(long long& value)
      : arg_type(scan_type::long_long_type), long_long_value(&value) {}
  scan_arg(unsigned long long& value)
      : arg_type(scan_type::ulong_long_type), ulong_long_value(&value) {}
  scan_arg(std::string& value)
      : arg_type(scan_type::string_type), string(&value) {}
  scan_arg(fmt::string_view& value)
      : arg_type(scan_type::string_view_type), string_view(&value) {}
};
}  // namespace internal

struct scan_args {
  int size;
  const internal::scan_arg* data;

  template <size_t N>
  scan_args(const std::array<internal::scan_arg, N>& store)
      : size(N), data(store.data()) {
    static_assert(N < INT_MAX, "too many arguments");
  }
};

namespace internal {

struct scan_handler : error_handler {
 private:
  const char* begin_;
  const char* end_;
  scan_args args_;
  int next_arg_id_;
  scan_arg arg_;

  template <typename T = unsigned> T read_uint() {
    T value = 0;
    while (begin_ != end_) {
      char c = *begin_++;
      if (c < '0' || c > '9') on_error("invalid input");
      // TODO: check overflow
      value = value * 10 + (c - '0');
    }
    return value;
  }

  template <typename T = int> T read_int() {
    T value = 0;
    bool negative = begin_ != end_ && *begin_ == '-';
    if (negative) ++begin_;
    value = read_uint<typename std::make_unsigned<T>::type>();
    if (negative) value = -value;
    return value;
  }

 public:
  scan_handler(string_view input, scan_args args)
      : begin_(input.data()),
        end_(begin_ + input.size()),
        args_(args),
        next_arg_id_(0) {}

  const char* pos() const { return begin_; }

  void on_text(const char* begin, const char* end) {
    auto size = end - begin;
    if (begin_ + size > end_ || !std::equal(begin, end, begin_))
      on_error("invalid input");
    begin_ += size;
  }

  void on_arg_id() {
    if (next_arg_id_ >= args_.size) on_error("argument index out of range");
    arg_ = args_.data[next_arg_id_++];
  }
  void on_arg_id(unsigned) { on_error("invalid format"); }
  void on_arg_id(string_view) { on_error("invalid format"); }

  void on_replacement_field(const char*) {
    switch (arg_.arg_type) {
    case scan_type::int_type:
      *arg_.int_value = read_int();
      break;
    case scan_type::uint_type:
      *arg_.uint_value = read_uint();
      break;
    case scan_type::long_long_type:
      *arg_.long_long_value = read_int<long long>();
      break;
    case scan_type::ulong_long_type:
      *arg_.ulong_long_value = read_uint<unsigned long long>();
      break;
    case scan_type::string_type:
      while (begin_ != end_ && *begin_ != ' ')
        arg_.string->push_back(*begin_++);
      break;
    case scan_type::string_view_type: {
      auto s = begin_;
      while (begin_ != end_ && *begin_ != ' ')
        ++begin_;
      *arg_.string_view = fmt::string_view(s, begin_ - s);
      break;
    }
    default:
      assert(false);
    }
  }

  const char* on_format_specs(const char* begin, const char*) { return begin; }
};
}  // namespace internal

template <typename... Args>
std::array<internal::scan_arg, sizeof...(Args)> make_scan_args(Args&... args) {
  return std::array<internal::scan_arg, sizeof...(Args)>{args...};
}

string_view::iterator vscan(string_view input, string_view format_str,
                            scan_args args) {
  internal::scan_handler h(input, args);
  internal::parse_format_string<false>(format_str, h);
  return input.begin() + (h.pos() - &*input.begin());
}

template <typename... Args>
string_view::iterator scan(string_view input, string_view format_str,
                           Args&... args) {
  return vscan(input, format_str, make_scan_args(args...));
}
FMT_END_NAMESPACE

TEST(ScanTest, ReadText) {
  fmt::string_view s = "foo";
  auto end = fmt::scan(s, "foo");
  EXPECT_EQ(end, s.end());
  EXPECT_THROW_MSG(fmt::scan("fob", "foo"), fmt::format_error, "invalid input");
}

TEST(ScanTest, ReadInt) {
  int n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(ScanTest, ReadLongLong) {
  long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(ScanTest, ReadUInt) {
  unsigned n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(ScanTest, ReadULongLong) {
  unsigned long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(ScanTest, ReadString) {
  std::string s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

TEST(ScanTest, ReadStringView) {
  fmt::string_view s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

TEST(ScanTest, InvalidFormat) {
  EXPECT_THROW_MSG(fmt::scan("", "{}"), fmt::format_error,
                   "argument index out of range");
  EXPECT_THROW_MSG(fmt::scan("", "{"), fmt::format_error,
                   "invalid format string");
}

TEST(ScanTest, Example) {
  std::string key;
  int value;
  fmt::scan("answer = 42", "{} = {}", key, value);
  EXPECT_EQ(key, "answer");
  EXPECT_EQ(value, 42);
}
