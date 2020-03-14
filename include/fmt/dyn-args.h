// Copyright (c) 2020 Vladimir Solontsov
// SPDX-License-Identifier: MIT Licence

#ifndef FMT_DYN_ARGS_H_
#define FMT_DYN_ARGS_H_

#include <forward_list>
#include <functional>
#include <memory>
#include <vector>

#include "core.h"

FMT_BEGIN_NAMESPACE

namespace internal {

template <typename T, typename Char> struct is_string_view : std::false_type {};

template <typename Char>
struct is_string_view<basic_string_view<Char>, Char> : std::true_type {};

#ifdef FMT_USE_STRING_VIEW
template <typename Traits, typename Char>
struct is_string_view<std::basic_string_view<Char, Traits>, Char>
    : std::true_type {};
#endif

#ifdef FMT_USE_EXPERIMENTAL_STRING_VIEW
template <typename Traits, typename Char>
struct is_string_view<std::experimental::basic_string_view<Char, Traits>, Char>
    : std::true_type {};
#endif

template <typename T> struct is_ref_wrapper : std::false_type {};

template <typename T>
struct is_ref_wrapper<std::reference_wrapper<T>> : std::true_type {};

template <typename T, typename Context> struct need_dyn_copy {
  using mapped_type = mapped_type_constant<T, Context>;
  static_assert(mapped_type::value != internal::type::named_arg_type,
                "Bug indicator. Named arguments must be processed separately");

  using type = std::integral_constant<
      bool, !(is_ref_wrapper<T>::value ||
              is_string_view<T, typename Context::char_type>::value ||
              (mapped_type::value != internal::type::cstring_type &&
               mapped_type::value != internal::type::custom_type &&
               mapped_type::value != internal::type::string_type))>;
};

template <typename T, typename Context>
using need_dyn_copy_t = typename need_dyn_copy<T, Context>::type;

class dyn_arg_storage {
  // Workaround clang's -Wweak-vtables. For templates (unlike regular classes
  // doesn't complain about inability to deduce translation unit to place vtable
  // So dyn_arg_node_base is made a fake template

  template <typename = void> struct storage_node_base {
    using owning_ptr = std::unique_ptr<storage_node_base<>>;
    virtual ~storage_node_base() = default;
    owning_ptr next_;
  };

  using owning_ptr = storage_node_base<>::owning_ptr;

  template <typename T> struct storage_node : storage_node_base<> {
    T value_;
    FMT_CONSTEXPR explicit storage_node(const T& arg, owning_ptr&& next)
        : value_{arg} {
      // Must be initialised after value_
      next_ = std::move(next);
    }
  };

  owning_ptr head_{nullptr};

 public:
  dyn_arg_storage() = default;
  dyn_arg_storage(const dyn_arg_storage&) = delete;
  dyn_arg_storage(dyn_arg_storage&&) = default;

  dyn_arg_storage& operator=(const dyn_arg_storage&) = delete;
  dyn_arg_storage& operator=(dyn_arg_storage&&) = default;

  template <typename T, typename Arg> const T& push(const Arg& arg) {
    auto node = new storage_node<T>{arg, std::move(head_)};
    head_.reset(node);
    return node->value_;
  }
};

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
template <typename Context>
class dynamic_format_arg_store
#if FMT_GCC_VERSION < 409
    // Workaround a GCC template argument substitution bug.
    : public basic_format_args<Context>
#endif
{
 private:
  using char_type = typename Context::char_type;
  using string_type = std::basic_string<char_type>;
  using value_type = basic_format_arg<Context>;

  template <typename T>
  using stored_type =
      conditional_t<internal::is_string<T>::value, string_type, T>;

  // Storage of basic_format_arg must be contiguous
  // Required by basic_format_args::args_ which is just a pointer
  std::vector<value_type> data_;

  // Storage of arguments not fitting into basic_format_arg must grow
  // without relocation because items in data_ refer to it.

  internal::dyn_arg_storage storage_;

  friend class basic_format_args<Context>;

  unsigned long long get_types() const {
    return internal::is_unpacked_bit | data_.size();
  }

  template <typename T> const T& stored_value(const T& arg, std::false_type) {
    return arg;
  }

  template <typename T>
  const T& stored_value(const std::reference_wrapper<T>& arg, std::false_type) {
    return arg.get();
  }

  template <typename T>
  const stored_type<T>& stored_value(const T& arg, std::true_type) {
    return storage_.push<stored_type<T>>(arg);
  }

  template <typename T> void emplace_arg(const T& arg) {
    data_.emplace_back(internal::make_arg<Context>(arg));
  }

 public:
  dynamic_format_arg_store() = default;
  ~dynamic_format_arg_store() = default;

  dynamic_format_arg_store(const dynamic_format_arg_store&) = delete;
  dynamic_format_arg_store& operator=(const dynamic_format_arg_store&) = delete;

  dynamic_format_arg_store(dynamic_format_arg_store&&) = default;
  dynamic_format_arg_store& operator=(dynamic_format_arg_store&&) = default;

  template <typename T> void push_back(const T& arg) {
    static_assert(
        !std::is_base_of<internal::named_arg_base<char_type>, T>::value,
        "Named arguments are not supported yet");
    emplace_arg(stored_value(arg, internal::need_dyn_copy_t<T, Context>{}));
  }

  template <typename T> void push_back(std::reference_wrapper<T> arg) {
    emplace_arg(arg.get());
  }
};

FMT_END_NAMESPACE

#endif /* FMT_DYN_ARGS_H_ */
