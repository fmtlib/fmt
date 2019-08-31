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

template <typename Char> struct format_part {
  struct named_arg_id {
    FMT_CONSTEXPR named_arg_id(string_view_metadata id) : id(id) {}
    string_view_metadata id;
  };

  struct replacement {
    FMT_CONSTEXPR replacement() : arg_id(0u) {}
    FMT_CONSTEXPR replacement(unsigned id) : arg_id(id) {}

    FMT_CONSTEXPR replacement(string_view_metadata id) : arg_id(id) {}

    arg_ref<Char> arg_id;
    dynamic_format_specs<Char> parsed_specs;
  };

  enum class kind { arg_index, arg_name, text, replacement };

  kind part_kind;
  std::size_t end_of_argument_id;
  union value {
    FMT_CONSTEXPR value() : arg_index(0u) {}
    FMT_CONSTEXPR value(unsigned id) : arg_index(id) {}
    FMT_CONSTEXPR value(named_arg_id named_id) : arg_name(named_id.id) {}
    FMT_CONSTEXPR value(string_view_metadata t) : text(t) {}
    FMT_CONSTEXPR value(replacement r) : repl(r) {}
    unsigned arg_index;
    string_view_metadata arg_name;
    string_view_metadata text;
    replacement repl;
  } val;

  FMT_CONSTEXPR format_part()
      : part_kind(kind::arg_index), end_of_argument_id(0u), val(0u) {}

  FMT_CONSTEXPR format_part(unsigned arg_index)
      : part_kind(kind::arg_index), end_of_argument_id(0u), val(arg_index) {}

  FMT_CONSTEXPR format_part(named_arg_id arg_name)
      : part_kind(kind::arg_name), end_of_argument_id(0u), val(arg_name) {}

  FMT_CONSTEXPR format_part(string_view_metadata text)
      : part_kind(kind::text), end_of_argument_id(0u), val(text) {}

  FMT_CONSTEXPR format_part(replacement repl)
      : part_kind(kind::replacement), end_of_argument_id(0u), val(repl) {}
};

template <typename Char, typename PartsContainer>
class format_preparation_handler : public internal::error_handler {
 private:
  using part = format_part<Char>;

 public:
  FMT_CONSTEXPR format_preparation_handler(basic_string_view<Char> format,
                                           PartsContainer& parts)
      : parts_(parts), format_(format), parse_context_(format) {}

  FMT_CONSTEXPR void on_text(const Char* begin, const Char* end) {
    if (begin == end) return;
    const auto offset = begin - format_.data();
    const auto size = end - begin;
    parts_.push_back(part(string_view_metadata(offset, size)));
  }

  FMT_CONSTEXPR void on_arg_id() {
    parts_.push_back(part(parse_context_.next_arg_id()));
  }

  FMT_CONSTEXPR void on_arg_id(unsigned id) {
    parse_context_.check_arg_id(id);
    parts_.push_back(part(id));
  }

  FMT_CONSTEXPR void on_arg_id(basic_string_view<Char> id) {
    const auto view = string_view_metadata(format_, id);
    const auto arg_id = typename part::named_arg_id(view);
    parts_.push_back(part(arg_id));
  }

  FMT_CONSTEXPR void on_replacement_field(const Char* ptr) {
    parts_.back().end_of_argument_id = ptr - format_.begin();
  }

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin,
                                            const Char* end) {
    const auto specs_offset = to_unsigned(begin - format_.begin());

    using parse_context = basic_parse_context<Char>;
    internal::dynamic_format_specs<Char> parsed_specs;
    dynamic_specs_handler<parse_context> handler(parsed_specs, parse_context_);
    begin = parse_format_specs(begin, end, handler);

    if (*begin != '}') on_error("missing '}' in format string");

    auto& last_part = parts_.back();
    auto specs = last_part.part_kind == part::kind::arg_index
                     ? typename part::replacement(last_part.val.arg_index)
                     : typename part::replacement(last_part.val.arg_name);
    specs.parsed_specs = parsed_specs;
    last_part = part(specs);
    last_part.end_of_argument_id = specs_offset;
    return begin;
  }

 private:
  PartsContainer& parts_;
  basic_string_view<Char> format_;
  basic_parse_context<Char> parse_context_;
};

template <typename Format, typename PreparedPartsProvider, typename... Args>
class compiled_format {
 public:
  using char_type = char_t<Format>;
  using format_part_t = format_part<char_type>;

  constexpr compiled_format(Format f)
      : format_(std::move(f)), parts_provider_(to_string_view(format_)) {}

  compiled_format() = delete;

  using context = buffer_context<char_type>;

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
        const auto text = value.text.to_view(format_view.data());
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
        const auto named_arg_id = value.arg_name.to_view(format_view.data());
        format_arg<Range>(parse_ctx, ctx, named_arg_id);
      } break;
      case format_part_t::kind::replacement: {
        const auto& arg_id_value = value.repl.arg_id.val;
        const auto arg = value.repl.arg_id.kind == arg_id_kind::index
                             ? ctx.arg(arg_id_value.index)
                             : ctx.arg(arg_id_value.name.to_view(
                                   to_string_view(format_).data()));

        auto specs = value.repl.parsed_specs;

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
    const auto specification_begin = view.data() + part.end_of_argument_id;
    advance_to(parse_ctx, specification_begin);
  }

  template <typename Range, typename Context, typename Id>
  void format_arg(basic_parse_context<char_type>& parse_ctx, Context& ctx,
                  Id arg_id) const {
    parse_ctx.check_arg_id(arg_id);
    const auto stopped_at =
        visit_format_arg(arg_formatter<Range>(ctx), ctx.arg(arg_id));
    ctx.advance_to(stopped_at);
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
    unsigned braces_counter = 0;
    for (; begin != end; ++begin) {
      if (*begin == '{') {
        ++braces_counter;
      } else if (*begin == '}') {
        if (braces_counter == 0u) break;
        --braces_counter;
      }
    }
    return begin;
  }

  FMT_CONSTEXPR void on_error(const char*) {}
};

template <typename Format> class compiletime_prepared_parts_type_provider {
 private:
  using char_type = char_t<Format>;

  static FMT_CONSTEXPR unsigned count_parts() {
    FMT_CONSTEXPR_DECL const auto text = to_string_view(Format{});
    part_counter<char_type> counter;
    internal::parse_format_string</*IS_CONSTEXPR=*/true>(text, counter);
    return counter.num_parts;
  }

// Workaround for old compilers. Compiletime parts preparation will not be
// performed with them anyway.
#if FMT_USE_CONSTEXPR
  static FMT_CONSTEXPR_DECL const unsigned number_of_format_parts =
      compiletime_prepared_parts_type_provider::count_parts();
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

template <typename Parts> class compiletime_prepared_parts_collector {
 private:
  using format_part = typename Parts::value_type;

 public:
  FMT_CONSTEXPR explicit compiletime_prepared_parts_collector(Parts& parts)
      : parts_{parts}, counter_{0u} {}

  FMT_CONSTEXPR void push_back(format_part part) { parts_[counter_++] = part; }

  FMT_CONSTEXPR format_part& back() { return parts_[counter_ - 1]; }

 private:
  Parts& parts_;
  unsigned counter_;
};

template <typename PartsContainer, typename Char>
FMT_CONSTEXPR PartsContainer
prepare_compiletime_parts(basic_string_view<Char> format) {
  using collector = compiletime_prepared_parts_collector<PartsContainer>;

  PartsContainer parts;
  collector c(parts);
  internal::parse_format_string</*IS_CONSTEXPR=*/true>(
      format, format_preparation_handler<Char, collector>(format, c));
  return parts;
}

template <typename Char> class runtime_parts_provider {
 public:
  using parts_container = std::vector<internal::format_part<Char>>;

  runtime_parts_provider(basic_string_view<Char> format) {
    internal::parse_format_string<false>(
        format,
        format_preparation_handler<Char, parts_container>(format, parts_));
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
