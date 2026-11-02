#pragma once
#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#include "roo_logging/buffered_log_stream.h"
namespace roo_logging {
/// Preserves the IDF stream name over the framework-independent formatter.
using EspidfLogStream = BufferedLogStream;
}  // namespace roo_logging
#endif
