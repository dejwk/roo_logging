#pragma once

#include "roo_flags.h"
#include "roo_time.h"

namespace roo_logging {
using WallTimeClockPtr = ::roo_time::WallTimeClock*;
}

// Which wall time clock to use. If nullptr (default), uses uptime in the logs,
// instead of wall time.
ROO_DECLARE_FLAG(roo_logging::WallTimeClockPtr, roo_logging_wall_time_clock);

// Which time zone to use to report wall time. Ignored if the
// roo_logging_wall_time_clock flag is nullptr.
ROO_DECLARE_FLAG(roo_time::TimeZone, roo_logging_timezone);

// Whether to prepend prefix (time, file, line number, etc.) in front of each
// log line.
ROO_DECLARE_FLAG(bool, roo_logging_prefix);

// Whether to ANSI-color the log lines written to stderr.
ROO_DECLARE_FLAG(bool, roo_logging_colorlogtostderr);

// Messages with severity below this level are not logged at all.
ROO_DECLARE_FLAG(uint8_t, roo_logging_minloglevel);

// The global value of ROO_STRIP_LOG. All the messages logged to
// LOG(XXX) with severity less than ROO_STRIP_LOG will not be displayed.
// If it can be determined at compile time that the message will not be
// printed, the statement will be compiled out.
//
// Example: to strip out all INFO and WARNING messages, use the value
// of 2 below. To make an exception for WARNING messages from a single
// file, add "#define ROO_STRIP_LOG 1" to that file _before_ including
// base/logging.h
#ifndef ROO_STRIP_LOG
#define ROO_STRIP_LOG 0
#endif

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON() 0
#else
#define DCHECK_IS_ON() 1
#endif

#if defined(VLOG_LEVEL)
#define VLOG_IS_ON(verboselevel) ((verboselevel) >= VLOG_LEVEL)
#else
#define VLOG_IS_ON(verboselevel) 0
#endif

#if !defined(ROO_LOGGING_HAVE_SYMBOLIZE)
// For emulation on Linux.
#if defined(__linux__)
#define ROO_LOGGING_HAVE_SYMBOLIZE
#else
#endif
#endif

#ifdef ROO_LOGGING_HAVE_SYMBOLIZE
#define ROO_LOGGING_SYMBOLIZE_STACKTRACE 1
#endif
