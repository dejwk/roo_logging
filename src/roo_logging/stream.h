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

#include <utility>

#include "roo_backport.h"
#include "roo_backport/string_view.h"
#include "roo_logging/config.h"
#include "roo_logging/predict.h"
#include "roo_time.h"

#if defined(ARDUINO)
#include "roo_logging/stream_arduino.h"

using StreamBase = roo_logging::ArduinoLogStream;

#elif defined(ESP_PLATFORM)
#include "roo_logging/stream_espidf.h"

using StreamBase = roo_logging::EspidfLogStream;

#endif

namespace roo_logging {

// We want the special COUNTER value available for LOG_EVERY_X()'ed messages
enum PRIVATE_Counter { COUNTER };

// An arbitrary limit on the length of a single log message. This
// is so that streaming can be done more efficiently.
static constexpr size_t kMaxLogMessageLen = 1024;

class DefaultLogStream : public StreamBase {
 public:
  DefaultLogStream(char* buf, size_t cap) : StreamBase(buf, cap) {}
};

DefaultLogStream& operator<<(DefaultLogStream& s, const char* val);

inline DefaultLogStream& operator<<(DefaultLogStream& s, char val) {
  s.write(val);
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, unsigned char val) {
  s.write(val);
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, short val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, int val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, long val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, long long val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, unsigned short val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, unsigned int val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, unsigned long val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, float val) {
  s.printf("%g", val);
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, double val) {
  s.printf("%g", val);
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s,
                                    unsigned long long val) {
  s.print(val, s.number_base());
  return s;
}

#if defined(ARDUINO)

inline DefaultLogStream& operator<<(DefaultLogStream& s,
                                    const ::Printable& val) {
  s.print(val);
  return s;
}

#endif

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Uptime uptime);

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Duration interval);

DefaultLogStream& operator<<(DefaultLogStream& s,
                             roo_time::Duration::Components interval);

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::DateTime dt);

// Output the COUNTER value.
inline DefaultLogStream& operator<<(DefaultLogStream& os,
                                    const PRIVATE_Counter&) {
  os << (int)os.ctr();
  return os;
}

inline DefaultLogStream& operator<<(DefaultLogStream& os, const void* ptr) {
  os.printf("%p", ptr);
  return os;
}

inline DefaultLogStream& dec(DefaultLogStream& stream) {
  stream.setBase(10);
  return stream;
}

inline DefaultLogStream& hex(DefaultLogStream& stream) {
  stream.setBase(16);
  return stream;
}

inline DefaultLogStream& oct(DefaultLogStream& stream) {
  stream.setBase(8);
  return stream;
}

inline DefaultLogStream& bin(DefaultLogStream& stream) {
  stream.setBase(2);
  return stream;
}

inline DefaultLogStream& operator<<(
    DefaultLogStream& s, DefaultLogStream& (*fn)(DefaultLogStream& stream)) {
  return fn(s);
}

class OStringStream : public DefaultLogStream {
 public:
  OStringStream() : DefaultLogStream(val_, kMaxLogMessageLen) {}

  StringType* newString() { return new StringType(val_, pcount()); }

 private:
  char val_[kMaxLogMessageLen];
};

using Stream = DefaultLogStream;

// A class for which we define operator<<, which does nothing.
class NullStream : public Stream {
 public:
  // Initialize the Stream so the messages can be written somewhere
  // (they'll never be actually displayed). This will be needed if a
  // NullStream& is implicitly converted to Stream&, in which case
  // the overloaded NullStream::operator<< will not be invoked.
  NullStream() : Stream(message_buffer_, 1) {}
  // NullStream(const char* /*file*/, int /*line*/,
  //            const CheckOpString& /*result*/)
  //     : Stream(message_buffer_, 1) {}
  NullStream& stream() { return *this; }

 private:
  // A very short buffer for messages (which we discard anyway). This
  // will be needed if NullStream& converted to LogStream& (e.g. as a
  // result of a conditional expression).
  char message_buffer_[2];
};

// Do nothing. This operator is inline, allowing the message to be
// compiled away. The message will not be compiled away if we do
// something like (flag ? LOG(INFO) : LOG(ERROR)) << message; when
// SKIP_LOG=WARNING. In those cases, NullStream will be implicitly
// converted to LogStream and the message will be computed and then
// quietly discarded.
template <class T>
inline NullStream& operator<<(NullStream& str, const T&) {
  return str;
}

// Similar to NullStream, but aborts the program (without stack
// trace), like LogMessageFatal.
class NullStreamFatal : public NullStream {
 public:
  NullStreamFatal() {}
  // NullStreamFatal(const char* file, int line, const CheckOpString& result)
  //     : NullStream(file, line, result) {}
  __attribute__((noreturn)) ~NullStreamFatal() throw() { exit(1); }
};

#if defined(ARDUINO)

roo_logging::DefaultLogStream& operator<<(roo_logging::DefaultLogStream& s,
                                          const ::String& val);

#endif

// Logging for iterable collections.
template <typename C, typename ItrB = decltype(std::declval<C>().begin()),
          typename ItrE = decltype(std::declval<C>().end())>
inline roo_logging::DefaultLogStream& operator<<(
    roo_logging::DefaultLogStream& s, const C& c) {
  s << "[";
  bool first = true;
  auto i = c.begin();
  while (i != c.end()) {
    if (!first) {
      s << ", ";
    }
    first = false;
    s << *i++;
  }
  s << "]";
  return s;
}

inline roo_logging::DefaultLogStream& operator<<(
    roo_logging::DefaultLogStream& s, roo::string_view val) {
  size_t len = val.size();
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val.data(), len);
  s.pos_ += len;
  return s;
}

}  // namespace roo_logging

#if defined(ESP_PLATFORM) || defined(__linux__)
#include <string>

namespace roo_logging {

inline roo_logging::DefaultLogStream& operator<<(
    roo_logging::DefaultLogStream& s, const std::string& val) {
  size_t len = val.size();
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val.data(), len);
  s.pos_ += len;
  return s;
}

}  // namespace roo_logging

#endif

// #if __cplusplus >= 201703L
// #include <string_view>

// #endif
