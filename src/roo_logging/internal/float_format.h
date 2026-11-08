#pragma once

#include <cfloat>
#include <cstddef>

#if FLT_RADIX == 2 && ((DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024) || \
                       (DBL_MANT_DIG == 24 && DBL_MAX_EXP == 128))
#define ROO_LOGGING_HAS_COMPACT_FLOAT 1
#else
#define ROO_LOGGING_HAS_COMPACT_FLOAT 0
#endif

namespace roo_logging {
namespace internal {

#if ROO_LOGGING_HAS_COMPACT_FLOAT
constexpr size_t kFloatBufferSize = 16;

/// Formats an IEEE binary32 or binary64 double using the default %g precision.
/// Rounds to nearest with ties to even, independently of locale, without heap
/// allocation. NUL-terminates @p buffer and returns the length excluding NUL.
size_t FormatFloat(double value, char (&buffer)[kFloatBufferSize]);
#endif

}  // namespace internal
}  // namespace roo_logging
