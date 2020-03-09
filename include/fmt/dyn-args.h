// Copyright (c) 2020 Vladimir Solontsov
// SPDX-License-Identifier: MIT Licence

#ifndef FMT_DYN_ARGS_H_
#define FMT_DYN_ARGS_H_

#include <vector>
#include <forward_list>
#include <functional>

#include "core.h"

#if (defined(FMT_HAS_VARIANT) || __cplusplus >= 201703L)
#  include <variant>
#  ifndef FMT_HAS_VARIANT
#    define FMT_HAS_VARIANT
#  endif
#endif

FMT_BEGIN_NAMESPACE

namespace internal {

template<typename T, typename Char>
struct is_string_view : std::false_type{};

template<typename Char>
struct is_string_view<basic_string_view<Char>, Char>
: std::true_type{};

#ifdef FMT_USE_STRING_VIEW
template<typename Traits, typename Char>
struct is_string_view<std::basic_string_view<Char, Traits>, Char>
: std::true_type{};
#endif

#ifdef FMT_USE_EXPERIMENTAL_STRING_VIEW
template<typename Traits, typename Char>
struct is_string_view<std::expeimental::basic_string_view<Char, Traits>, Char>
: std::true_type{};
#endif

template<typename T>
struct is_ref_wrapper : std::false_type{};

template<typename T>
struct is_ref_wrapper<std::reference_wrapper<T>> : std::true_type{};

template<typename T, typename Context>
struct need_dyn_copy{
  using mapped_type = mapped_type_constant<T, Context>;
  static_assert(mapped_type::value != internal::type::named_arg_type,
    "Bug indicator. Named arguments must be processed separately");

  using type = std::integral_constant<bool,!(
      is_ref_wrapper<T>::value ||
      is_string_view<T, typename Context::char_type>::value ||
      (
        mapped_type::value != internal::type::cstring_type &&
        mapped_type::value != internal::type::custom_type &&
        mapped_type::value != internal::type::string_type
      )
  )>;
};

template<typename T, typename Context> using need_dyn_copy_t =
    typename need_dyn_copy<T, Context>::type;

template<typename T, typename StorageValue>
const T& get(const StorageValue& v){
  return v;
}

}  // namespace internal

/**
  \rst
  A dynamic version of `fmt::format_arg_store<>`.
  It's equipped with a storage to potentially temporary objects which lifetime
  could be shorter than the format arguments object.

  It can be implicitly converted into `~fmt::basic_format_args` for passing
  into type-erased formatting functions such as `~fmt::vformat`.
  \endrst
 */
template <typename Context, typename... Args>
class dynamic_format_arg_store
#if FMT_GCC_VERSION < 409
    // Workaround a GCC template argument substitution bug.
    : public basic_format_args<Context>
#endif
{
 private:
  using char_type = typename Context::char_type;
  static const bool is_packed = false;

  static const bool has_custom_args = (sizeof...(Args) > 0);
  using string_type = std::basic_string<char_type>;
#ifdef FMT_HAS_VARIANT
  using storage_item_type =
      conditional_t<has_custom_args,
                    std::variant<string_type, Args...>,
                    string_type>;
#else
  static_assert(!has_custom_args, "std::variant<> is required to support "
      "custom types in dynamic_format_arg_store");
  using storage_item_type = string_type;
#endif
  using value_type = basic_format_arg<Context>;
  using named_value_type = internal::named_arg_base<char_type>;

  template<typename T>
  using storaged_type =
      conditional_t<internal::is_string<T>::value, string_type, T>;

  // Storage of basic_format_arg must be contiguous
  // Required by basic_format_args::args_ which is just a pointer
  std::vector<value_type> data_;

  // Storage of arguments not fitting into basic_format_arg must grow
  // without relocation because items in data_ refer to it.

  std::forward_list<storage_item_type> storage_;

  // Storage of serialized name_args. Must grow without relocation
  // because items in data_ refer to it.
  std::forward_list<named_value_type> named_args_;

  friend class basic_format_args<Context>;

  template<typename T>
  const T& get_last_pushed() const{
    using internal::get;
    return get<T>(storage_.front());
  }

  unsigned long long get_types() const {
    return internal::is_unpacked_bit | data_.size();
  }

  template<typename T> const T& stored_value(const T& arg, std::false_type) {
    return arg;
  }

  template<typename T> const T& stored_value(
      const std::reference_wrapper<T>& arg, std::false_type) {
    return arg.get();
  }

  template<typename T> const storaged_type<T>& stored_value(const T& arg,
                                                            std::true_type) {
    using type = storaged_type<T>;
    storage_.emplace_front(type{arg});
    return get_last_pushed<type>();
  }

  template<typename T> void emplace_arg(const T& arg) {
    data_.emplace_back(internal::make_arg<Context>(arg));
  }

 public:
  dynamic_format_arg_store() FMT_NOEXCEPT = default;
  ~dynamic_format_arg_store() FMT_NOEXCEPT = default;

  dynamic_format_arg_store(const dynamic_format_arg_store&) = delete;
  dynamic_format_arg_store& operator=(const dynamic_format_arg_store&) = delete;

  dynamic_format_arg_store(dynamic_format_arg_store&&) = default;
  dynamic_format_arg_store& operator=(dynamic_format_arg_store&&) = default;

  template <typename T> void push_back(const T& arg) {
    emplace_arg(stored_value(arg, internal::need_dyn_copy_t<T, Context>{}));
  }

  template <typename T> void push_back(std::reference_wrapper<T> arg) {
    emplace_arg(arg.get());
  }

  template <typename T> void push_back(
      const internal::named_arg<T, char_type>& arg) {
    // Named argument is tricky. It's returned by value from fmt::arg()
    // and then pointer to it is stored in basic_format_arg<>.
    // So after end of expression the pointer becomes dangling.
    storage_.emplace_front(string_type{arg.name.data(), arg.name.size()});
    basic_string_view<char_type> name = get_last_pushed<string_type>();
    const auto& val = stored_value(
        arg.value, internal::need_dyn_copy_t<T, Context>{});

    auto named_with_stored_parts = fmt::arg(name, val);
    // Serialize value into base
    internal::arg_mapper<Context>().map(named_with_stored_parts);
    named_args_.push_front(named_with_stored_parts);
    data_.emplace_back(internal::make_arg<Context>(named_args_.front()));
  }
};

FMT_END_NAMESPACE

#endif /* FMT_DYN_ARGS_H_ */
