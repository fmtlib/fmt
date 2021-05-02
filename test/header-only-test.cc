// Header-only configuration test

#include "fmt/core.h"

#ifndef FMT_HEADER_ONLY
#  error "Not in the header-only mode."
#endif
