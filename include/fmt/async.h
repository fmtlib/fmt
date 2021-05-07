#ifndef FMT_ASYNC_H_
#define FMT_ASYNC_H_

#include "format.h"
#include <tuple>

namespace fmt {

template <typename Context> struct basic_async_entry {
protected:
    using char_type = typename Context::char_type;
    using format_arg = typename basic_format_args<Context>::format_arg;
    using arg_destructor = void (*)(void *p);

    basic_string_view<char_type> format_;
    unsigned long long desc_;
    arg_destructor dtor_;

    FMT_CONSTEXPR basic_async_entry(basic_string_view<char_type> format) : format_(format), desc_(0), dtor_(0) {}
    const format_arg* get_format_args() const;

    template <typename OutIt, typename T = OutIt>
    using enable_out = enable_if_t<detail::is_output_iterator<OutIt, char_type>::value, T>;

    void destruct() { if (dtor_) dtor_(this); }
public:
    struct dtor_sentry {
        dtor_sentry(basic_async_entry& entry) : entry_(entry) {}
        ~dtor_sentry() { entry_.destruct(); }
        basic_async_entry& entry_;
    };

    // libfmt public APIs
    std::basic_string<char_type> format() const { return vformat(format_, {desc_, get_format_args()}); }

    template <typename OutIt>
    auto format_to(OutIt out) const -> enable_out<OutIt> {
        return vformat_to(out, format_, {desc_, get_format_args()});
    }

    // template <typename OutIt>
    // auto format_to(OutIt out, size_t n) const -> enable_out<OutIt, format_to_n_result<OutIt>> {
    //     return vformat_to(detail::truncating_iterator<OutIt>(out, n), format_, {desc_, get_format_args()});
    // }

    size_t formatted_size() const {
        detail::counting_buffer<> buf;
        format_to(buf);
        return buf.count();
    }

    void print(std::FILE* file = stdout) const { return vprint(file, format_, {desc_, get_format_args()}); }

};

template <typename Context, typename... Args> struct async_entry : basic_async_entry<Context> {
    using entry = basic_async_entry<Context>;

    format_arg_store<Context, Args...> arg_store_;

    template <typename S>
    FMT_CONSTEXPR async_entry(const S& format_str, const Args&... args) : entry(to_string_view(format_str)), arg_store_(args...) {
        entry::desc_ = arg_store_.desc;
    }
    FMT_CONSTEXPR void set_dtor(typename entry::arg_destructor dtor) { this->dtor_ = dtor; }
};

template <typename Context>
inline const typename basic_async_entry<Context>::format_arg* basic_async_entry<Context>::get_format_args() const {
    union obfuscated_args {
        const detail::value<Context>* values_;
        const format_arg* args_;
        intptr_t pointer_; // more efficient to add integer with size, as the compiler is able to avoid emitting branch
    } args;
    auto& entry = static_cast<const async_entry<Context>&>(*this);
    args.values_ = entry.arg_store_.data_.args_;
    if (entry.desc_ & detail::has_named_args_bit) {
        args.pointer_ += (desc_ & detail::is_unpacked_bit) ? sizeof(*args.args_) : sizeof(*args.values_);
    }
    return args.args_;
}

//
// A stored entry looks like:
// -----------------------------------------------------------------------
// | basic_async_entry | arg_store | stored_objs... | stored_buffers... |
// -----------------------------------------------------------------------
//
namespace async {
namespace detail {
namespace detail = fmt::detail;
template <typename T> using decay_t = typename std::decay<T>::type;
template <typename T> using add_const_t = typename std::add_const<T>::type;
template <typename...> struct disjunction : std::false_type {};
template <typename B1> struct disjunction<B1> : B1 {};
template <typename B1, typename... Bn> struct disjunction<B1, Bn...> : conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};

enum class store_method {
    numeric,        // stored by libfmt as numeric value, no need for extra storage
    object,         // stored by libfmt as object, copy/move construct the object manually (requires properly calling destructor)
    buffer,         // string/hexdump, store the string/binary trivially as buffer.
    constexpr_str   // compile-time string, can be stored as pointer directly (requires c++20 is_constant_evaluated())
};

template <store_method method> using store_method_constant = std::integral_constant<store_method, method>;
using store_as_object = store_method_constant<store_method::object>;
using store_as_buffer = store_method_constant<store_method::buffer>;

template <typename Type>
using stored_as_numeric = std::integral_constant<bool, detail::is_arithmetic_type(Type::value) || Type::value == detail::type::pointer_type>;
template <typename Type>
using stored_as_string = std::integral_constant<bool, Type::value == detail::type::cstring_type || Type::value == detail::type::string_type>;
template <typename T> struct is_basic_string : std::false_type {};
template <typename C, typename T, typename A> struct is_basic_string<std::basic_string<C, T, A>> : std::true_type {};
template <typename Type, typename T>
using stored_as_string_object = std::integral_constant<bool, stored_as_string<Type>::value && is_basic_string<remove_reference_t<T>>::value && std::is_rvalue_reference<T>::value>;

struct custom_store_method_checker {
    template <typename Arg, typename Context, typename RawT = decay_t<Arg>, typename Formatter = typename Context::template formatter_type<RawT>>
    static enable_if_t<has_formatter<RawT, Context>::value, std::tuple<std::true_type, decltype(Formatter::store(std::declval<char*&>(), std::declval<Arg>()))>> test(double);
    template <typename Arg, typename Context, typename RawT = decay_t<Arg>, typename Formatter = typename Context::template formatter_type<RawT>>
    static enable_if_t<has_formatter<RawT, Context>::value, std::tuple<std::false_type, decltype(Formatter::store(std::declval<char*>(), std::declval<Arg>()))>> test(int);
    template <typename Arg, typename Context>
    static std::tuple<std::false_type, store_as_object> test(...);
};

template <typename Arg, typename Context>
struct custom_store_method {
    using transformed_type = typename std::tuple_element<1, decltype(custom_store_method_checker::template test<Arg, Context>(0))>::type;
    static constexpr bool custom_store = std::tuple_element<0, decltype(custom_store_method_checker::template test<Arg, Context>(0))>::type::value && !std::is_same<transformed_type, store_as_object>::value;
    using store_type = conditional_t<std::is_same<transformed_type, store_as_object>::value, store_as_object, store_as_buffer>;
};

template <typename Arg, typename Context, typename Type = detail::mapped_type_constant<remove_reference_t<Arg>, Context>>
struct stored_method_constant : std::integral_constant<store_method,  // using class (not template using) to allow easier partial specialization.
    stored_as_numeric<Type>::value ? store_method::numeric :
    stored_as_string<Type>::value ? (stored_as_string_object<Type, Arg>::value ? store_method::object : store_method::buffer) :
    // store_method::object> {};
    custom_store_method<Arg, Context>::store_type::value> {};

// Check for integer_sequence
#if defined(__cpp_lib_integer_sequence) || FMT_MSC_VER >= 1900
template <typename T, T... N>
using integer_sequence = std::integer_sequence<T, N...>;
template <size_t... N> using index_sequence = std::index_sequence<N...>;
template <size_t N> using make_index_sequence = std::make_index_sequence<N>;
#else
template <typename T, T... N> struct integer_sequence {
  using value_type = T;

  static FMT_CONSTEXPR size_t size() { return sizeof...(N); }
};

template <size_t... N> using index_sequence = integer_sequence<size_t, N...>;

template <typename T, size_t N, T... Ns>
struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Ns...> {};
template <typename T, T... Ns>
struct make_integer_sequence<T, 0, Ns...> : integer_sequence<T, Ns...> {};

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;
#endif

template <typename... Args>
struct arg_transformer {
    using arg_tuple = std::tuple<Args...>;
    template <size_t N> using arg_at = typename std::tuple_element<N, arg_tuple>::type;
    using type_tuple = std::tuple<decay_t<Args>...>;
    template <size_t N> using type_at = typename std::tuple_element<N, type_tuple>::type;
    template <size_t N> using size_at = std::integral_constant<size_t, sizeof(type_at<N>)>;
    template <typename Context, size_t N> using objsize_at = conditional_t<stored_method_constant<arg_at<N>, Context>::value == store_method::object, size_at<N>, std::integral_constant<size_t, 0>>;
    template <typename Context, size_t N> struct objsizesum_at : std::integral_constant<size_t, conditional_t<N == 0, std::integral_constant<size_t, 0>, objsizesum_at<Context, N - 1>>::value + objsize_at<Context, N>::value> {};
    template <typename Context, size_t N> using objoffset_at = conditional_t<N == 0, std::integral_constant<size_t, 0>, objsizesum_at<Context, N - 1>>;
};

template <typename Arg, typename Context, typename Type = detail::mapped_type_constant<remove_reference_t<Arg>, Context>>
using transformed_arg_type = conditional_t<custom_store_method<Arg, Context>::custom_store, typename custom_store_method<Arg, Context>::transformed_type,
                             conditional_t<stored_as_string<Type>::value && !stored_as_string_object<Type, Arg>::value, basic_string_view<typename Context::char_type>, add_const_t<remove_reference_t<Arg>>&>
                             >;

template <typename Context, typename... Args>
struct async_entry_constructor {
    template <typename S>
    static size_t construct(void* buf, const S& format_str, Args... args) {
        return async_entry_constructor<Context, Args...>(buf, format_str, range(), std::forward<Args>(args)...).get_total_size();
    }

private:
    using entry = async_entry<Context, decay_t<transformed_arg_type<Args, Context>>...>;
    using trans = arg_transformer<Args...>;
    using char_type = typename Context::char_type;
    using range = make_index_sequence<sizeof...(Args)>;

    template <typename S, size_t... Indice>
    FMT_CONSTEXPR async_entry_constructor(void* buf, const S& format_str, index_sequence<Indice...>, Args... args) : pentry(reinterpret_cast<char*>(buf)), pBuffer(get_buffer_store(buf)) {
        auto p = new(buf) entry(format_str, store<Indice>(std::forward<Args>(args))...);
        if (disjunction<need_destruct<Indice>...>::value) p->set_dtor(destructor<Indice...>::dtor);
    }

    template <size_t N> using arg_at = typename trans::template arg_at<N>;
    template <size_t N> using type_at = typename trans::template type_at<N>;

    template <size_t N> using need_destruct = std::integral_constant<bool, stored_method_constant<arg_at<N>, Context>::value == store_method::object && std::is_destructible<arg_at<N>>::value && !std::is_trivially_destructible<arg_at<N>>::value>;
    template <size_t N> static bool destruct(void *p, std::true_type) { reinterpret_cast<type_at<N>*>(p + sizeof(entry) + trans::template objoffset_at<Context, N>::value)->~type_at<N>(); return true; }
    template <size_t N> static bool destruct(void *p, std::false_type) { return false; }
    template <size_t... Indice> struct destructor {
        static void dummy(...) {}
        static void dtor(void *p) { dummy(destruct<Indice>(p, need_destruct<Indice>())...); }
    };

    template <size_t N> transformed_arg_type<arg_at<N>, Context> store(arg_at<N> arg) {
        using Arg = arg_at<N>;
        using select_store_method = custom_store_method<Arg, Context>;
        using mapped_type = detail::mapped_type_constant<remove_reference_t<Arg>, Context>;
        if constexpr (select_store_method::custom_store == true) {
            using Formatter = typename Context::template formatter_type<decay_t<Arg>>;

            return Formatter::store(pBuffer, std::forward<Arg>(arg));
        }
        else if constexpr (stored_as_string<mapped_type>::value && !stored_as_string_object<mapped_type, Arg>::value) {
            return copy_string(pBuffer, detail::arg_mapper<Context>().map(std::forward<Arg>(arg)));
        }
        else if constexpr (stored_as_numeric<mapped_type>::value) {
            return std::forward<Arg>(arg);
        }
        else {
            char* const pobjs = pentry + sizeof(entry);
            char* const pobj = pobjs + trans::template objoffset_at<Context, N>::value;
            using Type = type_at<N>;
            auto p = new(pobj) Type(std::forward<Arg>(arg));
            return *p;
        }
    }

    static basic_string_view<char_type> copy_string(char*& pBuffer, const char_type* cstr) {
        if constexpr (std::is_same<char_type, wchar_t>::value) {
            wchar_t* pStart = reinterpret_cast<wchar_t*>(pBuffer);
            wchar_t* pEnd = wcpcpy(pStart, cstr);
            pBuffer = reinterpret_cast<char*>(pEnd);
            return basic_string_view<wchar_t>(pStart, pEnd - pStart);
        }
        else {
            char* pStart = pBuffer;
            char* pEnd = stpcpy(pStart, cstr);
            pBuffer = pEnd;
            return basic_string_view<char>(pStart, pEnd - pStart);
        }
    }
    static basic_string_view<char_type> copy_string(char*& pBuffer, basic_string_view<char_type> sv) {
        char_type* pStart = reinterpret_cast<char_type*>(pBuffer);
        size_t size = sizeof(char_type) * sv.size();
        std::memcpy(pStart, sv.data(), size);
        pBuffer += size;
        return basic_string_view<char_type>(pStart, sv.size());
    }

    static FMT_CONSTEXPR char* get_buffer_store(void* buf) {
        char* const pentry = reinterpret_cast<char*>(buf);      // entry will be constructed here
        char* const pobjs = pentry + sizeof(entry);             // objects will be stored starting here
        char* const pbufs = pobjs + get_obj_size();             // buffers will be stored starting here
        return pbufs;
    }
    static constexpr size_t get_obj_size() { return trans::template objsizesum_at<Context, sizeof...(Args) - 1>::value; }
    constexpr size_t get_total_size() const { return pBuffer - pentry; }
    char* const pentry;
    char* pBuffer;
};

}  // namespace detail

template <typename S, typename... Args, typename Char = char_t<S>>
inline size_t store(void* buf, const S& format_str, Args&&... args) {
    using Context = buffer_context<Char>;
    using Constructor = detail::async_entry_constructor<Context, Args&&...>;
    return Constructor::construct(buf, format_str, std::forward<Args>(args)...);
}

template <typename Context>
inline auto format(basic_async_entry<Context>& entry) -> decltype(entry.format()) {
    typename basic_async_entry<Context>::dtor_sentry _(entry);
    return entry.format();
}

template <typename OutIt, typename Context>
inline auto format_to(basic_async_entry<Context>& entry, OutIt out) -> decltype(entry.format_to(out)) {
    typename basic_async_entry<Context>::dtor_sentry _(entry);
    return entry.format_to(out);
}

// template <typename OutIt, typename Context>
// inline auto format_to(basic_async_entry<Context>& entry, OutIt out, size_t n) -> decltype(entry.format_to_n(out, n)) {
//     typename basic_async_entry<Context>::dtor_sentry _(entry);
//     return entry.format_to(out, n);
// }

template <typename Context>
inline void print(basic_async_entry<Context>& entry, std::FILE* f = stdout) {
    typename basic_async_entry<Context>::dtor_sentry _(entry);
    entry.print(f);
}

// TODO should we add wrappers like this?
// template <typename Context = format_context>
// inline auto format(void* entry) -> decltype(format(std::declval<basic_async_entry<Context>&>())) {
//     return format(*reinterpret_cast<basic_async_entry<Context>*>(entry));
// }
// inline auto wformat(void* entry) -> decltype(format<wformat_context>(entry)) { return format<wformat_context>(entry); }

}  // namespace async

template <typename S, typename... Args, typename Char = char_t<S>>
inline auto make_async_entry(const S& format_str, Args&&... args) -> async_entry<buffer_context<Char>, remove_reference_t<Args>...> {
    return { format_str, args... };
}

template <typename S, typename... Args, typename Char = char_t<S>>
inline size_t store_async_entry(void* buf, const S& format_str, Args&&... args) {
    return async::store(buf, format_str, std::forward<Args>(args)...);
}

}  // namespace fmt
#endif  // FMT_ASYNC_H_
