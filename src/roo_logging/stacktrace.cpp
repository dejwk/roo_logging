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

#include "roo_logging/stacktrace.h"

#include <cstring>

#include "roo_logging/symbolize.h"

#if defined(__linux__)

#include <execinfo.h>

namespace roo_logging {

// int GetStackTrace(void** result, int max_depth, int skip_count) { return 0; }

int GetStackTrace(void** result, int max_depth, int skip_count) {
  static const int kStackLength = 64;
  void* stack[kStackLength];
  int size;

  size = backtrace(stack, kStackLength);
  skip_count++;  // we want to skip the current frame as well
  int result_count = size - skip_count;
  if (result_count < 0) result_count = 0;
  if (result_count > max_depth) result_count = max_depth;
  for (int i = 0; i < result_count; i++) result[i] = stack[i + skip_count];

  return result_count;
}

namespace {

// The %p field width for printf() functions is two characters per byte.
// For some environments, add two extra bytes for the leading "0x".
static const int kPrintfPointerFieldWidth = 2 + 2 * sizeof(void*);

void DumpPC(DebugWriter* writerfn, void* arg, void* pc,
            const char* const prefix) {
  char buf[100];
  snprintf(buf, sizeof(buf), "%s@ %*p\n", prefix, kPrintfPointerFieldWidth, pc);
  writerfn(buf, arg);
}

#ifdef ROO_LOGGING_HAVE_SYMBOLIZE
// Print a program counter and its symbol name.
static void DumpPCAndSymbol(DebugWriter* writerfn, void* arg, void* pc,
                            const char* const prefix) {
  char tmp[1024];
  const char* symbol = "(unknown)";
  // Symbolizes the previous address of pc because pc may be in the
  // next function.  The overrun happens when the function ends with
  // a call to a function annotated noreturn (e.g. CHECK).
  if (Symbolize(reinterpret_cast<char*>(pc) - 1, tmp, sizeof(tmp))) {
    symbol = tmp;
  }
  char buf[1024];
  snprintf(buf, sizeof(buf), "%s@ %*p  %s\n", prefix, kPrintfPointerFieldWidth,
           pc, symbol);
  writerfn(buf, arg);
}
#endif

}  // namespace

// Dump current stack trace as directed by writerfn.
void DumpStackTrace(int skip_count, DebugWriter* writerfn, void* arg) {
  // Print stack trace
  void* stack[32];
  int depth =
      GetStackTrace(stack, sizeof(stack) / sizeof(void*), skip_count + 1);
  for (int i = 0; i < depth; i++) {
#if defined(ROO_LOGGING_HAVE_SYMBOLIZE)
    if (ROO_LOGGING_SYMBOLIZE_STACKTRACE) {
      DumpPCAndSymbol(writerfn, arg, stack[i], "    ");
    } else {
      DumpPC(writerfn, arg, stack[i], "    ");
    }
#else
    DumpPC(writerfn, arg, stack[i], "    ");
#endif
  }
}

}  // namespace roo_logging

#elif defined(ESP32)

#include "esp_attr.h"
#include "esp_debug_helpers.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_types.h"
#include "sdkconfig.h"
#include "soc/cpu.h"
#include "soc/soc_memory_layout.h"
#include "xtensa/xtensa_context.h"

namespace roo_logging {

static void IRAM_ATTR print_entry(uint32_t pc, uint32_t sp) {
  esp_rom_printf("0x%08X:0x%08X", pc, sp);
}

void DumpStackTrace(int skip_count, DebugWriter* writerfn, void* arg) {
  esp_backtrace_frame_t start = {0};
  esp_backtrace_get_start(&(start.pc), &(start.sp), &(start.next_pc));
  int depth = -1;

  esp_backtrace_frame_t stk_frame = {0};
  auto frame = &start;
  memcpy(&stk_frame, frame, sizeof(esp_backtrace_frame_t));

  print_entry(esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp);
  esp_rom_printf("\n");

  // Check if first frame is valid
  bool corrupted =
      !(esp_stack_ptr_is_sane(stk_frame.sp) &&
        (esp_ptr_executable((void*)esp_cpu_process_stack_pc(stk_frame.pc)) ||
         /* Ignore the first corrupted PC in case of
         InstrFetchProhibited */
         (stk_frame.exc_frame && ((XtExcFrame*)stk_frame.exc_frame)->exccause ==
                                     EXCCAUSE_INSTR_PROHIBITED)));

  uint32_t i = (depth <= 0) ? INT32_MAX : depth;
  while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted) {
    if (!esp_backtrace_get_next_frame(&stk_frame)) {  // Get previous
      corrupted = true;
    }
    print_entry(esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp);
    esp_rom_printf("\n");
  }

  // Print backtrace termination marker
  esp_err_t ret = ESP_OK;
  if (corrupted) {
    esp_rom_printf(" |<-CORRUPTED");
    ret = ESP_FAIL;
  } else if (stk_frame.next_pc != 0) {  // Backtrace continues
    esp_rom_printf(" |<-CONTINUES");
  }
  esp_rom_printf("\r\n\r\n");
}

//   esp_backtrace_print(200);

}  // namespace roo_logging

#else

namespace roo_logging {
void DumpStackTrace(int skip_count, DebugWriter* writerfn, void* arg) {}
}  // namespace roo_logging
#endif
