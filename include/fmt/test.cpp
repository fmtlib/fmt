#define FMT_HEADER_ONLY
#include "format.h"
#include <tuple>

namespace fmtlog {
using namespace fmt;

template <typename Context> struct basic_format_entry {
    using char_type = typename Context::char_type;
    using format_arg = basic_format_arg<Context>;
    using arg_destructor = void (*)(void *p);

    basic_string_view<char_type> format_;
    unsigned long long desc_;
    arg_destructor dtor_;

    void destruct() {
        if (dtor_) {
            fmt::print("calling dtor\n");
            dtor_(this);
        } else {
            fmt::print("dtor is empty\n");
        }
    }
    FMT_CONSTEXPR basic_format_entry(basic_string_view<char_type> format) : format_(format), desc_(0), dtor_(0) {}
};

template <typename Context, typename... Args> struct format_entry : basic_format_entry<Context> {
    using entry = basic_format_entry<Context>;

    format_arg_store<Context, Args...> arg_store_;

    template <typename S>
    FMT_CONSTEXPR format_entry(const S& format_str, const Args&... args) : entry(to_string_view(format_str)), arg_store_(args...) {
        entry::desc_ = arg_store_.desc;
    }
};

//
// A stored entry looks like:
// -----------------------------------------------------------------------
// | basic_format_entry | arg_store | stored_objs... | stored_buffers... |
// -----------------------------------------------------------------------
//

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
using stored_as_numeric = std::integral_constant<bool, fmt::detail::is_arithmetic_type(Type::value) || Type::value == fmt::detail::type::pointer_type>;
template <typename Type>
using stored_as_string = std::integral_constant<bool, Type::value == fmt::detail::type::cstring_type || Type::value == fmt::detail::type::string_type>;
template <typename T> struct is_basic_string : std::false_type {};
template <typename C, typename T, typename A> struct is_basic_string<std::basic_string<C, T, A>> : std::true_type {};
template <typename Type, typename T>
using stored_as_string_object = std::integral_constant<bool, stored_as_string<Type>::value && is_basic_string<std::remove_reference_t<T>>::value && std::is_rvalue_reference<T>::value>;

struct custom_store_method_checker {
    template <typename Arg, typename Context, typename RawT = std::decay_t<Arg>, typename Formatter = typename Context::template formatter_type<RawT>>
    static std::enable_if_t<has_formatter<RawT, Context>::value, std::tuple<std::true_type, decltype(Formatter::store(std::declval<char*&>(), std::declval<Arg>()))>> test(int);
    template <typename Arg, typename Context>
    static std::tuple<std::false_type, store_as_object> test(...);
};

template <typename Arg, typename Context>
struct custom_store_method {
    using transformed_type = typename std::tuple_element<1, decltype(custom_store_method_checker::template test<Arg, Context>(0))>::type;
    static constexpr bool custom_store = std::tuple_element<0, decltype(custom_store_method_checker::template test<Arg, Context>(0))>::type::value && !std::is_same_v<transformed_type, store_as_object>;
    using store_type = std::conditional_t<std::is_same_v<transformed_type, store_as_object>, store_as_object, store_as_buffer>;
};

template <typename Arg, typename Context, typename Type = fmt::detail::mapped_type_constant<std::remove_reference_t<Arg>, Context>>
struct stored_method_constant : std::integral_constant<store_method,  // using class (not template using) to allow easier partial specialization.
    stored_as_numeric<Type>::value ? store_method::numeric :
    stored_as_string<Type>::value ? (stored_as_string_object<Type, Arg>::value ? store_method::object : store_method::buffer) :
    // store_method::object> {};
    custom_store_method<Arg, Context>::store_type::value> {};

struct stored_objs_dtor_base : std::integral_constant<size_t, 0> {
    static void destruct(void* p) {}
};

template <typename Base, ptrdiff_t offset, typename RawT>
struct stored_objs_dtor_gen : std::integral_constant<size_t, offset + sizeof(RawT)> {
    static void destruct(void* p) {
        reinterpret_cast<RawT*>((reinterpret_cast<char*>(p) + offset))->~RawT();
        Base::destruct(p);
    }
};

template <typename Context, ptrdiff_t offset, typename Base, typename T, typename... Args>
struct stored_objs_dtor_select {
    using RawT = std::remove_reference_t<T>;
    using type = std::conditional_t<stored_method_constant<T, Context>::value == store_method::object,
                                    typename stored_objs_dtor_select<Context, offset + sizeof(RawT), stored_objs_dtor_gen<Base, offset, RawT>, Args...>::type,
                                    typename stored_objs_dtor_select<Context, offset, Base, Args...>::type>;
};

template <typename Context, ptrdiff_t offset, typename Base>
struct stored_objs_dtor_select<Context, offset, Base, void> { using type = Base; };

template <typename Context, ptrdiff_t offset, typename... Args>
using stored_objs_dtor = typename stored_objs_dtor_select<Context, offset, stored_objs_dtor_base, Args..., void>::type;

// Collects internal details for generating index ranges [MIN, MAX)
inline namespace detail
{
    template <size_t...> struct index_list {};

    // Induction step
    template <size_t MIN, size_t N, size_t... Is>
    struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...>  {};

    // Base step
    template <size_t MIN, size_t... Is> struct range_builder<MIN, MIN, Is...> { typedef index_list<Is...> type; };
}

// Meta-function that returns a [MIN, MAX) index range
template<size_t MIN, size_t MAX>
using index_range = typename range_builder<MIN, MAX>::type;

template <typename... Args>
struct arg_transformer {
    using arg_tuple = std::tuple<Args...>;
    template <size_t N> using arg_at = typename std::tuple_element<N, arg_tuple>::type;
    using type_tuple = std::tuple<std::decay_t<Args>...>;
    template <size_t N> using type_at = typename std::tuple_element<N, type_tuple>::type;
    template <size_t N> using size_at = std::integral_constant<size_t, sizeof(type_at<N>)>;
    template <typename Context, size_t N> using objsize_at = std::conditional_t<stored_method_constant<arg_at<N>, Context>::value == store_method::object, size_at<N>, std::integral_constant<size_t, 0>>;
    template <typename Context, size_t N> struct objsizesum_at : std::integral_constant<size_t, std::conditional_t<N == 0, std::integral_constant<size_t, 0>, objsizesum_at<Context, N - 1>>::value + objsize_at<Context, N>::value> {};
    template <typename Context, size_t N> using objoffset_at = std::conditional_t<N == 0, std::integral_constant<size_t, 0>, objsizesum_at<Context, N - 1>>;
};

template <typename Arg, typename Context, typename Type = fmt::detail::mapped_type_constant<std::remove_reference_t<Arg>, Context>>
using transformed_arg_type = std::conditional_t<custom_store_method<Arg, Context>::custom_store, typename custom_store_method<Arg, Context>::transformed_type,
                             std::conditional_t<stored_as_string<Type>::value && !stored_as_string_object<Type, Arg>::value, basic_string_view<typename Context::char_type>, std::add_const_t<std::remove_reference_t<Arg>>&>
                             >;

template <typename Context, typename... Args>
struct format_entry_constructor {
    using Entry = format_entry<Context, std::decay_t<transformed_arg_type<Args, Context>>...>;
    using Dtor = stored_objs_dtor<Context, sizeof(Entry), Args...>;
    using Trans = arg_transformer<Args...>;

    template <typename S>
    static size_t construct(void* buf, const S& format_str, Args... args) {
        return format_entry_constructor<Context, Args...>(buf, format_str, range(), std::forward<Args>(args)...).get_total_size();
    }

private:
    using range = typename range_builder<0, sizeof...(Args)>::type;
    template <typename S, size_t... Indice>
    constexpr format_entry_constructor(void* buf, const S& format_str, index_list<Indice...>, Args... args) : pEntry(reinterpret_cast<char*>(buf)), pBuffer(get_buffer_store(buf)) {
        auto p = new(buf) Entry(format_str, store<Indice>(std::forward<Args>(args))...);
        p->dtor_ = Dtor::value ? Dtor::destruct : nullptr;
    }

    template <size_t N> using arg_at = typename Trans::template arg_at<N>;
    template <size_t N> transformed_arg_type<arg_at<N>, Context> store(typename Trans::template arg_at<N> arg) {
        using Arg = typename Trans::template arg_at<N>;
        using select_store_method = custom_store_method<Arg, Context>;
        using MappedType = fmt::detail::mapped_type_constant<std::remove_reference_t<Arg>, Context>;
        if constexpr (select_store_method::custom_store == true) {
            using Formatter = typename Context::template formatter_type<std::decay_t<Arg>>;

    // fmt::print("store custom arg {} @ {}\n", N, (void*)pBuffer);
            return Formatter::store(pBuffer, std::forward<Arg>(arg));
        }
        else if constexpr (stored_as_string<MappedType>::value && !stored_as_string_object<MappedType, Arg>::value) {
    // fmt::print("store string arg {} @ {}({})\n", N, (void*)pBuffer, fmt::detail::arg_mapper<Context>().map(std::forward<Arg>(arg)));
            return copy_string(pBuffer, fmt::detail::arg_mapper<Context>().map(std::forward<Arg>(arg)));
        }
        else if constexpr (stored_as_numeric<MappedType>::value) {
            return std::forward<Arg>(arg);
        }
        else {
            char* const pobjs = pEntry + sizeof(Entry);
            char* const pobj = pobjs + Trans::template objoffset_at<Context, N>::value;
            using Type = typename Trans::template type_at<N>;
            auto p = new(pobj) Type(std::forward<Arg>(arg));
            return *p;
        }
    }

    static basic_string_view<typename Context::char_type> copy_string(char*& pBuffer, const typename Context::char_type* cstr) {
        if constexpr (std::is_same_v<typename Context::char_type, wchar_t>) {
            wchar_t* pStart = reinterpret_cast<wchar_t*>(pBuffer);
            wchar_t* pEnd = wcpcpy(pStart, cstr);
            pBuffer = reinterpret_cast<char*>(pEnd);
    // fmt::print("copied wstring ({}) @ {}\n", basic_string_view<wchar_t>(pStart, pEnd - pStart), (void*)pStart);
            return basic_string_view<wchar_t>(pStart, pEnd - pStart);
        }
        else {
            char* pStart = pBuffer;
            char* pEnd = stpcpy(pStart, cstr);
            pBuffer = pEnd;
    // fmt::print("copied string ({}) @ {}\n", basic_string_view<char>(pStart, pEnd - pStart), (void*)pStart);
            return basic_string_view<char>(pStart, pEnd - pStart);
        }
    }
    static basic_string_view<typename Context::char_type> copy_string(char*& pBuffer, basic_string_view<typename Context::char_type> sv) {
        typename Context::char_type* pStart = reinterpret_cast<typename Context::char_type*>(pBuffer);
        size_t size = sizeof(typename Context::char_type) * sv.size();
        std::memcpy(pStart, sv.data(), size);
        pBuffer += size;
        return basic_string_view<typename Context::char_type>(pStart, sv.size());
    }

    static constexpr char* get_buffer_store(void* buf) {
        char* const pentry = reinterpret_cast<char*>(buf);      // entry will be constructed here
        char* const pobjs = pentry + sizeof(Entry);             // objects will be stored starting here
        char* const pbufs = pobjs + get_obj_size();             // buffers will be stored starting here
    // fmt::print("creating custom arg entry @ {}, {}, {}\n", (void*)pentry, (void*)pobjs, (void*)pbufs);
        return pbufs;
    }
    static constexpr size_t get_obj_size() { return Trans::template objsizesum_at<Context, sizeof...(Args) - 1>::value; }
    constexpr size_t get_total_size() const { return pBuffer - pEntry; }
    char* const pEntry;
    char* pBuffer;
};

template <typename S, typename... Args, typename Char = char_t<S>>
inline size_t store_format_entry(void* buf, const S& format_str, Args&&... args) {
    using Context = buffer_context<Char>;
    using Constructor = format_entry_constructor<Context, Args&&...>;
    return Constructor::construct(buf, format_str, std::forward<Args>(args)...);
}

#if 0
template <typename Context>
inline char* store_objs(void** stored_args, char* p) { return p; }

template <typename Context, typename T, typename... Args>
inline char* store_objs(void** stored_args, char* p, T t, Args... args) {
    using RawT = std::remove_reference_t<T>;
    if constexpr (stored_method_constant<T, Context>::value == store_method::object) {
        new(p) RawT(static_cast<T>(t));
        *stored_args = p;
        p += sizeof(RawT);
    }
    ++stored_args;
    return store_objs<Context, Args...>(stored_args, p, static_cast<Args>(args)...);
}

template <typename Context>
inline char* store_bufs(void** stored_args, char* p) { return p; }

template <typename Context, typename T, typename... Args>
inline char* store_bufs(void** stored_args, char* p, T t, Args... args) {
    using RawT = std::remove_reference_t<T>;
    if constexpr (stored_method_constant<T, Context>::value == store_method::buffer) {
        *stored_args = p;
        //p = copy_buffer(p, t);
    }
    ++stored_args;
    return store_bufs<Context, Args...>(stored_args, p, static_cast<Args>(args)...);
}

template <typename S, typename... Args, size_t... Indice>
inline auto make_format_entry(void* buf, const S& format_str, void* const (&stored_args)[sizeof...(Args)], index_list<Indice...>, Args&&... args) {
    using Char = char_t<S>;
    using Context = buffer_context<Char>;
    using entry = format_entry<Context, remove_reference_t<Args>...>;
    return new(buf) entry { format_str, std::forward<Args>(stored_args[Indice] == 0 ? args : *reinterpret_cast<std::remove_reference_t<Args>*>(stored_args[Indice]))... };
}

template <typename S, typename... Args, typename Char = char_t<S>>
inline size_t store_format_entry(void* buf, const S& format_str, Args&&... args) {
    using Context = buffer_context<Char>;
    using entry = format_entry<Context, remove_reference_t<Args>...>;
    using dtor = stored_objs_dtor<Context, sizeof(entry), Args&&...>;
    char* pentry = reinterpret_cast<char*>(buf);
    char* pobjs = pentry + sizeof(entry); // objects will be stored starting here
    void* stored_args[sizeof...(Args)] = {0}; // if an object is stored, it has a pointer other than 0
    char* pbufs = store_objs<Context, Args&&...>(stored_args, pobjs, std::forward<Args>(args)...);
    char* pend = store_bufs<Context, Args&&...>(stored_args, pbufs, std::forward<Args>(args)...);

    //auto p = new(buf) entry{ format_str, args... };
    auto p = make_format_entry(buf, format_str, stored_args, index_range<0, sizeof...(Args)>(), std::forward<Args>(args)...);
    p->dtor_ = dtor::value ? dtor::destruct : nullptr;
    return pend - pentry;
}
#endif

template <typename Context> inline
void print_format_entry(basic_format_entry<Context>& entry) {
    auto& full_entry = static_cast<format_entry<Context>&>(entry);
    vprint(entry.format_, {entry.desc_, &full_entry.arg_store_.data_.args_[entry.desc_ & fmt::detail::has_named_args_bit ? 1 : 0]});
    entry.destruct();
}


struct format_queue {

};

template <typename S, typename... Args, typename Char = char_t<S>>
inline auto mk_format_entry(const S& format_str, Args&&... args) -> format_entry<buffer_context<Char>, remove_reference_t<Args>...> {
    return { format_str, args... };
}

template <typename S, typename... Args, typename Char = char_t<S>>
inline size_t write_format_entry(void* buf, const S& format_str, Args&&... args) {
    using entry = format_entry<buffer_context<Char>, remove_reference_t<Args>...>;
    auto p = new(buf) entry{ format_str, args... };
    (void) p;
    return sizeof(entry);
}


}
using namespace fmtlog;


#include <iostream>



int main() {
    fmt::print("entry header: {}\n", sizeof(basic_format_entry<format_context>));
    auto entry = mk_format_entry("The answer is {}\n", 42);
    print_format_entry(entry);

    char buf[2000];
    size_t size = store_format_entry(buf, "This {} is {}{}\n", "answer", std::to_string(4), 2);
    fmt::print("entry size: {}\n", size);
    auto& entryref1 = entry;
    auto& entryref = reinterpret_cast<decltype(entryref1)>(buf[0]);
    print_format_entry(entryref);

    std::cout << &entryref << "\n" << &(entryref.arg_store_) << "\n" << sizeof(entryref.arg_store_) << std::endl;
}
