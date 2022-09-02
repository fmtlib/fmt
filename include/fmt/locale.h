// Formatting library for C++ - optional locale support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_LOCALE_H_
#define FMT_LOCALE_H_

#include <ios>       // std::ios_base
#include <iterator>  // std::ostreambuf_iterator

#include "core.h"

FMT_BEGIN_NAMESPACE

// A locale facet that formats numeric values in UTF-8.
template <typename Locale> class num_format_facet : public Locale::facet {
 public:
  static FMT_API typename Locale::id id;

  using iter_type = std::ostreambuf_iterator<char>;

  auto put(iter_type out, std::ios_base& str, char fill,
           unsigned long long val) const -> iter_type {
    return do_put(out, str, fill, val);
  }

 protected:
  virtual auto do_put(iter_type out, std::ios_base& str, char fill,
                      unsigned long long val) const -> iter_type = 0;
};

FMT_END_NAMESPACE

#endif  // FMT_LOCALE_H_