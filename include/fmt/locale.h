// Formatting library for C++ - locale support
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_LOCALE_H_
#define FMT_LOCALE_H_

#include "format.h"
#include <locale>

FMT_BEGIN_NAMESPACE
class locale {
 private:
  std::locale locale_;

 public:
  explicit locale(std::locale loc = std::locale()) : locale_(loc) {}
  std::locale get() { return locale_; }
};
FMT_END_NAMESPACE

#endif  // FMT_LOCALE_
