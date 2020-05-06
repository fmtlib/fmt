// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format-inl.h"

FMT_BEGIN_NAMESPACE
namespace internal {

template <typename T>
int format_float(char* buf, std::size_t size, const char* format, int precision,
                 T value) {
#ifdef FMT_FUZZ
  if (precision > 100000)
    throw std::runtime_error(
        "fuzz mode - avoid large allocation inside snprintf");
#endif
  // Suppress the warning about nonliteral format string.
  int (*snprintf_ptr)(char*, size_t, const char*, ...) = FMT_SNPRINTF;
  return precision < 0 ? snprintf_ptr(buf, size, format, value)
                       : snprintf_ptr(buf, size, format, precision, value);
}

// DEPRECATED.
template <typename Context> class arg_map {
 private:
  struct entry {
    basic_string_view<typename Context::char_type> name;
    basic_format_arg<Context> arg;
  };

  entry* map_;
  unsigned size_;

  void push_back(value<Context> val) {
    const auto& named = *val.named_arg;
    map_[size_] = {named.name, named.template deserialize<Context>()};
    ++size_;
  }

 public:
  void init(const basic_format_args<Context>& args);
};

// This is deprecated and is kept only to preserve ABI compatibility.
template <typename Context>
void arg_map<Context>::init(const basic_format_args<Context>& args) {
  if (map_) return;
  map_ = new entry[internal::to_unsigned(args.max_size())];
  if (args.is_packed()) {
    for (int i = 0;; ++i) {
      internal::type arg_type = args.type(i);
      if (arg_type == internal::type::none_type) return;
      if (arg_type == internal::type::named_arg_type)
        push_back(args.values_[i]);
    }
  }
  for (int i = 0, n = args.max_size(); i < n; ++i) {
    auto type = args.args_[i].type_;
    if (type == internal::type::named_arg_type) push_back(args.args_[i].value_);
  }
}
}  // namespace internal

template struct FMT_INSTANTIATION_DEF_API internal::basic_data<void>;

// Workaround a bug in MSVC2013 that prevents instantiation of format_float.
int (*instantiate_format_float)(double, int, internal::float_specs,
                                internal::buffer<char>&) =
    internal::format_float;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template FMT_API internal::locale_ref::locale_ref(const std::locale& loc);
template FMT_API std::locale internal::locale_ref::get<std::locale>() const;
#endif

// Explicit instantiations for char.

template FMT_API std::string internal::grouping_impl<char>(locale_ref);
template FMT_API char internal::thousands_sep_impl(locale_ref);
template FMT_API char internal::decimal_point_impl(locale_ref);

template FMT_API void internal::buffer<char>::append(const char*, const char*);

template FMT_API void internal::arg_map<format_context>::init(
    const basic_format_args<format_context>& args);

template FMT_API std::string internal::vformat<char>(
    string_view, basic_format_args<format_context>);

template FMT_API format_context::iterator internal::vformat_to(
    internal::buffer<char>&, string_view, basic_format_args<format_context>);

template FMT_API int internal::snprintf_float(double, int,
                                              internal::float_specs,
                                              internal::buffer<char>&);
template FMT_API int internal::snprintf_float(long double, int,
                                              internal::float_specs,
                                              internal::buffer<char>&);
template FMT_API int internal::format_float(double, int, internal::float_specs,
                                            internal::buffer<char>&);
template FMT_API int internal::format_float(long double, int,
                                            internal::float_specs,
                                            internal::buffer<char>&);

// Explicit instantiations for wchar_t.

template FMT_API std::string internal::grouping_impl<wchar_t>(locale_ref);
template FMT_API wchar_t internal::thousands_sep_impl(locale_ref);
template FMT_API wchar_t internal::decimal_point_impl(locale_ref);

template FMT_API void internal::buffer<wchar_t>::append(const wchar_t*,
                                                        const wchar_t*);
FMT_END_NAMESPACE
