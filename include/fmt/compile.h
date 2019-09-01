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
  std::size_t arg_id_end = 0;  // Position past the end of the argument id.

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
    part_.arg_id_end = ptr - format_str_.begin();
    handler_(part_);
  }

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin,
                                            const Char* end) {
    const auto specs_offset = to_unsigned(begin - format_str_.begin());

    auto repl = typename part::replacement();
    dynamic_specs_handler<basic_parse_context<Char>> handler(repl.specs,
                                                             parse_context_);
    begin = parse_format_specs(begin, end, handler);
    if (*begin != '}') on_error("missing '}' in format string");

    repl.arg_id = part_.part_kind == part::kind::arg_index
                      ? arg_ref<Char>(part_.val.arg_index)
                      : arg_ref<Char>(part_.val.str);
    auto part = part::make_replacement(repl);
    part.arg_id_end = specs_offset;
    handler_(part);
    return begin;
  }

 private:
  PartHandler handler_;
  part part_;
  basic_string_view<Char> format_str_;
  basic_parse_context<Char> parse_context_;
};

// Compiles a format string and invokes handler(part) for each parsed part.
template <bool IS_CONSTEXPR, typename Char, typename PartHandler>
FMT_CONSTEXPR void compile_format_string(basic_string_view<Char> format_str,
                                         PartHandler handler) {
  parse_format_string<IS_CONSTEXPR>(
      format_str,
      format_string_compiler<Char, PartHandler>(format_str, handler));
}

template <typename Format, typename PreparedPartsProvider, typename... Args>
class compiled_format {
 public:
  using char_type = char_t<Format>;
  using format_part_t = format_part<char_type>;

  constexpr compiled_format(Format f)
      : format_(std::move(f)), parts_provider_(to_string_view(format_)) {}

  compiled_format() = delete;

  template <typename Range, typename Context>
  auto vformat_to(Range out, basic_format_args<Context> args) const ->
      typename Context::iterator {
    const auto format_view = internal::to_string_view(format_);
    basic_parse_context<char_type> parse_ctx(format_view);
    Context ctx(out.begin(), args);

    const auto& parts = parts_provider_.parts();
    for (auto part_it = parts.begin(); part_it != parts.end(); ++part_it) {
      const auto& part = *part_it;
      const auto& value = part.val;

      switch (part.part_kind) {
      case format_part_t::kind::text: {
        const auto text = value.str;
        auto output = ctx.out();
        auto&& it = internal::reserve(output, text.size());
        it = std::copy_n(text.begin(), text.size(), it);
        ctx.advance_to(output);
      } break;

      case format_part_t::kind::arg_index: {
        advance_parse_context_to_specification(parse_ctx, part);
        format_arg<Range>(parse_ctx, ctx, value.arg_index);
      } break;

      case format_part_t::kind::arg_name: {
        advance_parse_context_to_specification(parse_ctx, part);
        format_arg<Range>(parse_ctx, ctx, value.str);
      } break;
      case format_part_t::kind::replacement: {
        const auto& arg_id_value = value.repl.arg_id.val;
        const auto arg = value.repl.arg_id.kind == arg_id_kind::index
                             ? ctx.arg(arg_id_value.index)
                             : ctx.arg(arg_id_value.name);

        auto specs = value.repl.specs;

        handle_dynamic_spec<internal::width_checker>(
            specs.width, specs.width_ref, ctx, format_view.begin());
        handle_dynamic_spec<internal::precision_checker>(
            specs.precision, specs.precision_ref, ctx, format_view.begin());

        check_prepared_specs(specs, arg.type());
        advance_parse_context_to_specification(parse_ctx, part);
        ctx.advance_to(
            visit_format_arg(arg_formatter<Range>(ctx, nullptr, &specs), arg));
      } break;
      }
    }

    return ctx.out();
  }

 private:
  void advance_parse_context_to_specification(
      basic_parse_context<char_type>& parse_ctx,
      const format_part_t& part) const {
    const auto view = to_string_view(format_);
    const auto specification_begin = view.data() + part.arg_id_end;
    advance_to(parse_ctx, specification_begin);
  }

  template <typename Range, typename Context, typename Id>
  void format_arg(basic_parse_context<char_type>& parse_ctx, Context& ctx,
                  Id arg_id) const {
    ctx.advance_to(visit_format_arg(arg_formatter<Range>(ctx, &parse_ctx),
                                    ctx.arg(arg_id)));
  }

  template <typename Char>
  void check_prepared_specs(const basic_format_specs<Char>& specs,
                            internal::type arg_type) const {
    internal::error_handler h;
    numeric_specs_checker<internal::error_handler> checker(h, arg_type);
    if (specs.align == align::numeric) checker.require_numeric_argument();
    if (specs.sign != sign::none) checker.check_sign();
    if (specs.alt) checker.require_numeric_argument();
    if (specs.precision >= 0) checker.check_precision();
  }

 private:
  Format format_;
  PreparedPartsProvider parts_provider_;
};

template <typename Format> class compiletime_prepared_parts_type_provider {
 private:
  using char_type = char_t<Format>;

// Workaround for old compilers. Compiletime parts preparation will not be
// performed with them anyway.
#if FMT_USE_CONSTEXPR
  static FMT_CONSTEXPR_DECL const unsigned number_of_format_parts =
      count_parts(to_string_view(Format()));
#else
  static const unsigned number_of_format_parts = 0u;
#endif

 public:
  template <unsigned N> struct format_parts_array {
    using value_type = format_part<char_type>;

    FMT_CONSTEXPR format_parts_array() : arr{} {}

    FMT_CONSTEXPR value_type& operator[](unsigned ind) { return arr[ind]; }

    FMT_CONSTEXPR const value_type* begin() const { return arr; }
    FMT_CONSTEXPR const value_type* end() const { return begin() + N; }

   private:
    value_type arr[N];
  };

  struct empty {
    // Parts preparator will search for it
    using value_type = format_part<char_type>;
  };

  using type = conditional_t<number_of_format_parts != 0,
                             format_parts_array<number_of_format_parts>, empty>;
};

template <typename PartsContainer, typename Char>
FMT_CONSTEXPR PartsContainer
prepare_compiletime_parts(basic_string_view<Char> format_str) {
  // This is not a lambda for compatibility with older compilers.
  struct collector {
    PartsContainer& parts;
    unsigned counter = 0;
    FMT_CONSTEXPR void operator()(const format_part<Char>& part) {
      parts[counter++] = part;
    }
  };
  PartsContainer parts;
  compile_format_string<true>(format_str, collector{parts});
  return parts;
}

template <typename Char> class runtime_parts_provider {
 public:
  using parts_container = std::vector<internal::format_part<Char>>;

  runtime_parts_provider(basic_string_view<Char> format_str) {
    compile_format_string<false>(
        format_str,
        [this](const format_part<Char>& part) { parts_.push_back(part); });
  }

  const parts_container& parts() const { return parts_; }

 private:
  parts_container parts_;
};

template <typename Format> struct compiletime_parts_provider {
  using parts_container =
      typename internal::compiletime_prepared_parts_type_provider<Format>::type;

  template <typename Char>
  FMT_CONSTEXPR compiletime_parts_provider(basic_string_view<Char>) {}

  const parts_container& parts() const {
    static FMT_CONSTEXPR_DECL const parts_container prepared_parts =
        prepare_compiletime_parts<parts_container>(
            internal::to_string_view(Format()));
    return prepared_parts;
  }
};
}  // namespace internal

#if FMT_USE_CONSTEXPR
template <typename... Args, typename S,
          FMT_ENABLE_IF(is_compile_string<S>::value)>
FMT_CONSTEXPR auto compile(S format_str)
    -> internal::compiled_format<S, internal::compiletime_parts_provider<S>,
                                 Args...> {
  return format_str;
}
#endif

// Compiles the format string which must be a string literal.
template <typename... Args, typename Char, size_t N>
auto compile(const Char (&format_str)[N]) -> internal::compiled_format<
    std::basic_string<Char>, internal::runtime_parts_provider<Char>, Args...> {
  return std::basic_string<Char>(format_str, N - 1);
}

template <typename CompiledFormat, typename... Args,
          typename Char = typename CompiledFormat::char_type>
std::basic_string<Char> format(const CompiledFormat& cf, const Args&... args) {
  basic_memory_buffer<Char> buffer;
  using range = buffer_range<Char>;
  using context = buffer_context<Char>;
  cf.template vformat_to<range, context>(range(buffer),
                                         {make_format_args<context>(args...)});
  return to_string(buffer);
}

template <typename OutputIt, typename CompiledFormat, typename... Args>
OutputIt format_to(OutputIt out, const CompiledFormat& cf,
                   const Args&... args) {
  using char_type = typename CompiledFormat::char_type;
  using range = internal::output_range<OutputIt, char_type>;
  using context = format_context_t<OutputIt, char_type>;
  return cf.template vformat_to<range, context>(
      range(out), {make_format_args<context>(args...)});
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
  return fmt::format_to(
             internal::counting_iterator<typename CompiledFormat::char_type>(),
             cf, args...)
      .count();
}

FMT_END_NAMESPACE

#endif  // FMT_COMPILE_H_
