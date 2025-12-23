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

#include "roo_logging/stream.h"

namespace roo_logging {

// A helper class for formatting "expr (V1 vs. V2)" in a CHECK_XX
// statement.  See MakeCheckOpString for sample usage.  Other
// approaches were considered: use of a template method (e.g.,
// base::BuildCheckOpString(exprtext, base::Print<T1>, &v1,
// base::Print<T2>, &v2), however this approach has complications
// related to volatile arguments and function-pointer arguments).
class CheckOpMessageBuilder {
 public:
  // Inserts "exprtext" and " (" to the stream.
  explicit CheckOpMessageBuilder(const char* exprtext);
  // Deletes "stream_".
  ~CheckOpMessageBuilder();
  // For inserting the first variable.
  Stream* ForVar1() { return stream_; }
  // For inserting the second variable (adds an intermediate " vs. ").
  Stream* ForVar2();
  // Get the result (inserts the closing ")").
  StringType* NewString();

 private:
  OStringStream* stream_;
};

// Function is overloaded for integral types to allow static const
// integrals declared in classes and not defined to be used as arguments to
// CHECK* macros. It's not encouraged though.
template <class T>
inline const T& GetReferenceableValue(const T& t) {
  return t;
}
inline char GetReferenceableValue(char t) { return t; }
inline unsigned char GetReferenceableValue(unsigned char t) { return t; }
inline signed char GetReferenceableValue(signed char t) { return t; }
inline short GetReferenceableValue(short t) { return t; }
inline unsigned short GetReferenceableValue(unsigned short t) { return t; }
inline int GetReferenceableValue(int t) { return t; }
inline unsigned int GetReferenceableValue(unsigned int t) { return t; }
inline long GetReferenceableValue(long t) { return t; }
inline unsigned long GetReferenceableValue(unsigned long t) { return t; }
inline long long GetReferenceableValue(long long t) { return t; }
inline unsigned long long GetReferenceableValue(unsigned long long t) {
  return t;
}

// This formats a value for a failing CHECK_XX statement.  Ordinarily,
// it uses the definition for operator<<, with a few special cases below.
template <typename T>
inline void MakeCheckOpValueString(Stream* os, const T& v) {
  (*os) << v;
}

// Overrides for char types provide readable values for unprintable
// characters.
template <>
void MakeCheckOpValueString(Stream* os, const char& v);
template <>
void MakeCheckOpValueString(Stream* os, const signed char& v);
template <>
void MakeCheckOpValueString(Stream* os, const unsigned char& v);

template <typename T1, typename T2>
StringType* MakeCheckOpString(const T1& v1, const T2& v2,
                              const char* exprtext) {
  CheckOpMessageBuilder comb(exprtext);
  MakeCheckOpValueString(comb.ForVar1(), v1);
  MakeCheckOpValueString(comb.ForVar2(), v2);
  return comb.NewString();
}

// Helper functions for CHECK_OP macro.
// The (int, int) specialization works around the issue that the compiler
// will not instantiate the template version of the function on values of
// unnamed enum type - see comment below.
#define DEFINE_CHECK_OP_IMPL(name, op)                                  \
  template <typename T1, typename T2>                                   \
  inline StringType* name##Impl(const T1& v1, const T2& v2,             \
                                const char* exprtext) {                 \
    if (ROO_PREDICT_TRUE(v1 op v2))                                     \
      return nullptr;                                                   \
    else                                                                \
      return ::roo_logging::MakeCheckOpString(v1, v2, exprtext);        \
  }                                                                     \
  inline StringType* name##Impl(int v1, int v2, const char* exprtext) { \
    return name##Impl<int, int>(v1, v2, exprtext);                      \
  }

// We use the full name Check_EQ, Check_NE, etc. in case the file including
// base/logging.h provides its own #defines for the simpler names EQ, NE, etc.
// This happens if, for example, those are used as token names in a
// yacc grammar.
DEFINE_CHECK_OP_IMPL(Check_EQ, ==)  // Compilation error with CHECK_EQ(NULL, x)?
DEFINE_CHECK_OP_IMPL(Check_NE, !=)  // Use CHECK(x == NULL) instead.
DEFINE_CHECK_OP_IMPL(Check_LE, <=)
DEFINE_CHECK_OP_IMPL(Check_LT, <)
DEFINE_CHECK_OP_IMPL(Check_GE, >=)
DEFINE_CHECK_OP_IMPL(Check_GT, >)
#undef DEFINE_CHECK_OP_IMPL

// Helper macro for binary operators.
// Don't use this macro directly in your code, use CHECK_EQ et al below.

#if defined(STATIC_ANALYSIS)
// Only for static analysis tool to know that it is equivalent to assert
#define CHECK_OP_LOG(name, op, val1, val2, log) CHECK((val1)op(val2))
#elif DCHECK_IS_ON()
// In debug mode, avoid constructing CheckOpStrings if possible,
// to reduce the overhead of CHECK statments by 2x.
// Real DCHECK-heavy tests have seen 1.5x speedups.

// The meaning of "string" might be different between now and
// when this macro gets invoked (e.g., if someone is experimenting
// with other string implementations that get defined after this
// file is included).  Save the current meaning now and use it
// in the macro.
typedef StringType _Check_string;

#define CHECK_OP_LOG(name, op, val1, val2, log)              \
  while (::roo_logging::_Check_string* _result =             \
             roo_logging::Check##name##Impl(                 \
                 ::roo_logging::GetReferenceableValue(val1), \
                 ::roo_logging::GetReferenceableValue(val2), \
                 #val1 " " #op " " #val2))                   \
  log(__FILE__, __LINE__, roo_logging::CheckOpString(_result)).stream()
#else
// In optimized mode, use CheckOpString to hint to compiler that
// the while condition is unlikely.
#define CHECK_OP_LOG(name, op, val1, val2, log)                               \
  while (roo_logging::CheckOpString _result = roo_logging::Check##name##Impl( \
             ::roo_logging::GetReferenceableValue(val1),                      \
             ::roo_logging::GetReferenceableValue(val2),                      \
             #val1 " " #op " " #val2))                                        \
  log(__FILE__, __LINE__, _result).stream()
#endif  // STATIC_ANALYSIS, DCHECK_IS_ON()

#if ROO_STRIP_LOG <= 3
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, ::roo_logging::LogMessageFatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, ::roo_logging::NullStreamFatal)
#endif  // STRIP_LOG <= 3

// Helper functions for string comparisons.
// To avoid bloat, the definitions are in logging.cc.
#define DECLARE_CHECK_STROP_IMPL(func, expected)                          \
  StringType* Check##func##expected##Impl(const char* s1, const char* s2, \
                                          const char* names);
DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(strcasecmp, true)
DECLARE_CHECK_STROP_IMPL(strcasecmp, false)
#undef DECLARE_CHECK_STROP_IMPL

// Helper macro for string comparisons.
// Don't use this macro directly in your code, use CHECK_STREQ et al below.
#define CHECK_STROP(func, op, expected, s1, s2)                               \
  while (::roo_logging::CheckOpString _result =                               \
             ::roo_logging::Check##func##expected##Impl((s1), (s2),           \
                                                        #s1 " " #op " " #s2)) \
  LOG(FATAL) << *_result.str_

}  // namespace roo_logging
