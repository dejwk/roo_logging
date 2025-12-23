#pragma once

#if defined(ARDUINO)
#include "Arduino.h"

#include "roo_backport.h"
#include "roo_backport/string_view.h"
#include "roo_logging/config.h"
#include "roo_logging/predict.h"
#include "roo_time.h"

namespace roo_logging {

class ArduinoLogStream : public Print {
 public:
  ArduinoLogStream(char* buf, size_t cap)
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

#if (defined(ESP32) || defined(ROO_TESTING))
  // Optimized version of printf.
  size_t printf(const char* format, ...);
#endif

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

}  // namespace roo_logging

#endif
