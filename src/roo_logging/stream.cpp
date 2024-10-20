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

namespace roo_logging {

DefaultLogStream& operator<<(DefaultLogStream& s, const char* val) {
  size_t len = strlen(val);
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val, len);
  s.pos_ += len;
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Uptime uptime) {
  roo_time::Interval::Components c =
      (uptime - roo_time::Uptime::Start()).toComponents();
  s.printf("S%s%06d.%02d:%02d:%02d.%06d", (c.negative ? "-" : "+"), (int)c.days,
           (int)c.hours, (int)c.minutes, (int)c.seconds, (int)c.micros);
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Interval interval) {
  s << interval.toComponents();
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Interval::Components components) {
  bool force = false;
  if (components.negative) s << "-";
  if (components.days > 0) {
    s.printf("%d.", components.days);
    force = true;
  }
  if (force || components.hours > 0) {
    s.printf("%02d:", components.hours);
    force = true;
  }
  if (force || components.minutes > 0) {
    s.printf("%02d:", components.minutes);
    force = true;
  }
  if (force) {
    s.printf("%02d", components.seconds);
  } else {
    s.printf("%d", components.seconds);
  }
  if (components.micros != 0) {
    uint32_t v = components.micros;
    while (v % 10 == 0) v /= 10;
    s.printf(".%d", v);
  }
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::DateTime dt) {
  s.printf("%04d-%02d-%02dT%02d:%02d:%02d.%06d", dt.year(), dt.month(),
           dt.day(), dt.hour(), dt.minute(), dt.second(), dt.micros());
  int tz_minutes = dt.timeZone().offset().inMinutes();
  if (tz_minutes != 0) {
    if (tz_minutes < 0) {
      s.write('-');
      tz_minutes = -tz_minutes;
    } else {
      s.write('+');
    }
    s.printf("%02d:%02d", tz_minutes / 60, tz_minutes % 60);
  }
  return s;
}

}  // namespace roo_logging

roo_logging::DefaultLogStream& operator<<(roo_logging::DefaultLogStream& s,
                                          const String& val) {
  size_t len = val.length();
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val.c_str(), len);
  s.pos_ += len;
  return s;
}
