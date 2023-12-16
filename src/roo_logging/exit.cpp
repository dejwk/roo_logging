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

#include "roo_logging/exit.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "roo_logging/stacktrace.h"
#include "roo_logging/symbolize.h"

namespace roo_logging {

typedef void (*logging_fail_func_t)() __attribute__((noreturn));

static void DebugWriteToStderr(const char* data, void*) {
  if (fwrite(data, strlen(data), 1, stderr) < 0) {
    // Ignore errors.
  }
}

void DumpStackTraceAndExit() {
  DumpStackTrace(1, DebugWriteToStderr, nullptr);
  abort();
}

logging_fail_func_t _logging_fail_func =
    reinterpret_cast<logging_fail_func_t>(&DumpStackTraceAndExit);

logging_fail_func_t roo_logging_fail_func() { return _logging_fail_func; }

void InstallFailureFunction(logging_fail_func_t fail_func) {
  _logging_fail_func = fail_func;
}

void Fail() { roo_logging_fail_func()(); }

}  // namespace roo_logging
