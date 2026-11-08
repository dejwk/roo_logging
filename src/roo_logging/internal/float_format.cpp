#include "roo_logging/internal/float_format.h"

#if ROO_LOGGING_HAS_COMPACT_FLOAT

#include <cstdint>
#include <cstring>

namespace roo_logging {
namespace internal {
namespace {

// Normalizing a binary64 value as a ratio of integers needs at most 807 bits.
// Twenty-six limbs leave room for digit extraction and the rounding comparison.
class BigUInt {
 public:
  /// Creates the positive integer @p value.
  explicit BigUInt(uint64_t value) : limbs_{}, size_(1) {
    limbs_[0] = static_cast<uint32_t>(value);
    if (value >> 32 != 0) limbs_[size_++] = static_cast<uint32_t>(value >> 32);
  }

  /// Multiplies this integer by @p factor without allocating.
  void multiply(uint32_t factor) {
    uint64_t carry = 0;
    for (int i = 0; i < size_; ++i) {
      const uint64_t product =
          static_cast<uint64_t>(limbs_[i]) * factor + carry;
      limbs_[i] = static_cast<uint32_t>(product);
      carry = product >> 32;
    }
    if (carry != 0) limbs_[size_++] = static_cast<uint32_t>(carry);
  }

  /// Multiplies by five to the nonnegative power @p exponent.
  void multiplyPow5(int exponent) {
    while (exponent >= 13) {
      multiply(1220703125);  // 5^13 fits in one limb.
      exponent -= 13;
    }
    uint32_t factor = 1;
    while (exponent-- > 0) factor *= 5;
    multiply(factor);
  }

  /// Multiplies by two to the nonnegative power @p bits.
  void shiftLeft(int bits) {
    const int words = bits / 32;
    for (int i = size_ - 1; i >= 0; --i) limbs_[i + words] = limbs_[i];
    for (int i = 0; i < words; ++i) limbs_[i] = 0;
    size_ += words;
    const int shift = bits % 32;
    if (shift == 0) return;
    uint32_t carry = 0;
    for (int i = words; i < size_; ++i) {
      const uint32_t next_carry = limbs_[i] >> (32 - shift);
      limbs_[i] = (limbs_[i] << shift) | carry;
      carry = next_carry;
    }
    if (carry != 0) limbs_[size_++] = carry;
  }

  /// Compares magnitudes, returning -1, 0, or 1.
  int compare(const BigUInt& other) const {
    if (size_ != other.size_) return size_ < other.size_ ? -1 : 1;
    for (int i = size_ - 1; i >= 0; --i) {
      if (limbs_[i] != other.limbs_[i]) {
        return limbs_[i] < other.limbs_[i] ? -1 : 1;
      }
    }
    return 0;
  }

  /// Extracts a small quotient and leaves the remainder in this integer.
  /// Conversion keeps the quotient below 100 initially, and below 10
  /// thereafter.
  uint32_t extractQuotient(const BigUInt& divisor) {
    uint32_t quotient = 0;
    while (compare(divisor) >= 0) {
      uint64_t borrow = 0;
      for (int i = 0; i < size_; ++i) {
        const uint32_t digit = i < divisor.size_ ? divisor.limbs_[i] : 0;
        const uint64_t difference =
            static_cast<uint64_t>(limbs_[i]) - digit - borrow;
        limbs_[i] = static_cast<uint32_t>(difference);
        borrow = difference >> 63;
      }
      while (size_ > 1 && limbs_[size_ - 1] == 0) --size_;
      ++quotient;
    }
    return quotient;
  }

 private:
  uint32_t limbs_[26];
  int size_;
};

// Converts an exact binary value to six rounded decimal digits and an exponent.
uint32_t RoundedSignificand(uint64_t mantissa, int exponent2, int& exponent10) {
  int highest_bit = 0;
  for (uint64_t bits = mantissa; bits > 1; bits >>= 1) ++highest_bit;
  // log10(2) ~= 78913 / 262144. Floor rather than truncate negative products;
  // the resulting decimal exponent is at most one away from the true exponent.
  const int32_t estimate =
      static_cast<int32_t>(exponent2 + highest_bit) * 78913;
  exponent10 = estimate / 262144;
  if (estimate < 0 && estimate % 262144 != 0) --exponent10;

  BigUInt numerator(mantissa);
  BigUInt denominator(1);
  if (exponent10 < 0) {
    numerator.multiplyPow5(-exponent10);
  } else {
    denominator.multiplyPow5(exponent10);
  }
  const int shift = exponent2 - exponent10;
  if (shift >= 0) {
    numerator.shiftLeft(shift);
  } else {
    denominator.shiftLeft(-shift);
  }

  // Correct the exponent estimate while extracting the first one or two digits.
  uint32_t significand = numerator.extractQuotient(denominator);
  while (significand == 0) {
    numerator.multiply(10);
    significand = numerator.extractQuotient(denominator);
    --exponent10;
  }
  int digits = 1;
  if (significand >= 10) {
    digits = 2;
    ++exponent10;
  }
  for (; digits < 6; ++digits) {
    numerator.multiply(10);
    significand = significand * 10 + numerator.extractQuotient(denominator);
  }

  // The exact remainder decides rounding, including decimal ties and carries
  // that change whether %g chooses fixed or scientific notation.
  numerator.multiply(2);
  const int rounding = numerator.compare(denominator);
  if (rounding > 0 || (rounding == 0 && significand % 2 != 0)) ++significand;
  if (significand == 1000000) {
    significand = 100000;
    ++exponent10;
  }
  return significand;
}

}  // namespace

size_t FormatFloat(double value, char (&buffer)[kFloatBufferSize]) {
#if DBL_MANT_DIG == 53
  using Bits = uint64_t;
  constexpr int kMantissaBits = 52;
  constexpr int kExponentBits = 11;
  constexpr int kBias = 1023;
#else
  using Bits = uint32_t;
  constexpr int kMantissaBits = 23;
  constexpr int kExponentBits = 8;
  constexpr int kBias = 127;
#endif
  static_assert(sizeof(Bits) == sizeof(value), "IEEE double storage required");
  Bits bits;
  std::memcpy(&bits, &value, sizeof(bits));
  const int raw_exponent =
      static_cast<int>((bits >> kMantissaBits) & ((1u << kExponentBits) - 1));
  const Bits mantissa = bits & ((Bits{1} << kMantissaBits) - 1);
  char* out = buffer;
  if (bits >> (kMantissaBits + kExponentBits) != 0) *out++ = '-';
  if (raw_exponent == (1 << kExponentBits) - 1) {
    std::memcpy(out, mantissa == 0 ? "inf" : "nan", 4);
    return out - buffer + 3;
  }
  if (raw_exponent == 0 && mantissa == 0) {
    *out++ = '0';
    *out = '\0';
    return out - buffer;
  }

  int exponent10;
  uint32_t significand = RoundedSignificand(
      raw_exponent == 0 ? mantissa : mantissa | (Bits{1} << kMantissaBits),
      (raw_exponent == 0 ? 1 : raw_exponent) - kBias - kMantissaBits,
      exponent10);
  char digits[6];
  for (int i = 5; i >= 0; --i) {
    digits[i] = '0' + significand % 10;
    significand /= 10;
  }
  int count = 6;
  while (count > 1 && digits[count - 1] == '0') --count;
  if (exponent10 < -4 || exponent10 >= 6) {
    *out++ = digits[0];
    if (count > 1) {
      *out++ = '.';
      std::memcpy(out, digits + 1, count - 1);
      out += count - 1;
    }
    *out++ = 'e';
    *out++ = exponent10 < 0 ? '-' : '+';
    if (exponent10 < 0) exponent10 = -exponent10;
    if (exponent10 >= 100) *out++ = '0' + exponent10 / 100;
    *out++ = '0' + exponent10 / 10 % 10;
    *out++ = '0' + exponent10 % 10;
  } else if (exponent10 < 0) {
    *out++ = '0';
    *out++ = '.';
    for (int i = 0; i < -exponent10 - 1; ++i) *out++ = '0';
    std::memcpy(out, digits, count);
    out += count;
  } else {
    for (int i = 0; i <= exponent10 || i < count; ++i) {
      if (i == exponent10 + 1) *out++ = '.';
      *out++ = i < count ? digits[i] : '0';
    }
  }
  *out = '\0';
  return out - buffer;
}

}  // namespace internal
}  // namespace roo_logging

#endif  // ROO_LOGGING_HAS_COMPACT_FLOAT
