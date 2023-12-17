#include <Arduino.h>
#include <Printable.h>

#include <roo_logging.h>

void setup() {}

// One possibility for logging custom classes is to make them implement the
// Printable interface.
//
// A big benefit of that is that it also makes the class printable to Serial
// (and, generally, to anything implementing Print). But there are two caveats:
// 1. sometimes you may not be able to change the definition of the class,
// 2. Making your class virtual adds some storage and CPU time overhead.
class PrintableRect : public Printable {
 public:
  PrintableRect(int x0, int y0, int x1, int y1)
      : x0_(x0), y0_(y0), x1_(x1), y1_(y1) {}

  size_t printTo(Print& p) const override {
    return p.printf("PrintableRect(%d, %d, %d, %d)", x0_, y0_, x1_, y1_);
  }

 private:
  int x0_, y0_, x1_, y1_;
};

// Another possibility is to define the << operator. We don't use std::ostream,
// because it adds significant overhead to the binary. Instead, we're using
// roo_logging::Stream.
//
// Here, we're using a struct, to illustrate that this method is applicable to
// very simple data structures.
struct PlainRect {
  int x0, y0, x1, y1;
};

roo_logging::Stream& operator<<(roo_logging::Stream& s, const PlainRect& r) {
  s << "PlainRect{" << r.x0 << ", " << r.y0 << ", " << r.x1 << ", " << r.y1
    << "}";
  // Also works:
  // s.printf("PlainRect{%d, %d, %d, %d}", r.x0, r.y0, r.x1, r.y1);
  return s;
}

void loop() {
  PrintableRect r1(2, 3, 10, 13);
  PlainRect r2{4, 5, 14, 17};

  LOG(INFO) << r1;
  LOG(INFO) << r2;
  delay(1000);
}
