// Formatting library for C++ - optional locale support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_LOCALE_H_
#define FMT_LOCALE_H_

#include <locale>  // std::num_put

#include "core.h"

FMT_BEGIN_NAMESPACE

// A locale facet that formats numeric values in UTF-8.
template <typename Char = char>
class num_format_facet : public std::num_put<Char> {
 public:
  static FMT_API std::locale::id id;
};

FMT_END_NAMESPACE

#endif  // FMT_LOCALE_H_