// Formatting library for C++ - experimental format string compilation
//
// Copyright (c) 2012 - present, Victor Zverovich and fmt contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COMPILE_H_
#define FMT_COMPILE_H_

#include <algorithm>
#include <vector>

#include "format.h"

#ifndef FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
#  if defined(__cpp_nontype_template_parameter_class) && \
      (!FMT_GCC_VERSION || FMT_GCC_VERSION >= 903)
#    define FMT_USE_NONTYPE_TEMPLATE_PARAMETERS 1
#  else
#    define FMT_USE_NONTYPE_TEMPLATE_PARAMETERS 0
#  endif
#endif

FMT_BEGIN_NAMESPACE
namespace detail {

template <typename OutputIt> class truncating_iterator_base {
 protected:
  OutputIt out_;
  size_t limit_;
  size_t count_ = 0;

  truncating_iterator_base() : out_(), limit_(0) {}

  truncating_iterator_base(OutputIt out, size_t limit)
      : out_(out), limit_(limit) {}

 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = typename std::iterator_traits<OutputIt>::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using _Unchecked_type =
      truncating_iterator_base;  // Mark iterator as checked.

  OutputIt base() const { return out_; }
  size_t count() const { return count_; }
};

// An output iterator that truncates the output and counts the number of objects
// written to it.
template <typename OutputIt,
          typename Enable = typename std::is_void<
              typename std::iterator_traits<OutputIt>::value_type>::type>
class truncating_iterator;

template <typename OutputIt>
class truncating_iterator<OutputIt, std::false_type>
    : public truncating_iterator_base<OutputIt> {
  mutable typename truncating_iterator_base<OutputIt>::value_type blackhole_;

 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;

  truncating_iterator() = default;

  truncating_iterator(OutputIt out, size_t limit)
      : truncating_iterator_base<OutputIt>(out, limit) {}

  truncating_iterator& operator++() {
    if (this->count_++ < this->limit_) ++this->out_;
    return *this;
  }

  truncating_iterator operator++(int) {
    auto it = *this;
    ++*this;
    return it;
  }

  value_type& operator*() const {
    return this->count_ < this->limit_ ? *this->out_ : blackhole_;
  }
};

template <typename OutputIt>
class truncating_iterator<OutputIt, std::true_type>
    : public truncating_iterator_base<OutputIt> {
 public:
  truncating_iterator() = default;

  truncating_iterator(OutputIt out, size_t limit)
      : truncating_iterator_base<OutputIt>(out, limit) {}

  template <typename T> truncating_iterator& operator=(T val) {
    if (this->count_++ < this->limit_) *this->out_++ = val;
    return *this;
  }

  truncating_iterator& operator++() { return *this; }
  truncating_iterator& operator++(int) { return *this; }
  truncating_iterator& operator*() { return *this; }
};

// A compile-time string which is compiled into fast formatting code.
class compiled_string {};

template <typename S>
struct is_compiled_string : std::is_base_of<compiled_string, S> {};

/**
  \rst
  Converts a string literal *s* into a format string that will be parsed at
  compile time and converted into efficient formatting code. Requires C++17
  ``constexpr if`` compiler support.

  **Example**::

    // Converts 42 into std::string using the most efficient method and no
    // runtime format string processing.
    std::string s = fmt::format(FMT_COMPILE("{}"), 42);
  \endrst
 */
#define FMT_COMPILE(s) FMT_STRING_IMPL(s, fmt::detail::compiled_string)

#if FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
template <typename Char, size_t N> struct fixed_string {
  constexpr fixed_string(const Char (&str)[N]) {
    copy_str<Char, const Char*, Char*>(static_cast<const Char*>(str), str + N,
                                       data);
  }
  Char data[N]{};
};

template <typename Char, size_t N, fixed_string<Char, N> Str>
struct udl_compiled_string : compiled_string {
  using char_type = Char;
  constexpr operator basic_string_view<char_type>() const {
    return {Str.data, N - 1};
  }
};
#endif

template <typename T, typename... Tail>
const T& first(const T& value, const Tail&...) {
  return value;
}

// Part of a compiled format string. It can be either literal text or a
// replacement field.
template <typename Char> struct format_part {
  enum class kind { arg_index, arg_name, text, replacement };

  struct replacement {
    arg_ref<Char> arg_id;
    dynamic_format_specs<Char> specs;
  };

  kind part_kind;
  union value {
    int arg_index;
    basic_string_view<Char> str;
    replacement repl;

    FMT_CONSTEXPR value(int index = 0) : arg_index(index) {}
    FMT_CONSTEXPR value(basic_string_view<Char> s) : str(s) {}
    FMT_CONSTEXPR value(replacement r) : repl(r) {}
  } val;
  // Position past the end of the argument id.
  const Char* arg_id_end = nullptr;

  FMT_CONSTEXPR format_part(kind k = kind::arg_index, value v = {})
      : part_kind(k), val(v) {}

  static FMT_CONSTEXPR format_part make_arg_index(int index) {
    return format_part(kind::arg_index, index);
  }
  static FMT_CONSTEXPR format_part make_arg_name(basic_string_view<Char> name) {
    return format_part(kind::arg_name, name);
  }
  static FMT_CONSTEXPR format_part make_text(basic_string_view<Char> text) {
    return format_part(kind::text, text);
  }
  static FMT_CONSTEXPR format_part make_replacement(replacement repl) {
    return format_part(kind::replacement, repl);
  }
};

template <typename Char> struct part_counter {
  unsigned num_parts = 0;

  FMT_CONSTEXPR void on_text(const Char* begin, const Char* end) {
    if (begin != end) ++num_parts;
  }

  FMT_CONSTEXPR int on_arg_id() { return ++num_parts, 0; }
  FMT_CONSTEXPR int on_arg_id(int) { return ++num_parts, 0; }
  FMT_CONSTEXPR int on_arg_id(basic_string_view<Char>) {
    return ++num_parts, 0;
  }

  FMT_CONSTEXPR void on_replacement_field(int, const Char*) {}

  FMT_CONSTEXPR const Char* on_format_specs(int, const Char* begin,
                                            const Char* end) {
    // Find the matching brace.
    unsigned brace_counter = 0;
    for (; begin != end; ++begin) {
      if (*begin == '{') {
        ++brace_counter;
      } else if (*begin == '}') {
        if (brace_counter == 0u) break;
        --brace_counter;
      }
    }
    return begin;
  }

  FMT_CONSTEXPR void on_error(const char*) {}
};

// Counts the number of parts in a format string.
template <typename Char>
FMT_CONSTEXPR unsigned count_parts(basic_string_view<Char> format_str) {
  part_counter<Char> counter;
  parse_format_string<true>(format_str, counter);
  return counter.num_parts;
}

template <typename Char, typename PartHandler>
class format_string_compiler : public error_handler {
 private:
  using part = format_part<Char>;

  PartHandler handler_;
  part part_;
  basic_string_view<Char> format_str_;
  basic_format_parse_context<Char> parse_context_;

 public:
  FMT_CONSTEXPR format_string_compiler(basic_string_view<Char> format_str,
                                       PartHandler handler)
      : handler_(handler),
        format_str_(format_str),
        parse_context_(format_str) {}

  FMT_CONSTEXPR void on_text(const Char* begin, const Char* end) {
    if (begin != end)
      handler_(part::make_text({begin, to_unsigned(end - begin)}));
  }

  FMT_CONSTEXPR int on_arg_id() {
    part_ = part::make_arg_index(parse_context_.next_arg_id());
    return 0;
  }

  FMT_CONSTEXPR int on_arg_id(int id) {
    parse_context_.check_arg_id(id);
    part_ = part::make_arg_index(id);
    return 0;
  }

  FMT_CONSTEXPR int on_arg_id(basic_string_view<Char> id) {
    part_ = part::make_arg_name(id);
    return 0;
  }

  FMT_CONSTEXPR void on_replacement_field(int, const Char* ptr) {
    part_.arg_id_end = ptr;
    handler_(part_);
  }

  FMT_CONSTEXPR const Char* on_format_specs(int, const Char* begin,
                                            const Char* end) {
    auto repl = typename part::replacement();
    dynamic_specs_handler<basic_format_parse_context<Char>> handler(
        repl.specs, parse_context_);
    auto it = parse_format_specs(begin, end, handler);
    if (*it != '}') on_error("missing '}' in format string");
    repl.arg_id = part_.part_kind == part::kind::arg_index
                      ? arg_ref<Char>(part_.val.arg_index)
                      : arg_ref<Char>(part_.val.str);
    auto replacement_part = part::make_replacement(repl);
    replacement_part.arg_id_end = begin;
    handler_(replacement_part);
    return it;
  }
};

// Compiles a format string and invokes handler(part) for each parsed part.
template <bool IS_CONSTEXPR, typename Char, typename PartHandler>
FMT_CONSTEXPR void compile_format_string(basic_string_view<Char> format_str,
                                         PartHandler handler) {
  parse_format_string<IS_CONSTEXPR>(
      format_str,
      format_string_compiler<Char, PartHandler>(format_str, handler));
}

template <typename OutputIt, typename Context, typename Id>
void format_arg(
    basic_format_parse_context<typename Context::char_type>& parse_ctx,
    Context& ctx, Id arg_id) {
  auto arg = ctx.arg(arg_id);
  if (arg.type() == type::custom_type) {
    visit_format_arg(custom_formatter<Context>(parse_ctx, ctx), arg);
  } else {
    ctx.advance_to(visit_format_arg(
        default_arg_formatter<OutputIt, typename Context::char_type>{
            ctx.out(), ctx.args(), ctx.locale()},
        arg));
  }
}

// vformat_to is defined in a subnamespace to prevent ADL.
namespace cf {
template <typename Context, typename OutputIt, typename CompiledFormat>
auto vformat_to(OutputIt out, CompiledFormat& cf,
                basic_format_args<Context> args) -> typename Context::iterator {
  using char_type = typename Context::char_type;
  basic_format_parse_context<char_type> parse_ctx(
      to_string_view(cf.format_str_));
  Context ctx(out, args);

  const auto& parts = cf.parts();
  for (auto part_it = std::begin(parts); part_it != std::end(parts);
       ++part_it) {
    const auto& part = *part_it;
    const auto& value = part.val;

    using format_part_t = format_part<char_type>;
    switch (part.part_kind) {
    case format_part_t::kind::text: {
      const auto text = value.str;
      auto output = ctx.out();
      auto&& it = reserve(output, text.size());
      it = std::copy_n(text.begin(), text.size(), it);
      ctx.advance_to(output);
      break;
    }

    case format_part_t::kind::arg_index:
      advance_to(parse_ctx, part.arg_id_end);
      detail::format_arg<OutputIt>(parse_ctx, ctx, value.arg_index);
      break;

    case format_part_t::kind::arg_name:
      advance_to(parse_ctx, part.arg_id_end);
      detail::format_arg<OutputIt>(parse_ctx, ctx, value.str);
      break;

    case format_part_t::kind::replacement: {
      const auto& arg_id_value = value.repl.arg_id.val;
      const auto arg = value.repl.arg_id.kind == arg_id_kind::index
                           ? ctx.arg(arg_id_value.index)
                           : ctx.arg(arg_id_value.name);

      auto specs = value.repl.specs;

      handle_dynamic_spec<width_checker>(specs.width, specs.width_ref, ctx);
      handle_dynamic_spec<precision_checker>(specs.precision,
                                             specs.precision_ref, ctx);

      error_handler h;
      numeric_specs_checker<error_handler> checker(h, arg.type());
      if (specs.align == align::numeric) checker.require_numeric_argument();
      if (specs.sign != sign::none) checker.check_sign();
      if (specs.alt) checker.require_numeric_argument();
      if (specs.precision >= 0) checker.check_precision();

      advance_to(parse_ctx, part.arg_id_end);
      ctx.advance_to(visit_format_arg(
          arg_formatter<OutputIt, typename Context::char_type>(ctx, specs),
          arg));
      break;
    }
    }
  }
  return ctx.out();
}
}  // namespace cf

struct basic_compiled_format {};

template <typename S, typename = void>
struct compiled_format_base : basic_compiled_format {
  using char_type = char_t<S>;
  using parts_container = std::vector<detail::format_part<char_type>>;

  parts_container compiled_parts;

  explicit compiled_format_base(basic_string_view<char_type> format_str) {
    compile_format_string<false>(format_str,
                                 [this](const format_part<char_type>& part) {
                                   compiled_parts.push_back(part);
                                 });
  }

  const parts_container& parts() const { return compiled_parts; }
};

template <typename Char, unsigned N> struct format_part_array {
  format_part<Char> data[N] = {};
  FMT_CONSTEXPR format_part_array() = default;
};

template <typename Char, unsigned N>
FMT_CONSTEXPR format_part_array<Char, N> compile_to_parts(
    basic_string_view<Char> format_str) {
  format_part_array<Char, N> parts;
  unsigned counter = 0;
  // This is not a lambda for compatibility with older compilers.
  struct {
    format_part<Char>* parts;
    unsigned* counter;
    FMT_CONSTEXPR void operator()(const format_part<Char>& part) {
      parts[(*counter)++] = part;
    }
  } collector{parts.data, &counter};
  compile_format_string<true>(format_str, collector);
  if (counter < N) {
    parts.data[counter] =
        format_part<Char>::make_text(basic_string_view<Char>());
  }
  return parts;
}

template <typename T> constexpr const T& constexpr_max(const T& a, const T& b) {
  return (a < b) ? b : a;
}

template <typename S>
struct compiled_format_base<S, enable_if_t<is_compile_string<S>::value>>
    : basic_compiled_format {
  using char_type = char_t<S>;

  FMT_CONSTEXPR explicit compiled_format_base(basic_string_view<char_type>) {}

// Workaround for old compilers. Format string compilation will not be
// performed there anyway.
#if FMT_USE_CONSTEXPR
  static FMT_CONSTEXPR_DECL const unsigned num_format_parts =
      constexpr_max(count_parts(to_string_view(S())), 1u);
#else
  static const unsigned num_format_parts = 1;
#endif

  using parts_container = format_part<char_type>[num_format_parts];

  const parts_container& parts() const {
    static FMT_CONSTEXPR_DECL const auto compiled_parts =
        compile_to_parts<char_type, num_format_parts>(
            detail::to_string_view(S()));
    return compiled_parts.data;
  }
};

template <typename S, typename... Args>
class compiled_format : private compiled_format_base<S> {
 public:
  using typename compiled_format_base<S>::char_type;

 private:
  basic_string_view<char_type> format_str_;

  template <typename Context, typename OutputIt, typename CompiledFormat>
  friend auto cf::vformat_to(OutputIt out, CompiledFormat& cf,
                             basic_format_args<Context> args) ->
      typename Context::iterator;

 public:
  compiled_format() = delete;
  explicit constexpr compiled_format(basic_string_view<char_type> format_str)
      : compiled_format_base<S>(format_str), format_str_(format_str) {}
};

#ifdef __cpp_if_constexpr
template <typename... Args> struct type_list {};

// Returns a reference to the argument at index N from [first, rest...].
template <int N, typename T, typename... Args>
constexpr const auto& get([[maybe_unused]] const T& first,
                          [[maybe_unused]] const Args&... rest) {
  static_assert(N < 1 + sizeof...(Args), "index is out of bounds");
  if constexpr (N == 0)
    return first;
  else
    return get<N - 1>(rest...);
}

template <int N, typename> struct get_type_impl;

template <int N, typename... Args> struct get_type_impl<N, type_list<Args...>> {
  using type = remove_cvref_t<decltype(get<N>(std::declval<Args>()...))>;
};

template <int N, typename T>
using get_type = typename get_type_impl<N, T>::type;

template <typename T> struct is_compiled_format : std::false_type {};

template <typename Char> struct text {
  basic_string_view<Char> data;
  using char_type = Char;

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&...) const {
    return write<Char>(out, data);
  }
};

template <typename Char>
struct is_compiled_format<text<Char>> : std::true_type {};

template <typename Char>
constexpr text<Char> make_text(basic_string_view<Char> s, size_t pos,
                               size_t size) {
  return {{&s[pos], size}};
}

template <typename Char> struct code_unit {
  Char value;
  using char_type = Char;

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&...) const {
    return write<Char>(out, value);
  }
};

template <typename Char>
struct is_compiled_format<code_unit<Char>> : std::true_type {};

// A replacement field that refers to argument N.
template <typename Char, typename T, int N> struct field {
  using char_type = Char;

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&... args) const {
    if constexpr (is_named_arg<typename std::remove_cv<T>::type>::value) {
      const auto& arg = get<N>(args...).value;
      return write<Char>(out, arg);
    } else {
      // This ensures that the argument type is convertile to `const T&`.
      const T& arg = get<N>(args...);
      return write<Char>(out, arg);
    }
  }
};

template <typename Char, typename T, int N>
struct is_compiled_format<field<Char, T, N>> : std::true_type {};

// A replacement field that refers to argument with name.
template <typename Char> struct runtime_named_field {
  using char_type = Char;
  basic_string_view<Char> name;

  template <typename OutputIt, typename T>
  constexpr static bool try_format_argument(
      OutputIt& out,
      // [[maybe_unused]] due to unused-but-set-parameter warning in GCC 7,8,9
      [[maybe_unused]] basic_string_view<Char> arg_name, const T& arg) {
    if constexpr (is_named_arg<typename std::remove_cv<T>::type>::value) {
      if (arg_name == arg.name) {
        out = write<Char>(out, arg.value);
        return true;
      }
    }
    return false;
  }

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&... args) const {
    bool found = (try_format_argument(out, name, args) || ...);
    if (!found) {
      throw format_error("argument with specified name is not found");
    }
    return out;
  }
};

template <typename Char>
struct is_compiled_format<runtime_named_field<Char>> : std::true_type {};

// A replacement field that refers to argument N and has format specifiers.
template <typename Char, typename T, int N> struct spec_field {
  using char_type = Char;
  formatter<T, Char> fmt;

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&... args) const {
    // This ensures that the argument type is convertile to `const T&`.
    const T& arg = get<N>(args...);
    const auto& vargs =
        make_format_args<basic_format_context<OutputIt, Char>>(args...);
    basic_format_context<OutputIt, Char> ctx(out, vargs);
    return fmt.format(arg, ctx);
  }
};

template <typename Char, typename T, int N>
struct is_compiled_format<spec_field<Char, T, N>> : std::true_type {};

template <typename L, typename R> struct concat {
  L lhs;
  R rhs;
  using char_type = typename L::char_type;

  template <typename OutputIt, typename... Args>
  constexpr OutputIt format(OutputIt out, const Args&... args) const {
    out = lhs.format(out, args...);
    return rhs.format(out, args...);
  }
};

template <typename L, typename R>
struct is_compiled_format<concat<L, R>> : std::true_type {};

template <typename L, typename R>
constexpr concat<L, R> make_concat(L lhs, R rhs) {
  return {lhs, rhs};
}

struct unknown_format {};

template <typename Char>
constexpr size_t parse_text(basic_string_view<Char> str, size_t pos) {
  for (size_t size = str.size(); pos != size; ++pos) {
    if (str[pos] == '{' || str[pos] == '}') break;
  }
  return pos;
}

template <typename Args, size_t POS, int ID, typename S>
constexpr auto compile_format_string(S format_str);

template <typename Args, size_t POS, int ID, typename T, typename S>
constexpr auto parse_tail(T head, S format_str) {
  if constexpr (POS !=
                basic_string_view<typename S::char_type>(format_str).size()) {
    constexpr auto tail = compile_format_string<Args, POS, ID>(format_str);
    if constexpr (std::is_same<remove_cvref_t<decltype(tail)>,
                               unknown_format>())
      return tail;
    else
      return make_concat(head, tail);
  } else {
    return head;
  }
}

template <typename T, typename Char> struct parse_specs_result {
  formatter<T, Char> fmt;
  size_t end;
  int next_arg_id;
};

constexpr int manual_indexing_id = -1;

template <typename T, typename Char>
constexpr parse_specs_result<T, Char> parse_specs(basic_string_view<Char> str,
                                                  size_t pos, int next_arg_id) {
  str.remove_prefix(pos);
  auto ctx = basic_format_parse_context<Char>(str, {}, next_arg_id);
  auto f = formatter<T, Char>();
  auto end = f.parse(ctx);
  return {f, pos + fmt::detail::to_unsigned(end - str.data()) + 1,
          next_arg_id == 0 ? manual_indexing_id : ctx.next_arg_id()};
}

template <typename Char> struct arg_id_handler {
  constexpr void on_error(const char* message) { throw format_error(message); }

  constexpr int on_arg_id() {
    FMT_ASSERT(false, "handler cannot be used with automatic indexing");
    return 0;
  }

  constexpr int on_arg_id(int id) {
    arg_id = arg_ref<Char>(id);
    return 0;
  }

  constexpr int on_arg_id(basic_string_view<Char> id) {
    arg_id = arg_ref<Char>(id);
    return 0;
  }

  arg_ref<Char> arg_id;
};

template <typename Char> struct parse_arg_id_result {
  arg_ref<Char> arg_id;
  const Char* arg_id_end;
};

template <int ID, typename Char>
constexpr auto parse_arg_id(const Char* begin, const Char* end) {
  auto handler = arg_id_handler<Char>{arg_ref<Char>{}};
  auto adapter = id_adapter<arg_id_handler<Char>, Char>{handler, 0};
  auto arg_id_end = parse_arg_id(begin, end, adapter);
  return parse_arg_id_result<Char>{handler.arg_id, arg_id_end};
}

// Compiles a non-empty format string and returns the compiled representation
// or unknown_format() on unrecognized input.
template <typename Args, size_t POS, int ID, typename S>
constexpr auto compile_format_string(S format_str) {
  using char_type = typename S::char_type;
  constexpr basic_string_view<char_type> str = format_str;
  if constexpr (str[POS] == '{') {
    if constexpr (POS + 1 == str.size())
      throw format_error("unmatched '{' in format string");
    if constexpr (str[POS + 1] == '{') {
      return parse_tail<Args, POS + 2, ID>(make_text(str, POS, 1), format_str);
    } else if constexpr (str[POS + 1] == '}' || str[POS + 1] == ':') {
      static_assert(ID != manual_indexing_id,
                    "cannot switch from manual to automatic argument indexing");
      using id_type = get_type<ID, Args>;
      if constexpr (str[POS + 1] == '}') {
        constexpr auto next_id =
            ID != manual_indexing_id ? ID + 1 : manual_indexing_id;
        return parse_tail<Args, POS + 2, next_id>(
            field<char_type, id_type, ID>(), format_str);
      } else {
        constexpr auto result = parse_specs<id_type>(str, POS + 2, ID + 1);
        return parse_tail<Args, result.end, result.next_arg_id>(
            spec_field<char_type, id_type, ID>{result.fmt}, format_str);
      }
    } else {
      constexpr auto arg_id_result =
          parse_arg_id<ID>(str.data() + POS + 1, str.data() + str.size());
      constexpr auto arg_id_end_pos = arg_id_result.arg_id_end - str.data();
      constexpr char_type c =
          arg_id_end_pos != str.size() ? str[arg_id_end_pos] : char_type();
      static_assert(c == '}' || c == ':', "missing '}' in format string");
      if constexpr (arg_id_result.arg_id.kind == arg_id_kind::index) {
        static_assert(
            ID == manual_indexing_id || ID == 0,
            "cannot switch from automatic to manual argument indexing");
        constexpr auto arg_index = arg_id_result.arg_id.val.index;
        using id_type = get_type<arg_index, Args>;
        if constexpr (c == '}') {
          return parse_tail<Args, arg_id_end_pos + 1, manual_indexing_id>(
              field<char_type, id_type, arg_index>(), format_str);
        } else if constexpr (c == ':') {
          constexpr auto result =
              parse_specs<id_type>(str, arg_id_end_pos + 1, 0);
          return parse_tail<Args, result.end, result.next_arg_id>(
              spec_field<char_type, id_type, arg_index>{result.fmt},
              format_str);
        }
      } else if constexpr (arg_id_result.arg_id.kind == arg_id_kind::name) {
        if constexpr (c == '}') {
          return parse_tail<Args, arg_id_end_pos + 1, ID>(
              runtime_named_field<char_type>{arg_id_result.arg_id.val.name},
              format_str);
        } else if constexpr (c == ':') {
          return unknown_format();  // no type info for specs parsing
        }
      }
    }
  } else if constexpr (str[POS] == '}') {
    if constexpr (POS + 1 == str.size())
      throw format_error("unmatched '}' in format string");
    return parse_tail<Args, POS + 2, ID>(make_text(str, POS, 1), format_str);
  } else {
    constexpr auto end = parse_text(str, POS + 1);
    if constexpr (end - POS > 1) {
      return parse_tail<Args, end, ID>(make_text(str, POS, end - POS),
                                       format_str);
    } else {
      return parse_tail<Args, end, ID>(code_unit<char_type>{str[POS]},
                                       format_str);
    }
  }
}

template <typename... Args, typename S,
          FMT_ENABLE_IF(is_compile_string<S>::value ||
                        detail::is_compiled_string<S>::value)>
constexpr auto compile(S format_str) {
  constexpr basic_string_view<typename S::char_type> str = format_str;
  if constexpr (str.size() == 0) {
    return detail::make_text(str, 0, 0);
  } else {
    constexpr auto result =
        detail::compile_format_string<detail::type_list<Args...>, 0, 0>(
            format_str);
    return result;
  }
}
#else
template <typename... Args, typename S,
          FMT_ENABLE_IF(is_compile_string<S>::value)>
constexpr auto compile(S format_str) -> detail::compiled_format<S, Args...> {
  return detail::compiled_format<S, Args...>(to_string_view(format_str));
}
#endif  // __cpp_if_constexpr

// Compiles the format string which must be a string literal.
template <typename... Args, typename Char, size_t N>
auto compile(const Char (&format_str)[N])
    -> detail::compiled_format<const Char*, Args...> {
  return detail::compiled_format<const Char*, Args...>(
      basic_string_view<Char>(format_str, N - 1));
}
}  // namespace detail

// DEPRECATED! use FMT_COMPILE instead.
template <typename... Args>
FMT_DEPRECATED auto compile(const Args&... args)
    -> decltype(detail::compile(args...)) {
  return detail::compile(args...);
}

FMT_MODULE_EXPORT_BEGIN

#if FMT_USE_CONSTEXPR
#  ifdef __cpp_if_constexpr

template <typename CompiledFormat, typename... Args,
          typename Char = typename CompiledFormat::char_type,
          FMT_ENABLE_IF(detail::is_compiled_format<CompiledFormat>::value)>
FMT_INLINE std::basic_string<Char> format(const CompiledFormat& cf,
                                          const Args&... args) {
  basic_memory_buffer<Char> buffer;
  cf.format(detail::buffer_appender<Char>(buffer), args...);
  return to_string(buffer);
}

template <typename OutputIt, typename CompiledFormat, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_format<CompiledFormat>::value)>
constexpr OutputIt format_to(OutputIt out, const CompiledFormat& cf,
                             const Args&... args) {
  return cf.format(out, args...);
}
#  endif  // __cpp_if_constexpr
#endif    // FMT_USE_CONSTEXPR

template <typename CompiledFormat, typename... Args,
          typename Char = typename CompiledFormat::char_type,
          FMT_ENABLE_IF(std::is_base_of<detail::basic_compiled_format,
                                        CompiledFormat>::value)>
std::basic_string<Char> format(const CompiledFormat& cf, const Args&... args) {
  basic_memory_buffer<Char> buffer;
  using context = buffer_context<Char>;
  detail::cf::vformat_to<context>(detail::buffer_appender<Char>(buffer), cf,
                                  make_format_args<context>(args...));
  return to_string(buffer);
}

template <typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
FMT_INLINE std::basic_string<typename S::char_type> format(const S&,
                                                           Args&&... args) {
#ifdef __cpp_if_constexpr
  if constexpr (std::is_same<typename S::char_type, char>::value) {
    constexpr basic_string_view<typename S::char_type> str = S();
    if constexpr (str.size() == 2 && str[0] == '{' && str[1] == '}') {
      const auto& first = detail::first(args...);
      if constexpr (detail::is_named_arg<
                        remove_cvref_t<decltype(first)>>::value) {
        return fmt::to_string(first.value);
      } else {
        return fmt::to_string(first);
      }
    }
  }
#endif
  constexpr auto compiled = detail::compile<Args...>(S());
#ifdef __cpp_if_constexpr
  if constexpr (std::is_same<remove_cvref_t<decltype(compiled)>,
                             detail::unknown_format>()) {
    return format(static_cast<basic_string_view<typename S::char_type>>(S()),
                  std::forward<Args>(args)...);
  } else {
    return format(compiled, std::forward<Args>(args)...);
  }
#else
  return format(compiled, std::forward<Args>(args)...);
#endif
}

template <typename OutputIt, typename CompiledFormat, typename... Args,
          FMT_ENABLE_IF(std::is_base_of<detail::basic_compiled_format,
                                        CompiledFormat>::value)>
constexpr OutputIt format_to(OutputIt out, const CompiledFormat& cf,
                             const Args&... args) {
  using char_type = typename CompiledFormat::char_type;
  using context = format_context_t<OutputIt, char_type>;
  return detail::cf::vformat_to<context>(out, cf,
                                         make_format_args<context>(args...));
}

template <typename OutputIt, typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
FMT_CONSTEXPR OutputIt format_to(OutputIt out, const S&, Args&&... args) {
  constexpr auto compiled = detail::compile<Args...>(S());
#ifdef __cpp_if_constexpr
  if constexpr (std::is_same<remove_cvref_t<decltype(compiled)>,
                             detail::unknown_format>()) {
    return format_to(out,
                     static_cast<basic_string_view<typename S::char_type>>(S()),
                     std::forward<Args>(args)...);
  } else {
    return format_to(out, compiled, std::forward<Args>(args)...);
  }
#else
  return format_to(out, compiled, std::forward<Args>(args)...);
#endif
}

template <typename OutputIt, typename CompiledFormat, typename... Args>
auto format_to_n(OutputIt out, size_t n, const CompiledFormat& cf,
                 const Args&... args) ->
    typename std::enable_if<
        detail::is_output_iterator<OutputIt,
                                   typename CompiledFormat::char_type>::value &&
            std::is_base_of<detail::basic_compiled_format,
                            CompiledFormat>::value,
        format_to_n_result<OutputIt>>::type {
  auto it =
      format_to(detail::truncating_iterator<OutputIt>(out, n), cf, args...);
  return {it.base(), it.count()};
}

template <typename OutputIt, typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
format_to_n_result<OutputIt> format_to_n(OutputIt out, size_t n, const S&,
                                         Args&&... args) {
  auto it = format_to(detail::truncating_iterator<OutputIt>(out, n), S(),
                      std::forward<Args>(args)...);
  return {it.base(), it.count()};
}

template <typename CompiledFormat, typename... Args,
          FMT_ENABLE_IF(std::is_base_of<detail::basic_compiled_format,
                                        CompiledFormat>::value ||
                        detail::is_compiled_string<CompiledFormat>::value)>
size_t formatted_size(const CompiledFormat& cf, const Args&... args) {
  return format_to(detail::counting_iterator(), cf, args...).count();
}

#if FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
inline namespace literals {
template <detail::fixed_string Str>
constexpr detail::udl_compiled_string<
    remove_cvref_t<decltype(Str.data[0])>,
    sizeof(Str.data) / sizeof(decltype(Str.data[0])), Str>
operator""_cf() {
  return {};
}
}  // namespace literals
#endif

FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_COMPILE_H_
