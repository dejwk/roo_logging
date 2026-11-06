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

#if defined(ESP_PLATFORM) && !defined(__linux__)
#include "esp_idf_version.h"
#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#endif

namespace roo_logging {
namespace {

// Writes already formatted bytes, including before stdio is initialized.
void WriteToStderr(const char* message, size_t len,
                   bool from_static_initializer) {
#if defined(ESP_PLATFORM) && !defined(__linux__)
  if (from_static_initializer) {
    for (size_t i = 0; i < len; ++i) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
      esp_rom_output_to_channels(message[i]);
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
      esp_rom_output_putc(message[i]);
#else
      esp_rom_uart_putc(message[i]);
#endif
    }
    return;
  }
#endif
  // Use stdio after initialization so USB/JTAG stderr routing is respected.
  fwrite(message, len, 1, stderr);
}

void ColoredWriteToStderr(LogSeverity severity, const char* message, size_t len,
                          bool from_static_initializer) {
  bool coloring = GET_ROO_FLAG(roo_logging_colorlogtostderr);
  LogColor color = coloring ? SeverityToColor(severity) : COLOR_DEFAULT;
  if (color != COLOR_DEFAULT) {
    WriteToStderr("\033[0;3", 5, from_static_initializer);
    WriteToStderr(GetAnsiColorCode(color), 1, from_static_initializer);
    WriteToStderr("m", 1, from_static_initializer);
  }
  WriteToStderr(message, len, from_static_initializer);
  if (color != COLOR_DEFAULT) {
    WriteToStderr("\033[m", 3, from_static_initializer);
  }
}

}  // namespace

// Take a log message of a particular severity and log it to stderr
// iff it's of a high enough severity to deserve it.
void MaybeLogToStderr(LogSeverity severity, const char* message, size_t len,
                      bool from_static_initializer) {
  //   if ((severity >= GET_ROO_FLAG(stderrthreshold)) ||
  //   GET_ROO_FLAG(alsologtostderr)) {
  ColoredWriteToStderr(severity, message, len, from_static_initializer);
}

}  // namespace roo_logging