// Formatting library for C++ - scanning API proof of concept
//
// Copyright (c) 2019 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <cassert>
#include <climits>

#include "fmt/format.h"

FMT_BEGIN_NAMESPACE
namespace detail {

struct maybe_contiguous_range {
  const char* begin;
  const char* end;

  explicit operator bool() const { return begin != nullptr; }
};

class scan_buffer {
 private:
  const char* ptr_;
  const char* end_;
  bool contiguous_;

 protected:
  scan_buffer(const char* ptr, const char* end, bool contiguous)
      : ptr_(ptr), end_(end), contiguous_(contiguous) {}
  ~scan_buffer() = default;

  auto is_empty() const -> bool { return ptr_ == end_; }

  void set(const char* ptr, const char* end) noexcept {
    ptr_ = ptr;
    end_ = end;
  }

  auto peek() -> int {
    if (ptr_ == end_) {
      // TODO: refill buffer
      return EOF;
    }
    return *ptr_;
  }

  // Fills the buffer with more input if available.
  virtual void fill() = 0;

 public:
  scan_buffer(const scan_buffer&) = delete;
  void operator=(const scan_buffer&) = delete;

  class iterator {
   private:
    const char** ptr_;
    scan_buffer* buf_;  // This could be merged with ptr_.
    char value_;

    static auto sentinel() -> const char** {
      static const char* ptr = nullptr;
      return &ptr;
    }

    friend class scan_buffer;

    friend auto operator==(iterator lhs, iterator rhs) -> bool {
      return *lhs.ptr_ == *rhs.ptr_;
    }
    friend auto operator!=(iterator lhs, iterator rhs) -> bool {
      return *lhs.ptr_ != *rhs.ptr_;
    }

    iterator(scan_buffer* buf)
        : ptr_(&buf->ptr_), buf_(buf), value_(static_cast<char>(buf->peek())) {
      if (value_ == EOF) ptr_ = sentinel();
    }

   public:
    iterator() : ptr_(sentinel()), buf_(nullptr) {}

    auto operator++() -> iterator& {
      if (!buf_->try_consume()) ptr_ = sentinel();
      value_ = *buf_->ptr_;
      return *this;
    }
    auto operator++(int) -> iterator {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    auto operator*() const -> char { return value_; }

    auto base() const -> const char* { return buf_->ptr_; }

    friend auto to_contiguous(iterator it) -> maybe_contiguous_range;
    friend void advance(iterator& it, size_t n);
  };

  friend auto to_contiguous(iterator it) -> maybe_contiguous_range {
    if (it.buf_->is_contiguous()) return {it.buf_->ptr_, it.buf_->end_};
    return {nullptr, nullptr};
  }
  friend void advance(iterator& it, size_t n) {
    FMT_ASSERT(it.buf_->is_contiguous(), "");
    const char*& ptr = it.buf_->ptr_;
    ptr += n;
    it.value_ = *ptr;
    if (ptr == it.buf_->end_) it.ptr_ = iterator::sentinel();
  }

  auto begin() noexcept -> iterator { return this; }
  auto end() noexcept -> iterator { return {}; }

  auto is_contiguous() const -> bool { return contiguous_; }

  // Tries consuming a single code unit.
  auto try_consume() -> bool {
    FMT_ASSERT(ptr_ != end_, "");
    ++ptr_;
    if (ptr_ == end_) {
      // TODO: refill buffer
      return false;
    }
    return true;
  }
};

class string_scan_buffer : public scan_buffer {
 private:
  void fill() override {}

 public:
  explicit string_scan_buffer(string_view s)
      : scan_buffer(s.begin(), s.end(), true) {}
};

class file_scan_buffer : public scan_buffer {
 private:
  FILE* file_;
  char next_;

  template <typename F, FMT_ENABLE_IF(sizeof(F::_p) != 0)>
  void set_buffer(int, F* f) {
    const char* ptr = reinterpret_cast<const char*>(f->_p);
    this->set(ptr, ptr + f->_r);
  }
  void set_buffer(int c, ...) {
    if (c == EOF) return;
    next_ = static_cast<char>(c);
    this->set(&next_, &next_ + 1);
  }

  void fill() override {
    int result = getc(file_);
    if (result == EOF) {
      if (ferror(file_) != 0)
        FMT_THROW(system_error(errno, FMT_STRING("I/O error")));
      return;
    }
    // Put the character back since we are only filling the buffer.
    if (ungetc(result, file_) == EOF)
      FMT_THROW(system_error(errno, FMT_STRING("I/O error")));
    set_buffer(result, file_);
  }

 public:
  explicit file_scan_buffer(FILE* f)
      : scan_buffer(nullptr, nullptr, false), file_(f) {
    // TODO: lock file?
    set_buffer(EOF, f);
    if (is_empty()) fill();
  }
};
}  // namespace detail

template <typename T, typename Char = char> struct scanner {
  // A deleted default constructor indicates a disabled scanner.
  scanner() = delete;
};

class scan_parse_context {
 private:
  string_view format_;

 public:
  using iterator = string_view::iterator;

  explicit FMT_CONSTEXPR scan_parse_context(string_view format)
      : format_(format) {}

  FMT_CONSTEXPR auto begin() const -> iterator { return format_.begin(); }
  FMT_CONSTEXPR auto end() const -> iterator { return format_.end(); }

  void advance_to(iterator it) {
    format_.remove_prefix(detail::to_unsigned(it - begin()));
  }
};

struct scan_context {
 private:
  detail::scan_buffer& buf_;

 public:
  using iterator = detail::scan_buffer::iterator;

  explicit FMT_CONSTEXPR scan_context(detail::scan_buffer& buf) : buf_(buf) {}

  auto begin() const -> iterator { return buf_.begin(); }
  auto end() const -> iterator { return buf_.end(); }

  void advance_to(iterator) {
    // The scan_buffer iterator automatically updates the buffer position when
    // incremented.
  }
};

namespace detail {
enum class scan_type {
  none_type,
  int_type,
  uint_type,
  long_long_type,
  ulong_long_type,
  string_type,
  string_view_type,
  custom_type
};

struct custom_scan_arg {
  void* value;
  void (*scan)(void* arg, scan_parse_context& parse_ctx, scan_context& ctx);
};

class scan_arg {
 public:
  scan_type type;
  union {
    int* int_value;
    unsigned* uint_value;
    long long* long_long_value;
    unsigned long long* ulong_long_value;
    std::string* string;
    fmt::string_view* string_view;
    custom_scan_arg custom;
    // TODO: more types
  };

  FMT_CONSTEXPR scan_arg() : type(scan_type::none_type), int_value(nullptr) {}
  FMT_CONSTEXPR scan_arg(int& value)
      : type(scan_type::int_type), int_value(&value) {}
  FMT_CONSTEXPR scan_arg(unsigned& value)
      : type(scan_type::uint_type), uint_value(&value) {}
  FMT_CONSTEXPR scan_arg(long long& value)
      : type(scan_type::long_long_type), long_long_value(&value) {}
  FMT_CONSTEXPR scan_arg(unsigned long long& value)
      : type(scan_type::ulong_long_type), ulong_long_value(&value) {}
  FMT_CONSTEXPR scan_arg(std::string& value)
      : type(scan_type::string_type), string(&value) {}
  FMT_CONSTEXPR scan_arg(fmt::string_view& value)
      : type(scan_type::string_view_type), string_view(&value) {}
  template <typename T>
  FMT_CONSTEXPR scan_arg(T& value) : type(scan_type::custom_type) {
    custom.value = &value;
    custom.scan = scan_custom_arg<T>;
  }

 private:
  template <typename T>
  static void scan_custom_arg(void* arg, scan_parse_context& parse_ctx,
                              scan_context& ctx) {
    auto s = scanner<T>();
    parse_ctx.advance_to(s.parse(parse_ctx));
    ctx.advance_to(s.scan(*static_cast<T*>(arg), ctx));
  }
};
}  // namespace detail

struct scan_args {
  int size;
  const detail::scan_arg* data;

  template <size_t N>
  FMT_CONSTEXPR scan_args(const std::array<detail::scan_arg, N>& store)
      : size(N), data(store.data()) {
    static_assert(N < INT_MAX, "too many arguments");
  }
};

namespace detail {

struct scan_handler : error_handler {
 private:
  scan_parse_context parse_ctx_;
  scan_context scan_ctx_;
  scan_args args_;
  int next_arg_id_;
  scan_arg arg_;

  template <typename T = unsigned> auto read_uint() -> T {
    T value = 0;
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    while (it != end) {
      char c = *it++;
      if (c < '0' || c > '9') on_error("invalid input");
      // TODO: check overflow
      value = value * 10 + static_cast<unsigned>(c - '0');
    }
    scan_ctx_.advance_to(it);
    return value;
  }

  template <typename T = int> auto read_int() -> T {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    bool negative = it != end && *it == '-';
    if (negative) ++it;
    scan_ctx_.advance_to(it);
    const auto value = read_uint<typename std::make_unsigned<T>::type>();
    if (negative) return -static_cast<T>(value);
    return static_cast<T>(value);
  }

 public:
  FMT_CONSTEXPR scan_handler(string_view format, scan_buffer& buf,
                             scan_args args)
      : parse_ctx_(format), scan_ctx_(buf), args_(args), next_arg_id_(0) {}

  auto pos() const -> scan_buffer::iterator { return scan_ctx_.begin(); }

  void on_text(const char* begin, const char* end) {
    auto it = scan_ctx_.begin(), scan_end = scan_ctx_.end();
    for (; begin != end; ++begin, ++it) {
      if (it == scan_end || *begin != *it) on_error("invalid input");
    }
    scan_ctx_.advance_to(it);
  }

  FMT_CONSTEXPR auto on_arg_id() -> int { return on_arg_id(next_arg_id_++); }
  FMT_CONSTEXPR auto on_arg_id(int id) -> int {
    if (id >= args_.size) on_error("argument index out of range");
    arg_ = args_.data[id];
    return id;
  }
  FMT_CONSTEXPR auto on_arg_id(string_view id) -> int {
    if (id.data()) on_error("invalid format");
    return 0;
  }

  void on_replacement_field(int, const char*) {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    switch (arg_.type) {
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
      while (it != end && *it != ' ') arg_.string->push_back(*it++);
      scan_ctx_.advance_to(it);
      break;
    case scan_type::string_view_type: {
      auto range = to_contiguous(it);
      // This could also be checked at compile time in scan.
      if (!range) on_error("string_view requires contiguous input");
      auto p = range.begin;
      while (p != range.end && *p != ' ') ++p;
      size_t size = to_unsigned(p - range.begin);
      *arg_.string_view = {range.begin, size};
      advance(it, size);
      scan_ctx_.advance_to(it);
      break;
    }
    case scan_type::none_type:
    case scan_type::custom_type:
      assert(false);
    }
  }

  auto on_format_specs(int, const char* begin, const char*) -> const char* {
    if (arg_.type != scan_type::custom_type) return begin;
    parse_ctx_.advance_to(begin);
    arg_.custom.scan(arg_.custom.value, parse_ctx_, scan_ctx_);
    return parse_ctx_.begin();
  }
};
}  // namespace detail

template <typename... T>
auto make_scan_args(T&... args) -> std::array<detail::scan_arg, sizeof...(T)> {
  return {{args...}};
}

void vscan(detail::scan_buffer& buf, string_view fmt, scan_args args) {
  auto h = detail::scan_handler(fmt, buf, args);
  detail::parse_format_string<false>(fmt, h);
}

template <typename... T>
auto scan(string_view input, string_view fmt, T&... args)
    -> string_view::iterator {
  auto&& buf = detail::string_scan_buffer(input);
  vscan(buf, fmt, make_scan_args(args...));
  return input.begin() + (buf.begin().base() - input.data());
}

template <typename... T> void scan(std::FILE* f, string_view fmt, T&... args) {
  auto&& buf = detail::file_scan_buffer(f);
  vscan(buf, fmt, make_scan_args(args...));
}

FMT_END_NAMESPACE
