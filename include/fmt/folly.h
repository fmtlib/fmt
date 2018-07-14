// Formatting library for C++ - folly::StringPiece formatter
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_FOLLY_H_
#define FMT_FOLLY_H_

#include <folly/Range.h>
#include "core.h"

FMT_BEGIN_NAMESPACE
template <typename Ctx>
inline internal::typed_value<Ctx, internal::string_type>
    make_value(folly::StringPiece s) {
  return string_view(s.data(), s.size());
}
FMT_END_NAMESPACE

#endif  // FMT_FOLLY_H_
