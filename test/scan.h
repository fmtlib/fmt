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

inline bool is_whitespace(char c) { return c == ' ' || c == '\n'; }

template <typename T> class optional {
 private:
  T value_;
  bool has_value_ = false;

 public:
  optional() = default;
  optional(T value) : value_(std::move(value)), has_value_(true) {}

  explicit operator bool() const { return has_value_; }

  auto operator*() const -> const T& {
    if (!has_value_) throw std::runtime_error("bad optional access");
    return value_;
  }
};

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

  void set(string_view buf) {
    ptr_ = buf.begin();
    end_ = buf.end();
  }

  auto ptr() const -> const char* { return ptr_; }

 public:
  scan_buffer(const scan_buffer&) = delete;
  void operator=(const scan_buffer&) = delete;

  // Fills the buffer with more input if available.
  virtual void consume() = 0;

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

    iterator(scan_buffer* buf) : buf_(buf) {
      if (buf->ptr_ == buf->end_) {
        ptr_ = sentinel();
        return;
      }
      ptr_ = &buf->ptr_;
      value_ = *buf->ptr_;
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

  auto begin() -> iterator { return this; }
  auto end() -> iterator { return {}; }

  auto is_contiguous() const -> bool { return contiguous_; }

  // Tries consuming a single code unit. Returns true iff there is more input.
  auto try_consume() -> bool {
    FMT_ASSERT(ptr_ != end_, "");
    ++ptr_;
    if (ptr_ != end_) return true;
    consume();
    return ptr_ != end_;
  }
};

class string_scan_buffer : public scan_buffer {
 private:
  void consume() override {}

 public:
  explicit string_scan_buffer(string_view s)
      : scan_buffer(s.begin(), s.end(), true) {}
};

#ifdef _WIN32
void flockfile(FILE* f) { _lock_file(f); }
void funlockfile(FILE* f) { _unlock_file(f); }
int getc_unlocked(FILE *f) { return _fgetc_nolock(f); }
#endif

// A FILE wrapper. F is FILE defined as a template parameter to make
// system-specific API detection work.
template <typename F> class file_base {
 protected:
  F* file_;

 public:
  file_base(F* file) : file_(file) {}
  operator F*() const { return file_; }

  // Reads a code unit from the stream.
  auto get() -> int {
    int result = getc_unlocked(file_);
    if (result == EOF && ferror(file_) != 0)
      FMT_THROW(system_error(errno, FMT_STRING("getc failed")));
    return result;
  }

  // Puts the code unit back into the stream buffer.
  void unget(char c) {
    if (ungetc(c, file_) == EOF)
      FMT_THROW(system_error(errno, FMT_STRING("ungetc failed")));
  }
};

// A FILE wrapper for glibc.
template <typename F> class glibc_file : public file_base<F> {
 public:
  using file_base<F>::file_base;

  // Returns the file's read buffer as a string_view.
  auto buffer() const -> string_view {
    return {this->file_->_IO_read_ptr,
            to_unsigned(this->file_->_IO_read_end - this->file_->_IO_read_ptr)};
  }
};

// A FILE wrapper for Apple's libc.
template <typename F> class apple_file : public file_base<F> {
 public:
  using file_base<F>::file_base;

  auto buffer() const -> string_view {
    return {reinterpret_cast<char*>(this->file_->_p),
            to_unsigned(this->file_->_r)};
  }
};

// A fallback FILE wrapper.
template <typename F> class fallback_file : public file_base<F> {
 private:
  char next_;  // The next unconsumed character in the buffer.
  bool has_next_ = false;

 public:
  using file_base<F>::file_base;

  auto buffer() const -> string_view { return {&next_, has_next_ ? 1u : 0u}; }

  auto get() -> int {
    has_next_ = false;
    return file_base<F>::get();
  }

  void unget(char c) {
    file_base<F>::unget(c);
    next_ = c;
    has_next_ = true;
  }
};

class file_scan_buffer : public scan_buffer {
 private:
  template <typename F, FMT_ENABLE_IF(sizeof(F::_IO_read_ptr) != 0)>
  static auto get_file(F* f, int) -> glibc_file<F> {
    return f;
  }
  template <typename F, FMT_ENABLE_IF(sizeof(F::_p) != 0)>
  static auto get_file(F* f, int) -> apple_file<F> {
    return f;
  }
  static auto get_file(FILE* f, ...) -> fallback_file<FILE> { return f; }

  decltype(get_file(static_cast<FILE*>(nullptr), 0)) file_;

  // Fills the buffer if it is empty.
  void fill() {
    string_view buf = file_.buffer();
    if (buf.size() == 0) {
      int c = file_.get();
      // Put the character back since we are only filling the buffer.
      if (c != EOF) file_.unget(static_cast<char>(c));
      buf = file_.buffer();
    }
    set(buf);
  }

  void consume() override {
    // Consume the current buffer content.
    size_t n = to_unsigned(ptr() - file_.buffer().begin());
    for (size_t i = 0; i != n; ++i) file_.get();
    fill();
  }

 public:
  explicit file_scan_buffer(FILE* f)
      : scan_buffer(nullptr, nullptr, false), file_(f) {
    flockfile(f);
    fill();
  }
  ~file_scan_buffer() { funlockfile(file_); }
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

  void advance_to(iterator) { buf_.consume(); }
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

  template <typename T = unsigned> auto read_uint() -> optional<T> {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    if (it == end) return {};
    char c = *it;
    if (c < '0' || c > '9') on_error("invalid input");

    int num_digits = 0;
    T value = 0, prev = 0;
    char prev_digit = c;
    do {
      prev = value;
      value = value * 10 + static_cast<unsigned>(c - '0');
      prev_digit = c;
      c = *++it;
      ++num_digits;
      if (c < '0' || c > '9') break;
    } while (it != end);
    scan_ctx_.advance_to(it);

    // Check overflow.
    if (num_digits <= std::numeric_limits<int>::digits10) return value;
    const unsigned max = to_unsigned((std::numeric_limits<int>::max)());
    if (num_digits == std::numeric_limits<int>::digits10 + 1 &&
        prev * 10ull + unsigned(prev_digit - '0') <= max) {
      return value;
    }
    throw format_error("number is too big");
  }

  template <typename T = int> auto read_int() -> optional<T> {
    auto it = scan_ctx_.begin(), end = scan_ctx_.end();
    bool negative = it != end && *it == '-';
    if (negative) {
      ++it;
      scan_ctx_.advance_to(it);
    }
    if (auto abs_value = read_uint<typename std::make_unsigned<T>::type>()) {
      auto value = static_cast<T>(*abs_value);
      return negative ? -value : value;
    }
    if (negative) on_error("invalid input");
    return {};
  }

 public:
  FMT_CONSTEXPR scan_handler(string_view format, scan_buffer& buf,
                             scan_args args)
      : parse_ctx_(format), scan_ctx_(buf), args_(args), next_arg_id_(0) {}

  auto pos() const -> scan_buffer::iterator { return scan_ctx_.begin(); }

  void on_text(const char* begin, const char* end) {
    if (begin == end) return;
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
    while (it != end && is_whitespace(*it)) ++it;
    scan_ctx_.advance_to(it);
    switch (arg_.type) {
    case scan_type::int_type:
      if (auto value = read_int()) *arg_.int_value = *value;
      break;
    case scan_type::uint_type:
      if (auto value = read_uint()) *arg_.uint_value = *value;
      break;
    case scan_type::long_long_type:
      if (auto value = read_int<long long>()) *arg_.long_long_value = *value;
      break;
    case scan_type::ulong_long_type:
      if (auto value = read_uint<unsigned long long>())
        *arg_.ulong_long_value = *value;
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

  void on_error(const char* message) {
    scan_ctx_.advance_to(scan_ctx_.end());
    error_handler::on_error(message);
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

template <typename... T> bool scan(std::FILE* f, string_view fmt, T&... args) {
  auto&& buf = detail::file_scan_buffer(f);
  vscan(buf, fmt, make_scan_args(args...));
  return buf.begin() != buf.end();
}

FMT_END_NAMESPACE
