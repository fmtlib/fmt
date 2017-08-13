#include "fmt/printf.h"

namespace fmt {

template <typename Char>
void printf(basic_writer<Char> &w, basic_string_view<Char> format, args args);

FMT_FUNC int vfprintf(std::FILE *f, string_view format, printf_args args) {
  memory_buffer buffer;
  printf(buffer, format, args);
  std::size_t size = buffer.size();
  return std::fwrite(
        buffer.data(), 1, size, f) < size ? -1 : static_cast<int>(size);
}

#ifndef FMT_HEADER_ONLY
template void printf_context<char>::format(string_view, buffer &);
template void printf_context<wchar_t>::format(wstring_view, wbuffer &);
#endif
}
