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

#include "Print.h"
#include "roo_logging/config.h"
#include "roo_logging/predict.h"
#include "roo_time.h"

namespace roo_logging {

// We want the special COUNTER value available for LOG_EVERY_X()'ed messages
enum PRIVATE_Counter { COUNTER };

// An arbitrary limit on the length of a single log message. This
// is so that streaming can be done more efficiently.
static constexpr size_t kMaxLogMessageLen = 1024;

class DefaultLogStream : public Print {
 public:
  DefaultLogStream(char* buf, size_t cap)
      : buf_(buf), pos_(0), cap_(cap), number_base_(10), ctr_(0) {}
  size_t remaining_capacity() const { return cap_ - pos_ - 1; }
  bool full() const { return remaining_capacity() == 0; }

  void setBase(int base) {
    if (base < 2) base = 10;
    number_base_ = base;
  }

  bool write(char b) {
    if (full()) return false;
    buf_[pos_++] = b;
    return true;
  }

  size_t write(uint8_t b) override { return write((char)b) ? 1 : 0; }

  size_t write(const uint8_t* buffer, size_t size) override {
    return write((const char*)buffer, size);
  }

  size_t write(const char* buf, size_t len) {
    size_t cap = remaining_capacity();
    if (len > cap) len = cap;
    memcpy(&buf_[pos_], buf, len);
    pos_ += len;
    return len;
  }

  int number_base() const { return number_base_; }

  int pcount() const { return pos_; }

  int ctr() const { return ctr_; }
  void set_ctr(int ctr) { ctr_ = ctr; }

  char* buf_;
  size_t pos_;
  size_t cap_;
  int number_base_;
  int ctr_;
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

inline DefaultLogStream& operator<<(DefaultLogStream& s,
                                    unsigned long long val) {
  s.print(val, s.number_base());
  return s;
}

inline DefaultLogStream& operator<<(DefaultLogStream& s, const Printable& val) {
  s.print(val);
  return s;
}

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::Uptime uptime);

DefaultLogStream& operator<<(DefaultLogStream& s, roo_time::DateTime dt);

// Output the COUNTER value.
inline DefaultLogStream& operator<<(DefaultLogStream& os,
                                    const PRIVATE_Counter&) {
  os << os.ctr();
  return os;
}

inline DefaultLogStream& operator<<(DefaultLogStream& os, const void* ptr) {
  os.printf("%p", ptr);
  return os;
}

class OStringStream : public DefaultLogStream {
 public:
  OStringStream() : DefaultLogStream(val_, kMaxLogMessageLen) {}

  String* newString() { return new String(val_, pcount()); }

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

template <typename Str, typename CStr = decltype(std::declval<Str>().c_str()),
          typename Size = decltype(std::declval<Str>().size())>
DefaultLogStream& operator<<(DefaultLogStream& s, const Str& val) = delete;

}  // namespace roo_logging

// Captures anything that resembles a string.
template <typename Str, const char*, size_t>
roo_logging::DefaultLogStream& operator<<(roo_logging::DefaultLogStream& s,
                                          const Str& val) {
  size_t len = val.size();
  size_t cap = s.remaining_capacity();
  if (len > cap) len = cap;
  memcpy(&s.buf_[s.pos_], val.c_str(), len);
  s.pos_ += len;
  return s;
}

roo_logging::DefaultLogStream& operator<<(roo_logging::DefaultLogStream& s,
                                          const String& val);
