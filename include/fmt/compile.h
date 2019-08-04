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
 public:
  struct named_argument_id {
    FMT_CONSTEXPR named_argument_id(internal::string_view_metadata id)
        : id(id) {}
    internal::string_view_metadata id;
  };

  struct argument_id {
    FMT_CONSTEXPR argument_id() : argument_id(0u) {}

    FMT_CONSTEXPR argument_id(unsigned id)
        : which(which_arg_id::index), val(id) {}

    FMT_CONSTEXPR argument_id(internal::string_view_metadata id)
        : which(which_arg_id::named_index), val(id) {}

    enum class which_arg_id { index, named_index };

    which_arg_id which;

    union value {
      FMT_CONSTEXPR value() : index(0u) {}
      FMT_CONSTEXPR value(unsigned id) : index(id) {}
      FMT_CONSTEXPR value(internal::string_view_metadata id)
          : named_index(id) {}

      unsigned index;
      internal::string_view_metadata named_index;
    } val;
  };

  struct specification {
    FMT_CONSTEXPR specification() : arg_id(0u) {}
    FMT_CONSTEXPR specification(unsigned id) : arg_id(id) {}

    FMT_CONSTEXPR specification(internal::string_view_metadata id)
        : arg_id(id) {}

    argument_id arg_id;
    internal::dynamic_format_specs<Char> parsed_specs;
  };

  FMT_CONSTEXPR format_part()
      : which(which_value::argument_id), end_of_argument_id(0u), val(0u) {}

  FMT_CONSTEXPR format_part(internal::string_view_metadata text)
      : which(which_value::text), end_of_argument_id(0u), val(text) {}

  FMT_CONSTEXPR format_part(unsigned id)
      : which(which_value::argument_id), end_of_argument_id(0u), val(id) {}

  FMT_CONSTEXPR format_part(named_argument_id arg_id)
      : which(which_value::named_argument_id),
        end_of_argument_id(0u),
        val(arg_id) {}

  FMT_CONSTEXPR format_part(specification spec)
      : which(which_value::specification), end_of_argument_id(0u), val(spec) {}

  enum class which_value {
    argument_id,
    named_argument_id,
    text,
    specification
  };

  which_value which;
  std::size_t end_of_argument_id;
  union value {
    FMT_CONSTEXPR value() : arg_id(0u) {}
    FMT_CONSTEXPR value(unsigned id) : arg_id(id) {}
    FMT_CONSTEXPR value(named_argument_id named_id)
        : named_arg_id(named_id.id) {}
    FMT_CONSTEXPR value(internal::string_view_metadata t) : text(t) {}
    FMT_CONSTEXPR value(specification s) : spec(s) {}
    unsigned arg_id;
    internal::string_view_metadata named_arg_id;
    internal::string_view_metadata text;
    specification spec;
  } val;
};

template <typename Char, typename PartsContainer>
class format_preparation_handler : public internal::error_handler {
 private:
  using part = format_part<Char>;

 public:
  using iterator = typename basic_string_view<Char>::iterator;

  FMT_CONSTEXPR format_preparation_handler(basic_string_view<Char> format,
                                           PartsContainer& parts)
      : parts_(parts), format_(format), parse_context_(format) {}

  FMT_CONSTEXPR void on_text(const Char* begin, const Char* end) {
    if (begin == end) return;
    const auto offset = begin - format_.data();
    const auto size = end - begin;
    parts_.add(part(string_view_metadata(offset, size)));
  }

  FMT_CONSTEXPR void on_arg_id() {
    parts_.add(part(parse_context_.next_arg_id()));
  }

  FMT_CONSTEXPR void on_arg_id(unsigned id) {
    parse_context_.check_arg_id(id);
    parts_.add(part(id));
  }

  FMT_CONSTEXPR void on_arg_id(basic_string_view<Char> id) {
    const auto view = string_view_metadata(format_, id);
    const auto arg_id = typename part::named_argument_id(view);
    parts_.add(part(arg_id));
  }

  FMT_CONSTEXPR void on_replacement_field(const Char* ptr) {
    auto last_part = parts_.last();
    last_part.end_of_argument_id = ptr - format_.begin();
    parts_.substitute_last(last_part);
  }

  FMT_CONSTEXPR const Char* on_format_specs(const Char* begin,
                                            const Char* end) {
    const auto specs_offset = to_unsigned(begin - format_.begin());

    using parse_context = basic_parse_context<Char>;
    internal::dynamic_format_specs<Char> parsed_specs;
    dynamic_specs_handler<parse_context> handler(parsed_specs, parse_context_);
    begin = parse_format_specs(begin, end, handler);

    if (*begin != '}') on_error("missing '}' in format string");

    const auto last_part = parts_.last();

    auto specs = last_part.which == part::which_value::argument_id
                     ? typename part::specification(last_part.val.arg_id)
                     : typename part::specification(last_part.val.named_arg_id);

    specs.parsed_specs = parsed_specs;

    auto new_part = part(specs);
    new_part.end_of_argument_id = specs_offset;

    parts_.substitute_last(new_part);

    return begin;
  }

 private:
  PartsContainer& parts_;
  basic_string_view<Char> format_;
  basic_parse_context<Char> parse_context_;
};

template <typename Format, typename PreparedPartsProvider, typename... Args>
class prepared_format {
 public:
  using char_type = char_t<Format>;
  using format_part_t = format_part<char_type>;

  prepared_format(Format f)
      : format_(std::move(f)), parts_provider_(to_string_view(format_)) {}

  prepared_format() = delete;

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

      switch (part.which) {
      case format_part_t::which_value::text: {
        const auto text = value.text.to_view(format_view.data());
        auto output = ctx.out();
        auto&& it = internal::reserve(output, text.size());
        it = std::copy_n(text.begin(), text.size(), it);
        ctx.advance_to(output);
      } break;

      case format_part_t::which_value::argument_id: {
        advance_parse_context_to_specification(parse_ctx, part);
        format_arg<Range>(parse_ctx, ctx, value.arg_id);
      } break;

      case format_part_t::which_value::named_argument_id: {
        advance_parse_context_to_specification(parse_ctx, part);
        const auto named_arg_id =
            value.named_arg_id.to_view(format_view.data());
        format_arg<Range>(parse_ctx, ctx, named_arg_id);
      } break;
      case format_part_t::which_value::specification: {
        const auto& arg_id_value = value.spec.arg_id.val;
        const auto arg = value.spec.arg_id.which ==
                                 format_part_t::argument_id::which_arg_id::index
                             ? ctx.arg(arg_id_value.index)
                             : ctx.arg(arg_id_value.named_index.to_view(
                                   to_string_view(format_).data()));

        auto specs = value.spec.parsed_specs;

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

template <typename Format> class compiletime_prepared_parts_type_provider {
 private:
  using char_type = char_t<Format>;

  class count_handler {
   public:
    FMT_CONSTEXPR count_handler() : counter_(0u) {}

    FMT_CONSTEXPR void on_text(const char_type* begin, const char_type* end) {
      if (begin != end) ++counter_;
    }

    FMT_CONSTEXPR void on_arg_id() { ++counter_; }
    FMT_CONSTEXPR void on_arg_id(unsigned) { ++counter_; }
    FMT_CONSTEXPR void on_arg_id(basic_string_view<char_type>) { ++counter_; }

    FMT_CONSTEXPR void on_replacement_field(const char_type*) {}

    FMT_CONSTEXPR const char_type* on_format_specs(const char_type* begin,
                                                   const char_type* end) {
      return find_matching_brace(begin, end);
    }

    FMT_CONSTEXPR void on_error(const char*) {}

    FMT_CONSTEXPR unsigned result() const { return counter_; }

   private:
    FMT_CONSTEXPR const char_type* find_matching_brace(const char_type* begin,
                                                       const char_type* end) {
      unsigned braces_counter{0u};
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

   private:
    unsigned counter_;
  };

  static FMT_CONSTEXPR unsigned count_parts() {
    FMT_CONSTEXPR_DECL const auto text = to_string_view(Format{});
    count_handler handler;
    internal::parse_format_string</*IS_CONSTEXPR=*/true>(text, handler);
    return handler.result();
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

  using type = conditional_t<static_cast<bool>(number_of_format_parts),
                             format_parts_array<number_of_format_parts>, empty>;
};

template <typename Parts> class compiletime_prepared_parts_collector {
 private:
  using format_part = typename Parts::value_type;

 public:
  FMT_CONSTEXPR explicit compiletime_prepared_parts_collector(Parts& parts)
      : parts_{parts}, counter_{0u} {}

  FMT_CONSTEXPR void add(format_part part) { parts_[counter_++] = part; }

  FMT_CONSTEXPR void substitute_last(format_part part) {
    parts_[counter_ - 1] = part;
  }

  FMT_CONSTEXPR format_part last() { return parts_[counter_ - 1]; }

 private:
  Parts& parts_;
  unsigned counter_;
};

template <typename PartsContainer, typename Char>
FMT_CONSTEXPR PartsContainer prepare_parts(basic_string_view<Char> format) {
  PartsContainer parts;
  internal::parse_format_string</*IS_CONSTEXPR=*/false>(
      format, format_preparation_handler<Char, PartsContainer>(format, parts));
  return parts;
}

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

template <typename PartsContainer> class runtime_parts_provider {
 public:
  runtime_parts_provider() = delete;
  template <typename Char>
  runtime_parts_provider(basic_string_view<Char> format)
      : parts_(prepare_parts<PartsContainer>(format)) {}

  const PartsContainer& parts() const { return parts_; }

 private:
  PartsContainer parts_;
};

template <typename Format, typename PartsContainer>
struct compiletime_parts_provider {
  compiletime_parts_provider() = delete;
  template <typename Char>
  FMT_CONSTEXPR compiletime_parts_provider(basic_string_view<Char>) {}

  const PartsContainer& parts() const {
    static FMT_CONSTEXPR_DECL const PartsContainer prepared_parts =
        prepare_compiletime_parts<PartsContainer>(
            internal::to_string_view(Format{}));

    return prepared_parts;
  }
};

template <typename PartsContainer>
struct parts_container_concept_check : std::true_type {
  static_assert(std::is_copy_constructible<PartsContainer>::value,
                "PartsContainer is not copy constructible");
  static_assert(std::is_move_constructible<PartsContainer>::value,
                "PartsContainer is not move constructible");

  template <typename T, typename = void>
  struct has_format_part_type : std::false_type {};
  template <typename T>
  struct has_format_part_type<T, void_t<typename T::format_part_type>>
      : std::true_type {};

  static_assert(has_format_part_type<PartsContainer>::value,
                "PartsContainer doesn't provide format_part_type");

  struct check_second {};
  struct check_first : check_second {};

  template <typename T> static std::false_type has_add_check(check_second);
  template <typename T>
  static decltype(
      (void)std::declval<T>().add(std::declval<typename T::format_part_type>()),
      std::true_type()) has_add_check(check_first);
  using has_add = decltype(has_add_check<PartsContainer>(check_first()));
  static_assert(has_add::value, "PartsContainer doesn't provide add() method");

  template <typename T> static std::false_type has_last_check(check_second);
  template <typename T>
  static decltype((void)std::declval<T>().last(),
                  std::true_type()) has_last_check(check_first);
  using has_last = decltype(has_last_check<PartsContainer>(check_first()));
  static_assert(has_last::value,
                "PartsContainer doesn't provide last() method");

  template <typename T>
  static std::false_type has_substitute_last_check(check_second);
  template <typename T>
  static decltype((void)std::declval<T>().substitute_last(
                      std::declval<typename T::format_part_type>()),
                  std::true_type()) has_substitute_last_check(check_first);
  using has_substitute_last =
      decltype(has_substitute_last_check<PartsContainer>(check_first()));
  static_assert(has_substitute_last::value,
                "PartsContainer doesn't provide substitute_last() method");

  template <typename T> static std::false_type has_begin_check(check_second);
  template <typename T>
  static decltype((void)std::declval<T>().begin(),
                  std::true_type()) has_begin_check(check_first);
  using has_begin = decltype(has_begin_check<PartsContainer>(check_first()));
  static_assert(has_begin::value,
                "PartsContainer doesn't provide begin() method");

  template <typename T> static std::false_type has_end_check(check_second);
  template <typename T>
  static decltype((void)std::declval<T>().end(),
                  std::true_type()) has_end_check(check_first);
  using has_end = decltype(has_end_check<PartsContainer>(check_first()));
  static_assert(has_end::value, "PartsContainer doesn't provide end() method");
};

template <bool IS_CONSTEXPR, typename Format, typename /*PartsContainer*/>
struct parts_provider_type {
  using type = compiletime_parts_provider<
      Format, typename compiletime_prepared_parts_type_provider<Format>::type>;
};

template <typename Format, typename PartsContainer>
struct parts_provider_type</*IS_CONSTEXPR=*/false, Format, PartsContainer> {
  static_assert(parts_container_concept_check<PartsContainer>::value,
                "Parts container doesn't meet the concept");
  using type = runtime_parts_provider<PartsContainer>;
};

template <typename Format, typename PreparedPartsContainer, typename... Args>
struct basic_prepared_format {
  using type =
      internal::prepared_format<Format,
                                typename internal::parts_provider_type<
                                    is_compile_string<Format>::value, Format,
                                    PreparedPartsContainer>::type,
                                Args...>;
};

template <typename Char>
std::basic_string<Char> to_runtime_format(basic_string_view<Char> format) {
  return std::basic_string<Char>(format.begin(), format.size());
}

template <typename Char>
std::basic_string<Char> to_runtime_format(const Char* format) {
  return std::basic_string<Char>(format);
}

template <typename Char, typename Container = std::vector<format_part<Char>>>
class parts_container {
 public:
  using format_part_type = format_part<Char>;

  void add(format_part_type part) { parts_.push_back(std::move(part)); }

  void substitute_last(format_part_type part) {
    parts_.back() = std::move(part);
  }

  format_part_type last() { return parts_.back(); }

  auto begin() -> decltype(std::declval<Container>().begin()) {
    return parts_.begin();
  }

  auto begin() const -> decltype(std::declval<const Container>().begin()) {
    return parts_.begin();
  }

  auto end() -> decltype(std::declval<Container>().end()) {
    return parts_.end();
  }

  auto end() const -> decltype(std::declval<const Container>().end()) {
    return parts_.end();
  }

 private:
  Container parts_;
};

// Delegate preparing to preparator, to take advantage of a partial
// specialization.
template <typename Format, typename... Args> struct preparator {
  using container = parts_container<char_t<Format>>;
  using prepared_format_type =
      typename basic_prepared_format<Format, container, Args...>::type;

  static auto prepare(Format format) -> prepared_format_type {
    return prepared_format_type(std::move(format));
  }
};

template <typename PassedFormat, typename PreparedFormatFormat,
          typename PartsContainer, typename... Args>
struct preparator<PassedFormat, prepared_format<PreparedFormatFormat,
                                                PartsContainer, Args...>> {
  using prepared_format_type =
      prepared_format<PreparedFormatFormat, PartsContainer, Args...>;

  static auto prepare(PassedFormat format) -> prepared_format_type {
    return prepared_format_type(std::move(format));
  }
};

struct compiletime_format_tag {};
struct runtime_format_tag {};

template <typename Format> struct format_tag {
  using type = conditional_t<is_compile_string<Format>::value,
                             compiletime_format_tag, runtime_format_tag>;
};

#if FMT_USE_CONSTEXPR
template <typename Format, typename... Args>
auto do_compile(runtime_format_tag, Format format) {
  return preparator<Format, Args...>::prepare(std::move(format));
}

template <typename Format, typename... Args>
FMT_CONSTEXPR auto do_compile(compiletime_format_tag, const Format& format) {
  return typename basic_prepared_format<Format, void, Args...>::type(format);
}
#else
template <typename Format, typename... Args>
auto do_compile(const Format& format)
    -> decltype(preparator<Format, Args...>::prepare(format)) {
  return preparator<Format, Args...>::prepare(format);
}
#endif

template <typename... Args>
using prepared_format_t =
    typename basic_prepared_format<std::string, parts_container<char>,
                                   Args...>::type;
}  // namespace internal

#if FMT_USE_CONSTEXPR

template <typename... Args, typename S>
FMT_CONSTEXPR auto compile(S format_str) {
  return internal::do_compile<S, Args...>(
      typename internal::format_tag<S>::type{}, std::move(format_str));
}
#else

template <typename... Args, typename S>
auto compile(S format_str) ->
    typename internal::preparator<S, Args...>::prepared_format_type {
  return internal::preparator<S, Args...>::prepare(std::move(format_str));
}
#endif

template <typename... Args, typename Char>
auto compile(const Char* format_str) ->
    typename internal::preparator<std::basic_string<Char>,
                                  Args...>::prepared_format_type {
  return compile<Args...>(internal::to_runtime_format(format_str));
}

template <typename... Args, typename Char, unsigned N>
auto compile(const Char(format_str)[N]) ->
    typename internal::preparator<std::basic_string<Char>,
                                  Args...>::prepared_format_type {
  const auto view = basic_string_view<Char>(format_str, N);
  return compile<Args...>(internal::to_runtime_format(view));
}

template <typename... Args, typename Char>
auto compile(basic_string_view<Char> format_str) ->
    typename internal::preparator<std::basic_string<Char>,
                                  Args...>::prepared_format_type {
  return compile<Args...>(internal::to_runtime_format(format_str));
}

template <typename CompiledFormat, typename... Args,
          typename Char = typename CompiledFormat::char_type>
std::basic_string<Char> format(const CompiledFormat& cf, const Args&... args) {
  basic_memory_buffer<Char> buffer;
  using range = internal::buffer_range<Char>;
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
  format_arg_store<context, Args...> as(args...);
  return cf.template vformat_to<range, context>(
      range(out), {make_format_args<context>(args...)});
}

template <typename OutputIt, typename CompiledFormat, typename... Args,
          FMT_ENABLE_IF(internal::is_output_iterator<OutputIt>::value)>
format_to_n_result<OutputIt> format_to_n(OutputIt out, unsigned n,
                                         const CompiledFormat& cf,
                                         const Args&... args) {
  auto it =
      cf.format_to(internal::truncating_iterator<OutputIt>(out, n), args...)
          .count();
  return {it.base(), it.count()};
}

template <typename CompiledFormat, typename... Args>
std::size_t formatted_size(const CompiledFormat& cf, const Args&... args) {
  return cf
      .format_to(
          internal::counting_iterator<typename CompiledFormat::char_type>(),
          args...)
      .count();
}

FMT_END_NAMESPACE

#endif  // FMT_COMPILE_H_
