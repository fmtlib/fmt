// Formatting library for C++ - experimental format string compilation
//
// Copyright (c) 2012 - present, Victor Zverovich and fmt contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COMPILE_H_
#define FMT_COMPILE_H_

#include <vector>
#include "format.h"

FMT_BEGIN_NAMESPACE
namespace internal {

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
    unsigned arg_index;
    basic_string_view<Char> str;
    replacement repl;

    FMT_CONSTEXPR value(unsigned index = 0) : arg_index(index) {}
    FMT_CONSTEXPR value(basic_string_view<Char> s) : str(s) {}
    FMT_CONSTEXPR value(replacement r) : repl(r) {}
  } val;
  // Position past the end of the argument id.
  const Char* arg_id_end = nullptr;

  FMT_CONSTEXPR format_part(kind k = kind::arg_index, value v = {})
      : part_kind(k), val(v) {}

  static FMT_CONSTEXPR format_part make_arg_index(unsigned index) {
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

  FMT_CONSTEXPR void on_arg_id() { ++num_parts; }
  FMT_CONSTEXPR void on_arg_id(unsigned) { ++num_parts; }
  FMT_CONSTEXPR void on_arg_id(basic_string_view<Char>) { ++num_parts; }

  FMT_CONSTEXPR void on_replacement_field(const Char*) {}

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin,
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
  basic_parse_context<Char> parse_context_;

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

  FMT_CONSTEXPR void on_arg_id() {
    part_ = part::make_arg_index(parse_context_.next_arg_id());
  }

  FMT_CONSTEXPR void on_arg_id(unsigned id) {
    parse_context_.check_arg_id(id);
    part_ = part::make_arg_index(id);
  }

  FMT_CONSTEXPR void on_arg_id(basic_string_view<Char> id) {
    part_ = part::make_arg_name(id);
  }

  FMT_CONSTEXPR void on_replacement_field(const Char* ptr) {
    part_.arg_id_end = ptr;
    handler_(part_);
  }

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin,
                                            const Char* end) {
    auto repl = typename part::replacement();
    dynamic_specs_handler<basic_parse_context<Char>> handler(repl.specs,
                                                             parse_context_);
    auto it = parse_format_specs(begin, end, handler);
    if (*it != '}') on_error("missing '}' in format string");
    repl.arg_id = part_.part_kind == part::kind::arg_index
                      ? arg_ref<Char>(part_.val.arg_index)
                      : arg_ref<Char>(part_.val.str);
    auto part = part::make_replacement(repl);
    part.arg_id_end = begin;
    handler_(part);
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

template <typename Range, typename Context, typename Id>
void format_arg(basic_parse_context<typename Range::value_type>& parse_ctx,
                Context& ctx, Id arg_id) {
  ctx.advance_to(
      visit_format_arg(arg_formatter<Range>(ctx, &parse_ctx), ctx.arg(arg_id)));
}

// vformat_to is defined in a subnamespace to prevent ADL.
namespace cf {
template <typename Context, typename Range, typename CompiledFormat>
auto vformat_to(Range out, CompiledFormat& cf, basic_format_args<Context> args)
    -> typename Context::iterator {
  using char_type = typename Context::char_type;
  basic_parse_context<char_type> parse_ctx(to_string_view(cf.format_str_));
  Context ctx(out.begin(), args);

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
      internal::format_arg<Range>(parse_ctx, ctx, value.arg_index);
      break;

    case format_part_t::kind::arg_name:
      advance_to(parse_ctx, part.arg_id_end);
      internal::format_arg<Range>(parse_ctx, ctx, value.str);
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
      ctx.advance_to(
          visit_format_arg(arg_formatter<Range>(ctx, nullptr, &specs), arg));
      break;
    }
    }
  }
  return ctx.out();
}
}  // namespace cf

template <typename S, typename = void> struct compiled_format_base {
  using char_type = char_t<S>;
  using parts_container = std::vector<internal::format_part<char_type>>;

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
struct compiled_format_base<S, enable_if_t<is_compile_string<S>::value>> {
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
            internal::to_string_view(S()));
    return compiled_parts.data;
  }
};

template <typename S, typename... Args>
class compiled_format : private compiled_format_base<S> {
 public:
  using typename compiled_format_base<S>::char_type;

 private:
  basic_string_view<char_type> format_str_;

  template <typename Context, typename Range, typename CompiledFormat>
  friend auto cf::vformat_to(Range out, CompiledFormat& cf,
                             basic_format_args<Context> args) ->
      typename Context::iterator;

 public:
  compiled_format() = delete;
  explicit constexpr compiled_format(basic_string_view<char_type> format_str)
      : compiled_format_base<S>(format_str), format_str_(format_str) {}
};
}  // namespace internal

#if FMT_USE_CONSTEXPR
template <typename... Args, typename S,
          FMT_ENABLE_IF(is_compile_string<S>::value)>
FMT_CONSTEXPR auto compile(S format_str)
    -> internal::compiled_format<S, Args...> {
  return internal::compiled_format<S, Args...>(to_string_view(format_str));
}
#endif

// Compiles the format string which must be a string literal.
template <typename... Args, typename Char, size_t N>
auto compile(const Char (&format_str)[N])
    -> internal::compiled_format<const Char*, Args...> {
  return internal::compiled_format<const Char*, Args...>(
      basic_string_view<Char>(format_str, N - 1));
}

template <typename CompiledFormat, typename... Args,
          typename Char = typename CompiledFormat::char_type>
std::basic_string<Char> format(const CompiledFormat& cf, const Args&... args) {
  basic_memory_buffer<Char> buffer;
  using range = buffer_range<Char>;
  using context = buffer_context<Char>;
  internal::cf::vformat_to<context>(range(buffer), cf,
                                    {make_format_args<context>(args...)});
  return to_string(buffer);
}

template <typename OutputIt, typename CompiledFormat, typename... Args>
OutputIt format_to(OutputIt out, const CompiledFormat& cf,
                   const Args&... args) {
  using char_type = typename CompiledFormat::char_type;
  using range = internal::output_range<OutputIt, char_type>;
  using context = format_context_t<OutputIt, char_type>;
  return internal::cf::vformat_to<context>(
      range(out), cf, {make_format_args<context>(args...)});
}

template <typename OutputIt, typename CompiledFormat, typename... Args,
          FMT_ENABLE_IF(internal::is_output_iterator<OutputIt>::value)>
format_to_n_result<OutputIt> format_to_n(OutputIt out, size_t n,
                                         const CompiledFormat& cf,
                                         const Args&... args) {
  auto it =
      format_to(internal::truncating_iterator<OutputIt>(out, n), cf, args...);
  return {it.base(), it.count()};
}

template <typename CompiledFormat, typename... Args>
std::size_t formatted_size(const CompiledFormat& cf, const Args&... args) {
  return format_to(
             internal::counting_iterator<typename CompiledFormat::char_type>(),
             cf, args...)
      .count();
}

FMT_END_NAMESPACE

#endif  // FMT_COMPILE_H_
