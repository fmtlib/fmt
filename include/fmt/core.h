#include "base.h"

// Using fmt::format via fmt/core.h has been deprecated since version 11
// and now requires an explicit opt in.
#ifdef FMT_DEPRECATED_HEAVY_CORE
#  include "format.h"
#endif
