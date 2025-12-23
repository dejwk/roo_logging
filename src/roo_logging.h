#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include <inttypes.h>

#include <cstring>

#include "roo_logging/base.h"
#include "roo_logging/check.h"
#include "roo_logging/config.h"
#include "roo_logging/exit.h"
#include "roo_logging/log_message.h"
#include "roo_logging/log_severity.h"
#include "roo_logging/predict.h"
#include "roo_logging/stream.h"
#include "roo_time.h"

// Make a bunch of macros for logging.  The way to log things is to stream
// things to LOG(<a particular severity level>).  E.g.,
//
//   LOG(INFO) << "Found " << num_cookies << " cookies";
//
// You can capture log messages in a string, rather than reporting them
// immediately:
//
//   vector<string> errors;
//   LOG_STRING(ERROR, &errors) << "Couldn't parse cookie #" << cookie_num;
//
// This pushes back the new error onto 'errors'; if given a NULL pointer,
// it reports the error via LOG(ERROR).
//
// You can also do conditional logging:
//
//   LOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
// You can also do occasional logging (log every n'th occurrence of an
// event):
//
//   LOG_EVERY_N(INFO, 10) << "Got the " << roo_logging::COUNTER << "th cookie";
//
// The above will cause log messages to be output on the 1st, 11th, 21st, ...
// times it is executed.  Note that the special roo_logging::COUNTER value is
// used to identify which repetition is happening.
//
// You can also do occasional conditional logging (log every n'th
// occurrence of an event, when condition is satisfied):
//
//   LOG_IF_EVERY_N(INFO, (size > 1024), 10) << "Got the " <<
//   roo_logging::COUNTER
//                                           << "th big cookie";
//
// You can log messages the first N times your code executes a line. E.g.
//
//   LOG_FIRST_N(INFO, 20) << "Got the " << roo_logging::COUNTER << "th cookie";
//
// Outputs log messages for the first 20 times it is executed.
//
// There are also "debug mode" logging macros like the ones above:
//
//   DLOG(INFO) << "Found cookies";
//
//   DLOG_IF(INFO, num_cookies > 10) << "Got lots of cookies";
//
//   DLOG_EVERY_N(INFO, 10) << "Got the " << roo_logging::COUNTER << "th
//   cookie";
//
// All "debug mode" logging is compiled away to nothing for non-debug mode
// compiles.
//
// We also have
//
//   LOG_ASSERT(assertion);
//   DLOG_ASSERT(assertion);
//
// which is syntactic sugar for {,D}LOG_IF(FATAL, assert fails) << assertion;
//
// There are "verbose level" logging macros.  They look like
//
//   VLOG(1) << "I'm printed when you run the program with --v=1 or more";
//   VLOG(2) << "I'm printed when you run the program with --v=2 or more";
//
// These always log at the INFO log level (when they log at all).
//
// Additionally, there are per-module logging macros, useful for conditionally
// enabling more detailed (INFO-level) logging for individual libraries:
//
//
//   #ifndef MLOG_my_module
//   #define MLOG_my_module 0
//   #endif
//   // ...
//   MLOG(my_module) << "Only logged if MLOG_my_module is > 0"
//
// There are also VLOG_IS_ON(n) and MLOG_IS_ON(my_module) "verbose level"
// condition macro. To be used as
//
//   if (VLOG_IS_ON(2)) {
//     // do some logging preparation and logging
//     // that can't be accomplished with just VLOG(2) << ...;
//   }
//
// There are also VLOG_IF, VLOG_EVERY_N and VLOG_IF_EVERY_N "verbose level"
// condition macros, as well as the corresponding MLOG_IF, MLOG_EVERY_N, and
// MLOG_IF_EVERY_N for per-module logging, for simple cases when some extra
// computation and preparation for logs is not needed.
//   VLOG_IF(1, (size > 1024))
//      << "I'm printed when size is more than 1024 and when you run the "
//         "program with --v=1 or more";
//   VLOG_EVERY_N(1, 10)
//      << "I'm printed every 10th occurrence, and when you run the program "
//         "with --v=1 or more. Present occurence is " << roo_logging::COUNTER;
//   VLOG_IF_EVERY_N(1, (size > 1024), 10)
//      << "I'm printed on every 10th occurence of case when size is more "
//         " than 1024, when you run the program with --v=1 or more. ";
//         "Present occurence is " << roo_logging::COUNTER;
//
// The supported severity levels for macros that allow you to specify one
// are (in increasing order of severity) INFO, WARNING, ERROR, and FATAL.
// Note that messages of a given severity are logged not only in the
// logfile for that severity, but also in all logfiles of lower severity.
// E.g., a message of severity FATAL will be logged to the logfiles of
// severity FATAL, ERROR, WARNING, and INFO.
//
// There is also the special severity of DFATAL, which logs FATAL in
// debug mode, ERROR in normal mode.
//
// Very important: logging a message at the FATAL severity level causes
// the program to terminate (after the message is logged).
//

// We use the preprocessor's merging operator, "##", so that, e.g.,
// LOG(INFO) becomes the token ROO_LOG_INFO.  There's some funny
// subtle difference between ostream member streaming functions (e.g.,
// ostream::operator<<(int) and ostream non-member streaming functions
// (e.g., ::operator<<(ostream&, string&): it turns out that it's
// impossible to stream something like a string directly to an unnamed
// ostream. We employ a neat hack by calling the stream() member
// function of LogMessage which seems to avoid the problem.
#define LOG(severity) COMPACT_ROO_LOG_##severity.stream()

#define LOG_IF(severity, condition) \
  !(condition) ? (void)0 : ::roo_logging::LogMessageVoidify() & LOG(severity)

#define LOG_ASSERT(condition) \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition

// CHECK dies with a fatal error if condition is not true.  It is *not*
// controlled by DCHECK_IS_ON(), so the check will be executed regardless of
// compilation mode.  Therefore, it is safe to do things like:
//    CHECK(fp->Write(x) == 4)
#define CHECK(condition)                                    \
  LOG_IF(FATAL, ROO_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
      << "Check failed: " #condition " "

// Equality/Inequality checks - compare two values, and log a FATAL message
// including the two values when the result is not as expected.  The values
// must have operator<<(ostream, ...) defined.
//
// You may append to the error message like so:
//   CHECK_NE(1, 2) << ": The world must be ending!";
//
// We are very careful to ensure that each argument is evaluated exactly
// once, and that anything which is legal to pass as a function argument is
// legal here.  In particular, the arguments may be temporary expressions
// which will end up being destroyed at the end of the apparent statement,
// for example:
//   CHECK_EQ(string("abc")[1], 'b');
//
// WARNING: These don't compile correctly if one of the arguments is a pointer
// and the other is NULL. To work around this, simply static_cast NULL to the
// type of the desired pointer.

#define CHECK_EQ(val1, val2) CHECK_OP(_EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(_NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(_LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(_LT, <, val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(_GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(_GT, >, val1, val2)

// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val)                                                     \
  roo_logging::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", \
                            (val))

// String (char*) equality/inequality checks.
// CASE versions are case-insensitive.
//
// Note that "s1" and "s2" may be temporary strings which are destroyed
// by the compiler at the end of the current "full expression"
// (e.g. CHECK_STREQ(Foo().c_str(), Bar().c_str())).

#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define CHECK_INDEX(I, A) CHECK(I < (sizeof(A) / sizeof(A[0])))
#define CHECK_BOUND(B, A) CHECK(B <= (sizeof(A) / sizeof(A[0])))

#define CHECK_DOUBLE_EQ(val1, val2)                \
  do {                                             \
    CHECK_LE((val1), (val2) + 0.000000000000001L); \
    CHECK_GE((val1), (val2) - 0.000000000000001L); \
  } while (0)

#define CHECK_NEAR(val1, val2, margin)   \
  do {                                   \
    CHECK_LE((val1), (val2) + (margin)); \
    CHECK_GE((val1), (val2) - (margin)); \
  } while (0)

// PLOG() and PLOG_IF() and PCHECK() behave exactly like their LOG* and
// CHECK equivalents with the addition that they postpend a description
// of the current state of errno to their output lines.

#define PLOG(severity) ROO_LOGGING_PLOG(severity, 0).stream()

#define PLOG_IF(severity, condition) \
  !(condition) ? (void)0 : ::roo_logging::LogMessageVoidify() & PLOG(severity)

// A CHECK() macro that postpends errno if the condition is false. E.g.
//
// if (poll(fds, nfds, timeout) == -1) { PCHECK(errno == EINTR); ... }
#define PCHECK(condition)                                            \
  PLOG_IF(FATAL, ROO_LOGGING_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
      << "Check failed: " #condition " "

#define LOG_EVERY_N(severity, n) \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), ::roo_logging::LogMessage::SendToLog)

#define LOG_EVERY_T(severity, T) SOME_KIND_OF_LOG_EVERY_T(severity, (T))

#define LOG_FIRST_N(severity, n) \
  SOME_KIND_OF_LOG_FIRST_N(severity, (n), ::roo_logging::LogMessage::SendToLog)

#define LOG_IF_EVERY_N(severity, condition, n)            \
  SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n), \
                              ::roo_logging::LogMessage::SendToLog)

// Plus some debug-logging macros that get compiled to nothing for production

#if DCHECK_IS_ON()

#define DLOG(severity) LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) \
  LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)

// debug-only checking.  executed if DCHECK_IS_ON().
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)

#else  // !DCHECK_IS_ON()

#define DLOG(severity) \
  true ? (void)0 : ::roo_logging::LogMessageVoidify() & LOG(severity)

#define DVLOG(verboselevel)           \
  (true || !VLOG_IS_ON(verboselevel)) \
      ? (void)0                       \
      : ::roo_logging::LogMessageVoidify() & LOG(INFO)

#define DLOG_IF(severity, condition) \
  (true || !(condition)) ? (void)0   \
                         : ::roo_logging::LogMessageVoidify() & LOG(severity)

#define DLOG_EVERY_N(severity, n) \
  true ? (void)0 : ::roo_logging::LogMessageVoidify() & LOG(severity)

#define DLOG_IF_EVERY_N(severity, condition, n) \
  (true || !(condition)) ? (void)0              \
                         : ::roo_logging::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) true ? (void)0 : LOG_ASSERT(condition)

// MSVC warning C4127: conditional expression is constant
#define DCHECK(condition) \
  while (false) CHECK(condition)

#define DCHECK_EQ(val1, val2) \
  while (false) CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
  while (false) CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
  while (false) CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
  while (false) CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
  while (false) CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
  while (false) CHECK_GT(val1, val2)

// You may see warnings in release mode if you don't use the return
// value of DCHECK_NOTNULL. Please just use DCHECK for such cases.
#define DCHECK_NOTNULL(val) (val)

#define DCHECK_STREQ(str1, str2) \
  while (false) CHECK_STREQ(str1, str2)

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false) CHECK_STRCASEEQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
  while (false) CHECK_STRNE(str1, str2)

#define DCHECK_STRCASENE(str1, str2) \
  while (false) CHECK_STRCASENE(str1, str2)

#endif  // DCHECK_IS_ON()

// Log only in verbose mode.

#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))

#define VLOG_IF(verboselevel, condition) \
  LOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#define VLOG_EVERY_N(verboselevel, n) \
  LOG_IF_EVERY_N(INFO, VLOG_IS_ON(verboselevel), n)

#define VLOG_IF_EVERY_N(verboselevel, condition, n) \
  LOG_IF_EVERY_N(INFO, (condition) && VLOG_IS_ON(verboselevel), n)

// Verbose logging per-module (e.g. switch on/off for individual libraries).

#define __ROO_LOGGING_CAT(a, b) a##b
#define ROO_LOGGING_CAT(a, b) __ROO_LOGGING_CAT(a, b)

#define MLOG_IS_ON(module) (ROO_LOGGING_CAT(MLOG_, module) > 0)
#define MLOG(module) LOG_IF(INFO, MLOG_IS_ON(module))

#define MLOG_IF(module, condition) \
  LOG_IF(INFO, (condition) && MLOG_IS_ON(module))

#define MLOG_EVERY_N(module, n) LOG_IF_EVERY_N(INFO, MLOG_IS_ON(module), n)

#define MLOG_IF_EVERY_N(module, condition, n) \
  LOG_IF_EVERY_N(INFO, (condition) && MLOG_IS_ON(module), n)

namespace roo_logging {

// A macro alternative of LogAtLevel. New code may want to use this
// version since there are two advantages: 1. this version outputs the
// file name and the line number where this macro is put like other
// LOG macros, 2. this macro can be used as C++ stream.
#define LOG_AT_LEVEL(severity) \
  roo_logging::LogMessage(__FILE__, __LINE__, severity).stream()

// In C++11, all cases can be handled by a single function. Since the value
// category of the argument is preserved (also for rvalue references),
// member initializer lists like the one below will compile correctly:
//
//   Foo()
//     : x_(CHECK_NOTNULL(MethodReturningUniquePtr())) {}
template <typename T>
T CheckNotNull(const char* file, int line, const char* names, T&& t) {
  if (t == nullptr) {
    LogMessageFatal(file, line, new String(names));
  }
  return std::forward<T>(t);
}

#if defined(ARDUINO)

// A non-macro interface to the log facility; (useful
// when the logging level is not a compile-time constant).
inline void LogAtLevel(int const severity, const ::String& msg) {
  LogMessage(__FILE__, __LINE__, severity).stream() << msg;
}

#endif

}  // namespace roo_logging
