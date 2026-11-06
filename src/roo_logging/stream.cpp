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

#include "roo_logging/stream.h"

#include <cstdint>

#include "roo_time/format.h"

namespace roo_logging {
namespace {

// Appends decimal fields independently of the stream's selected integer base.
void AppendPaddedDecimal(DefaultLogStream& stream, uint32_t value,
                         size_t width) {
  char digits[10];
  char* end = digits + sizeof(digits);
  char* current = end;
  do {
    *--current = '0' + value % 10;
    value /= 10;
  } while (value != 0);
  const size_t count = end - current;
  while (width > count) {
    stream.write('0');
    --width;
  }
  stream.write(current, count);
}

}  // namespace

bool DefaultLogStream::formatDuration(roo_time::Duration duration,
                                      const char* format) {
  const size_t available = remaining_capacity();
  const roo_time::FormatResult result =
      roo_time::FormatDuration(duration, format, buf_ + pos_, available + 1);
  if (result.status != roo_time::TextStatus::kOk &&
      result.status != roo_time::TextStatus::kBufferTooSmall) {
    return false;
  }
  pos_ += result.size < available ? result.size : available;
  return true;
}

bool DefaultLogStream::formatUptime(roo_time::Uptime uptime,
                                    const char* format) {
  const size_t available = remaining_capacity();
  const roo_time::FormatResult result =
      roo_time::FormatUptime(uptime, format, buf_ + pos_, available + 1);
  if (result.status != roo_time::TextStatus::kOk &&
      result.status != roo_time::TextStatus::kBufferTooSmall) {
    return false;
  }
  pos_ += result.size < available ? result.size : available;
  return true;
}

DefaultLogStream& operator<<(DefaultLogStream& s, const char* val) {
  size_t len = strlen(val);
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val, len);
  s.pos_ += len;
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Uptime uptime) {
  s.formatUptime(uptime, roo_time::kUptimeClockMicrosFormat);
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Duration interval) {
  s.formatDuration(interval, roo_time::kDurationClockFormat);
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s,
                             roo_time::Duration::Components components) {
  return s << roo_time::Duration::FromComponents(components);
}

DefaultLogStream& operator<<(DefaultLogStream& s, const void* ptr) {
  uintptr_t value = reinterpret_cast<uintptr_t>(ptr);
  char digits[2 * sizeof(value)];
  char* end = digits + sizeof(digits);
  char* current = end;
  do {
    *--current = "0123456789abcdef"[value & 0xf];
    value >>= 4;
  } while (value != 0);
  s << "0x";
  s.write(current, end - current);
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::DateTime dt) {
  AppendPaddedDecimal(s, dt.year(), 4);
  s.write('-');
  AppendPaddedDecimal(s, dt.month(), 2);
  s.write('-');
  AppendPaddedDecimal(s, dt.day(), 2);
  s.write('T');
  AppendPaddedDecimal(s, dt.hour(), 2);
  s.write(':');
  AppendPaddedDecimal(s, dt.minute(), 2);
  s.write(':');
  AppendPaddedDecimal(s, dt.second(), 2);
  s.write('.');
  AppendPaddedDecimal(s, dt.micros(), 6);
  int tz_minutes = dt.utcOffset().inMinutes();
  if (tz_minutes != 0) {
    if (tz_minutes < 0) {
      s.write('-');
      tz_minutes = -tz_minutes;
    } else {
      s.write('+');
    }
    AppendPaddedDecimal(s, tz_minutes / 60, 2);
    s.write(':');
    AppendPaddedDecimal(s, tz_minutes % 60, 2);
  }
  return s;
}

#if defined(ARDUINO)

roo_logging::DefaultLogStream& operator<<(roo_logging::DefaultLogStream& s,
                                          const ::String& val) {
  size_t len = val.length();
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val.c_str(), len);
  s.pos_ += len;
  return s;
}

#endif

}  // namespace roo_logging
