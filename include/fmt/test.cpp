#define FMT_HEADER_ONLY
#include "async.h"

using namespace fmt;

#include <iostream>



int main() {
    fmt::print("entry header: {}\n", sizeof(basic_async_entry<format_context>));
    auto entry = make_async_entry("The answer is {}\n", 42);
    async::print(entry);

    char buf[2000];
    size_t size = store_async_entry(buf, "This {} is {}{}\n", "answer", std::to_string(4), 2);
    fmt::print("entry size: {}\n", size);
    auto& entryref1 = entry;
    auto& entryref = reinterpret_cast<decltype(entryref1)>(buf[0]);
    async::print(entryref);

    std::cout << &entryref << "\n" << &(entryref.arg_store_) << "\n" << sizeof(entryref.arg_store_) << std::endl;
}
