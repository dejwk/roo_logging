#pragma once

#if defined(__linux__)

#include <cstddef>

namespace roo_logging {
namespace internal {

/// Collects up to @p max_depth stack addresses in @p result. Skips its own
/// frame and @p skip_count additional frames. Returns the number collected.
int GetStackTrace(void** result, int max_depth, int skip_count);

/// Formats a stack frame into @p buffer, right-aligning @p pc in a
/// pointer-width field. Pass nullptr for @p symbol to omit the symbol.
/// Truncates to @p capacity and NUL-terminates when storage is available.
void FormatStackTraceLine(char* buffer, size_t capacity, const char* prefix,
                          const void* pc, const char* symbol);

}  // namespace internal
}  // namespace roo_logging

#endif  // defined(__linux__)
