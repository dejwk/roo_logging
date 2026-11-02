#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace roo_logging {
/// Formats log values into borrowed fixed-capacity storage without a framework.
class BufferedLogStream {
 public:
  /// Uses @p buf with @p cap bytes, reserving one byte for a terminator.
  /// A zero capacity permits a null buffer and accepts no output.
  BufferedLogStream(char* buf, size_t cap)
      : buf_(buf), pos_(0), cap_(cap), number_base_(10), ctr_(0) {}

  /// Returns available payload capacity, excluding the terminator.
  size_t remaining_capacity() const { return cap_ == 0 ? 0 : cap_ - pos_ - 1; }
  /// Reports whether no more payload fits.
  bool full() const { return remaining_capacity() == 0; }
  /// Selects an integer base in 2..36; invalid values select decimal.
  void setBase(int base) { number_base_ = base < 2 || base > 36 ? 10 : base; }
  /// Appends one character if it fits.
  bool write(char value) {
    if (full()) return false;
    buf_[pos_++] = value;
    return true;
  }

  /// Appends up to @p len bytes, returning the number that fit.
  size_t write(const char* bytes, size_t len) {
    if (len > remaining_capacity()) len = remaining_capacity();
    if (len != 0) std::memcpy(buf_ + pos_, bytes, len);
    pos_ += len;
    return len;
  }

  /// Formats and truncates output to the remaining payload capacity.
  size_t printf(const char* format, ...);

  /// Appends an integer in @p base, with a minus sign for decimal negatives.
  template <typename T>
  size_t print(T value, uint8_t base) {
    static_assert(std::is_integral<T>::value, "integer required");
    if (base < 2 || base > 36) base = 10;
    using Unsigned = typename std::make_unsigned<T>::type;
    Unsigned magnitude = static_cast<Unsigned>(value);
    size_t count = 0;
    if (std::is_signed<T>::value && value < 0 && base == 10) {
      count = write('-') ? 1 : 0;
      magnitude = Unsigned{0} - magnitude;
    }
    return count + printUnsigned(magnitude, base);
  }

  /// Returns the selected integer base.
  int number_base() const { return number_base_; }
  /// Returns the number of payload bytes written.
  int pcount() const { return pos_; }
  /// Returns the counter used by LOG_EVERY_N.
  int ctr() const { return ctr_; }
  /// Updates the counter used by LOG_EVERY_N.
  void set_ctr(int value) { ctr_ = value; }

  // Retained public storage fields support existing stream insertion helpers.
  char* buf_;
  size_t pos_;
  size_t cap_;
  int number_base_;
  int ctr_;

 private:
  size_t printUnsigned(unsigned long long value, uint8_t base);
};
}  // namespace roo_logging
