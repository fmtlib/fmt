#define FMT_HEADER_ONLY
#include "format.h"

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
    // char stored_objs_[0];
    // char stored_bufs_[];

    template <typename S>
    FMT_CONSTEXPR format_entry(const S& format_str, const Args&... args) : entry(to_string_view(format_str)), arg_store_(args...) {
        entry::desc_ = arg_store_.desc;
    }


};



template <typename... Args> struct obj_store;






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





enum class store_method {
    numeric,        // stored by libfmt as numeric value, no need for extra storage
    object,         // stored by libfmt as object, copy/move construct the object manually (requires properly calling destructor)
    buffer,         // string/hexdump, store the string/binary trivially as buffer.
    constexpr_str   // compile-time string, can be stored as pointer directly (requires c++20 is_constant_evaluated())
};

template <typename Type>
using stored_as_numeric = std::integral_constant<bool, fmt::detail::is_arithmetic_type(Type::value) || Type::value == fmt::detail::type::pointer_type>;
template <typename Type>
using stored_as_string = std::integral_constant<bool, Type::value == fmt::detail::type::cstring_type || Type::value == fmt::detail::type::string_type>;
template <typename T> struct is_basic_string : std::false_type {};
template <typename C, typename T, typename A> struct is_basic_string<std::basic_string<C, T, A>> : std::true_type {};
template <typename Type, typename T>
using stored_as_string_object = std::integral_constant<bool, stored_as_string<Type>::value && is_basic_string<std::remove_reference_t<T>>::value && std::is_rvalue_reference<T>::value>;

template <typename T, typename Context, typename Type = fmt::detail::mapped_type_constant<std::remove_reference_t<T>, Context>>
struct stored_method_constant : std::integral_constant<store_method,  // using class (not template using) to allow easier partial specialization.
    stored_as_numeric<Type>::value ? store_method::numeric :
    stored_as_string<Type>::value ? (stored_as_string_object<Type, T>::value ? store_method::object : store_method::buffer) :
    store_method::object> {};

struct stored_objs_dtor_base : std::false_type {
    static void destruct(void* p) {}
};

template <typename Base, ptrdiff_t offset, typename T>
struct stored_objs_dtor_gen : std::true_type {
    static void destruct(void* p) {
        Base::destruct(p);
        reinterpret_cast<T*>((reinterpret_cast<char*>(p) + offset))->~T();
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


template <size_t... Is> struct index_list {};

// Collects internal details for generating index ranges [MIN, MAX)
inline namespace detail
{
    // Induction step
    template <size_t MIN, size_t N, size_t... Is>
    struct range_builder : public range_builder<MIN, N - 1, N - 1, Is...>  {};

    // Base step
    template <size_t MIN, size_t... Is> struct range_builder<MIN, MIN, Is...> { typedef index_list<Is...> type; };
}

// Meta-function that returns a [MIN, MAX) index range
template<size_t MIN, size_t MAX>
using index_range = typename range_builder<MIN, MAX>::type;

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



template <typename Context> inline
void print_format_entry(basic_format_entry<Context>& entry) {
    auto& full_entry = static_cast<format_entry<Context>&>(entry);
    vprint(entry.format_, {entry.desc_, full_entry.arg_store_.data_.args_});    ///// args_ or args + 1 depends on desc_ & detail::has_named_args_bit
    entry.destruct();
}


struct format_queue {

};

}
using namespace fmtlog;


#include <iostream>



int main() {
    std::cout << sizeof(basic_format_entry<format_context>) << std::endl;
    auto entry = mk_format_entry("The answer is {}\n", 42);
    print_format_entry(entry);

    char buf[2000];
    size_t size = store_format_entry(buf, "The answer is {}{}\n", std::to_string(4), 2);
    std::cout << size << std::endl;
    auto& entryref = entry;
    print_format_entry(reinterpret_cast<decltype(entryref)>(buf[0]));

    std::cout << &entry << "\n" << &(entry.arg_store_) << "\n" << sizeof(entry.arg_store_) << std::endl;
}
