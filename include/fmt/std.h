// Formatting library for C++ - formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_STD_H_
#define FMT_STD_H_

#include <thread>

#include "ostream.h"

#if FMT_HAS_INCLUDE(<version>)
#  include <version>
#endif

#ifdef __cpp_lib_filesystem
#  include <filesystem>

FMT_BEGIN_NAMESPACE
template <> struct formatter<std::filesystem::path> : ostream_formatter {};
FMT_END_NAMESPACE
#endif

FMT_BEGIN_NAMESPACE
template <> struct formatter<std::thread::id> : ostream_formatter {};
FMT_END_NAMESPACE

#endif  // FMT_STD_H_
