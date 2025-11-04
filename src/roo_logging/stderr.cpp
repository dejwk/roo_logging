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

#include "roo_logging/stderr.h"

#include <stdio.h>

#include "roo_logging/color.h"
#include "roo_logging/config.h"
#include "roo_logging/log_severity.h"

namespace roo_logging {
namespace {

void ColoredWriteToStderr(LogSeverity severity, const char* message,
                          size_t len) {
  bool coloring = GET_ROO_FLAG(roo_logging_colorlogtostderr);
  LogColor color = coloring ? SeverityToColor(severity) : COLOR_DEFAULT;
  // Note: not using ets_printf, because it assumes UART0, which doesn't
  // work well with JTAG debugging which sends stderr over USB.
  if (color == COLOR_DEFAULT) {
    fwrite(message, len, 1, stderr);
    return;
  }
  fwrite("\033[0;3", 5, 1, stderr);
  fwrite(GetAnsiColorCode(color), 1, 1, stderr);
  fwrite("m", 1, 1, stderr);
  fwrite(message, len, 1, stderr);
  fwrite("\033[m", 3, 1, stderr);  // Resets the terminal to default.
}

}  // namespace

// Take a log message of a particular severity and log it to stderr
// iff it's of a high enough severity to deserve it.
void MaybeLogToStderr(LogSeverity severity, const char* message, size_t len) {
  //   if ((severity >= GET_ROO_FLAG(stderrthreshold)) ||
  //   GET_ROO_FLAG(alsologtostderr)) {
  ColoredWriteToStderr(severity, message, len);
}

}  // namespace roo_logging