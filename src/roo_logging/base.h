// Based on Google logging library, subject to the copyright below.
//
// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <Arduino.h>
#include <inttypes.h>

#include <cstring>

#include "roo_logging/config.h"
#include "roo_logging/log_severity.h"
#include "roo_logging/predict.h"
#include "roo_logging/stream.h"
#include "roo_time.h"

// A few definitions of macros that don't generate much code.  Since
// LOG(INFO) and its ilk can be used all over the code, it's
// better to have compact code for these operations.

#if ROO_STRIP_LOG == 0
#define COMPACT_ROO_LOG_INFO ::roo_logging::LogMessage(__FILE__, __LINE__)
#define LOG_TO_STRING_INFO(message)             \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_INFO, message)
#else
#define COMPACT_ROO_LOG_INFO ::roo_logging::NullStream()
#define LOG_TO_STRING_INFO(message) ::roo_logging::NullStream()
#endif

#if ROO_STRIP_LOG <= 1
#define COMPACT_ROO_LOG_WARNING                 \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_WARNING)
#define LOG_TO_STRING_WARNING(message)          \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_WARNING, message)
#else
#define COMPACT_ROO_LOG_WARNING ::roo_logging::NullStream()
#define LOG_TO_STRING_WARNING(message) ::roo_logging::NullStream()
#endif

#if ROO_STRIP_LOG <= 2
#define COMPACT_ROO_LOG_ERROR                   \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_ERROR)
#define LOG_TO_STRING_ERROR(message)            \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_ERROR, message)
#else
#define COMPACT_ROO_LOG_ERROR ::roo_logging::NullStream()
#define LOG_TO_STRING_ERROR(message) ::roo_logging::NullStream()
#endif

#if ROO_STRIP_LOG <= 3
#define COMPACT_ROO_LOG_FATAL ::roo_logging::LogMessageFatal(__FILE__, __LINE__)
#define LOG_TO_STRING_FATAL(message)            \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGINGG_FATAL, message)
#else
#define COMPACT_ROO_LOG_FATAL ::roo_logging::NullStreamFatal()
#define LOG_TO_STRING_FATAL(message) ::roo_logging::NullStreamFatal()
#endif

// For DFATAL, we want to use LogMessage (as opposed to
// LogMessageFatal), to be consistent with the original behavior.
#if !DCHECK_IS_ON()
#define COMPACT_ROO_LOG_DFATAL COMPACT_ROO_LOG_ERROR
#elif ROO_STRIP_LOG <= 3
#define COMPACT_ROO_LOG_DFATAL                  \
  ::roo_logging::LogMessage(__FILE__, __LINE__, \
                            ::roo_logging::ROO_LOGGING_FATAL)
#else
#define COMPACT_ROO_LOG_DFATAL ::roo_logging::NullStreamFatal()
#endif

#define ROO_LOGGING_PLOG(severity, counter)                               \
  ::roo_logging::ErrnoLogMessage(                                         \
      __FILE__, __LINE__, ::roo_logging::ROO_LOGGING_##severity, counter, \
      &::roo_logging::LogMessage::SendToLog)

// Use macro expansion to create, for each use of LOG_EVERY_N(), static
// variables with the __LINE__ expansion as part of the variable name.
#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base##line

#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define SOME_KIND_OF_LOG_EVERY_N(severity, n, what_to_do)          \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0;       \
  ++LOG_OCCURRENCES;                                               \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n;     \
  if (LOG_OCCURRENCES_MOD_N == 1)                                  \
  ::roo_logging::LogMessage(__FILE__, __LINE__,                    \
                            ::roo_logging::ROO_LOGGING_##severity, \
                            LOG_OCCURRENCES, &what_to_do)          \
      .stream()

#define SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n, what_to_do)       \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0;                  \
  ++LOG_OCCURRENCES;                                                          \
  if (condition &&                                                            \
      ((LOG_OCCURRENCES_MOD_N = (LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
  ::roo_logging::LogMessage(__FILE__, __LINE__,                               \
                            ::roo_logging::ROO_LOGGING_##severity,            \
                            LOG_OCCURRENCES, &what_to_do)                     \
      .stream()

#define SOME_KIND_OF_LOG_FIRST_N(severity, n, what_to_do)          \
  static int LOG_OCCURRENCES = 0;                                  \
  if (LOG_OCCURRENCES <= n) ++LOG_OCCURRENCES;                     \
  if (LOG_OCCURRENCES <= n)                                        \
  ::roo_logging::LogMessage(__FILE__, __LINE__,                    \
                            ::roo_logging::ROO_LOGGING_##severity, \
                            LOG_OCCURRENCES, &what_to_do)          \
      .stream()

#define SOME_KIND_OF_LOG_EVERY_T(severity, interval)                          \
  constexpr roo_time::Interval LOG_TIME_PERIOD =                              \
      ::roo_time::Seconds(interval);                                          \
  static roo_time::Uptime LOG_PREVIOUS_TIME = roo_time::Uptime();             \
  const roo_time::Uptime LOG_CURRENT_TIME = roo_time::Uptime::Now();          \
  const auto LOG_TIME_DELTA = LOG_CURRENT_TIME - LOG_PREVIOUS_TIME;           \
  if (LOG_TIME_DELTA > LOG_TIME_PERIOD) LOG_PREVIOUS_TIME = LOG_CURRENT_TIME; \
  if (LOG_TIME_DELTA > LOG_TIME_PERIOD)                                       \
  ::roo_logging::LogMessage(__FILE__, __LINE__,                               \
                            ::roo_logging::ROO_LOGGING_##severity)            \
      .stream()

namespace roo_logging {

// A container for a string pointer which can be evaluated to a bool -
// true iff the pointer is NULL.
struct CheckOpString {
  CheckOpString(::String* str) : str_(str) {}
  // No destructor: if str_ is non-NULL, we're about to LOG(FATAL),
  // so there's no point in cleaning up str_.
  operator bool() const {
    return ROO_PREDICT_BRANCH_NOT_TAKEN(str_ != nullptr);
  }
  ::String* str_;
};

}  // namespace roo_logging
