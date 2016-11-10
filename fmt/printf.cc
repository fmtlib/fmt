/*
 Formatting library for C++

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "format.h"
#include "printf.h"

namespace fmt {

namespace {

const char RESET_COLOR[] = "\x1b[0m";

}  // namespace

FMT_FUNC void print(std::FILE *f, CStringRef format_str, ArgList args) {
  MemoryWriter w;
  w.write(format_str, args);
  std::fwrite(w.data(), 1, w.size(), f);
}

FMT_FUNC void print(CStringRef format_str, ArgList args) {
  print(stdout, format_str, args);
}

FMT_FUNC void print_colored(Color c, CStringRef format, ArgList args) {
  char escape[] = "\x1b[30m";
  escape[3] = static_cast<char>('0' + c);
  std::fputs(escape, stdout);
  print(format, args);
  std::fputs(RESET_COLOR, stdout);
}

template <typename Char>
void printf(BasicWriter<Char> &w, BasicCStringRef<Char> format, ArgList args);

FMT_FUNC int fprintf(std::FILE *f, CStringRef format, ArgList args) {
  MemoryWriter w;
  printf(w, format, args);
  std::size_t size = w.size();
  return std::fwrite(w.data(), 1, size, f) < size ? -1 : static_cast<int>(size);
}

#ifndef FMT_HEADER_ONLY

template void PrintfFormatter<char>::format(CStringRef format);
template void PrintfFormatter<wchar_t>::format(WCStringRef format);

#endif  // FMT_HEADER_ONLY

}  // namespace fmt
