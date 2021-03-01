#define FMT_HEADER_ONLY
#include "fmtlog.h"

using namespace fmtlog;

template <typename S, typename... Args, typename Char = char_t<S>>
inline auto mk_format_entry(const S& format_str, Args&&... args) -> format_entry<buffer_context<Char>, remove_reference_t<Args>...> {
    return { format_str, args... };
}


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
