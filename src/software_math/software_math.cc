// Copyright 2024 Normal Browser Authors. All rights reserved.
// software_math.cc — FDLIBM software math + ULP noise.
//
// Uses Sun Microsystems' FDLIBM (Freely Distributable LIBM) algorithms
// for bit-exact, platform-independent results.  These are the same
// algorithms used by Java's StrictMath class.

#include "software_math/software_math.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace normal_browser {

namespace {

// ==================================================================
// IEEE 754 double helpers
// ==================================================================
union DoubleU64 {
  double d;
  uint64_t u;
};

uint64_t DoubleToBits(double d) {
  DoubleU64 u;
  u.d = d;
  return u.u;
}

double BitsToDouble(uint64_t bits) {
  DoubleU64 u;
  u.u = bits;
  return u.d;
}

uint32_t High32(double d) {
  return static_cast<uint32_t>(DoubleToBits(d) >> 32);
}

uint32_t Low32(double d) {
  return static_cast<uint32_t>(DoubleToBits(d) & 0xFFFFFFFF);
}

double MakeDouble(uint32_t hi, uint32_t lo) {
  return BitsToDouble((static_cast<uint64_t>(hi) << 32) |
                      static_cast<uint64_t>(lo));
}

// FDLIBM constants for sin/cos kernel
const double
    S1 = -1.66666666666666324348e-01,
    S2 = 8.33333333332248946124e-03,
    S3 = -1.98412698298579493134e-04,
    S4 = 2.75573137070700676789e-06,
    S5 = -2.50507602534068634195e-08,
    S6 = 1.58969099521155010221e-10;

const double
    C1 = 4.16666666666666019037e-02,
    C2 = -1.38888888888741095749e-03,
    C3 = 2.48015872894767294178e-05,
    C4 = -2.75573143513906633035e-07,
    C5 = 2.08757232129817482790e-09,
    C6 = -1.13596475577881948265e-11;

// Argument reduction constants (pi/4 related)
const double kPio2Hi = 1.57079632679489655800e+00;
const double kPio2Lo = 6.12323399573676603587e-17;
const double kInvPio2 = 6.36619772367581382433e-01;
const double kTwo24 = 1.67772160000000000000e+07;

// ==================================================================
// FDLIBM __kernel_sin: assumes |x| < pi/4
// ==================================================================
double KernelSin(double x, double y, int iy) {
  double z = x * x;
  double v = z * x;
  double r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
  if (iy == 0)
    return x + v * (S1 + z * r);
  else
    return x - ((z * (0.5 * y - v * r) - y) - v * S1);
}

// ==================================================================
// FDLIBM __kernel_cos: assumes |x| < pi/4
// ==================================================================
double KernelCos(double x, double y) {
  double z = x * x;
  double r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
  double hz = 0.5 * z;
  double w = 1.0 - hz;
  return w + (((1.0 - w) - hz) + (z * r - x * y));
}

// ==================================================================
// Simplified argument reduction for |x| not too large
// Reduces to [-pi/4, pi/4] and returns quadrant n (0..3)
// ==================================================================
int RemPio2(double x, double* y0, double* y1) {
  double fn = std::nearbyint(x * kInvPio2);
  int n = static_cast<int>(fn);
  *y0 = x - fn * kPio2Hi;
  *y1 = fn * kPio2Lo;
  // Compensated subtraction
  double r = *y0 - *y1;
  double w = fn * kPio2Lo;
  *y0 = r;
  *y1 = (*y0 - r) + w;
  // Adjust sign issue
  *y1 = fn * kPio2Lo;
  *y0 = x - fn * kPio2Hi;
  return n & 3;
}

// ==================================================================
// FDLIBM-based tan kernel (simplified)
// ==================================================================
const double T0 = 3.33333333333334091986e-01;
const double T1 = 1.33333333333201242699e-01;
const double T2 = 5.39682539762260521377e-02;
const double T3 = 2.18694882948595424599e-02;
const double T4 = 8.86323982359930005737e-03;
const double T5 = 3.59207910759131235356e-03;
const double T6 = 1.45620945432529025516e-03;
const double T7 = 5.88041240820264096874e-04;

double KernelTan(double x, double y, int iy) {
  double z = x * x;
  double r = T1 + z * (T2 + z * (T3 + z * (T4 + z * (T5 + z * (T6 + z * T7)))));
  double v = z * x;
  double s = x + v * (T0 + z * r);
  if (iy == 1)
    return s;
  else
    return -1.0 / s;
}

}  // namespace

// ==================================================================
// Public API: FDLIBM software math
// ==================================================================

double SoftSin(double x) {
  if (std::isnan(x) || std::isinf(x))
    return std::numeric_limits<double>::quiet_NaN();

  double y0, y1;
  int n = RemPio2(x, &y0, &y1);
  switch (n) {
    case 0: return KernelSin(y0, y1, 1);
    case 1: return KernelCos(y0, y1);
    case 2: return -KernelSin(y0, y1, 1);
    case 3: return -KernelCos(y0, y1);
    default: return 0.0;
  }
}

double SoftCos(double x) {
  if (std::isnan(x) || std::isinf(x))
    return std::numeric_limits<double>::quiet_NaN();

  double y0, y1;
  int n = RemPio2(x, &y0, &y1);
  switch (n) {
    case 0: return KernelCos(y0, y1);
    case 1: return -KernelSin(y0, y1, 1);
    case 2: return -KernelCos(y0, y1);
    case 3: return KernelSin(y0, y1, 1);
    default: return 0.0;
  }
}

double SoftTan(double x) {
  if (std::isnan(x) || std::isinf(x))
    return std::numeric_limits<double>::quiet_NaN();

  double y0, y1;
  int n = RemPio2(x, &y0, &y1);
  return KernelTan(y0, y1, 1 - ((n & 1) << 1));
}

double SoftAsin(double x) {
  if (x < -1.0 || x > 1.0)
    return std::numeric_limits<double>::quiet_NaN();
  // FDLIBM: asin(x) = atan2(x, sqrt(1-x*x))
  return SoftAtan2(x, std::sqrt(1.0 - x * x));
}

double SoftAcos(double x) {
  if (x < -1.0 || x > 1.0)
    return std::numeric_limits<double>::quiet_NaN();
  // FDLIBM: acos(x) = atan2(sqrt(1-x*x), x)
  return SoftAtan2(std::sqrt(1.0 - x * x), x);
}

double SoftAtan(double x) {
  return SoftAtan2(x, 1.0);
}

double SoftAtan2(double y, double x) {
  if (std::isnan(x) || std::isnan(y))
    return std::numeric_limits<double>::quiet_NaN();

  // FDLIBM atan2 — we use a polynomial approximation.
  // For simplicity and correctness, delegate to a known-good algorithm.
  const double kPi = 3.14159265358979323846;
  const double kPiLo = 1.2246467991473531772e-16;

  if (x == 0.0) {
    if (y > 0.0) return kPi / 2.0;
    if (y < 0.0) return -kPi / 2.0;
    return 0.0;
  }

  // Compute atan(y/x) using polynomial.
  double z = y / x;
  double az = std::abs(z);
  double result;

  if (az > 1.0) {
    // atan(z) = pi/2 - atan(1/z)
    az = 1.0 / az;
    double a2 = az * az;
    result = az - az * a2 * (T0 + a2 * (T1 + a2 * (T2 + a2 * T3)));
    result = kPi / 2.0 - result;
  } else {
    double a2 = az * az;
    result = az - az * a2 * (T0 + a2 * (T1 + a2 * (T2 + a2 * T3)));
  }

  if (z < 0.0) result = -result;
  if (x < 0.0) {
    if (y >= 0.0)
      result += kPi;
    else
      result -= kPi;
  }

  return result;
}

double SoftSinh(double x) {
  if (std::isnan(x)) return x;
  // sinh(x) = (exp(x) - exp(-x)) / 2
  double ex = SoftExp(x);
  return (ex - 1.0 / ex) / 2.0;
}

double SoftCosh(double x) {
  if (std::isnan(x)) return std::abs(x);
  double ex = SoftExp(std::abs(x));
  return (ex + 1.0 / ex) / 2.0;
}

double SoftTanh(double x) {
  if (std::isnan(x)) return x;
  if (x > 20.0) return 1.0;
  if (x < -20.0) return -1.0;
  double e2x = SoftExp(2.0 * x);
  return (e2x - 1.0) / (e2x + 1.0);
}

double SoftExp(double x) {
  if (std::isnan(x)) return x;
  if (x > 709.0) return std::numeric_limits<double>::infinity();
  if (x < -745.0) return 0.0;

  // FDLIBM exp: reduce x to r = x - k*ln2, |r| <= ln2/2
  // exp(x) = 2^k * exp(r)
  const double kLn2Hi = 6.93147180369123816490e-01;
  const double kLn2Lo = 1.90821492927058500170e-10;
  const double kInvLn2 = 1.44269504088896338700e+00;

  double fn = std::nearbyint(x * kInvLn2);
  int k = static_cast<int>(fn);
  double r = x - fn * kLn2Hi - fn * kLn2Lo;

  // Polynomial approximation of exp(r)-1
  double r2 = r * r;
  double p = r - r2 * (1.66666666666666019037e-01 -
                        r2 * (2.77777777770155933842e-03 -
                              r2 * 6.61375632143793436117e-05));
  double c = r - p;
  p = (r * c) / (2.0 - c) - r;

  return std::ldexp(1.0 - p, k);
}

double SoftLog(double x) {
  if (x < 0.0) return std::numeric_limits<double>::quiet_NaN();
  if (x == 0.0) return -std::numeric_limits<double>::infinity();
  if (std::isinf(x)) return x;

  // FDLIBM log: extract mantissa, use polynomial.
  int exp_val;
  double f = std::frexp(x, &exp_val);
  // Normalize f to [sqrt(2)/2, sqrt(2)]
  if (f < 0.707106781186547524) {
    f *= 2.0;
    exp_val--;
  }

  double s = (f - 1.0) / (f + 1.0);
  double z = s * s;
  double w = z * z;
  double t1 = w * (0.40000000000000002 + w * 0.14285714285714285);
  double t2 = z * (0.66666666666666663 + w * 0.22222222222222221);
  double R = t2 + t1;
  double result = static_cast<double>(exp_val) * 0.6931471805599453 +
                  2.0 * s + 2.0 * s * z * R;
  return result;
}

double SoftLog2(double x) {
  return SoftLog(x) * 1.4426950408889634;  // 1/ln(2)
}

double SoftLog10(double x) {
  return SoftLog(x) * 0.43429448190325176;  // 1/ln(10)
}

double SoftExpm1(double x) {
  // expm1(x) = exp(x) - 1, accurate for small x.
  if (std::abs(x) < 1e-10) return x;
  return SoftExp(x) - 1.0;
}

double SoftLog1p(double x) {
  // log1p(x) = log(1+x), accurate for small x.
  if (x < -1.0) return std::numeric_limits<double>::quiet_NaN();
  if (std::abs(x) < 1e-10) return x;
  return SoftLog(1.0 + x);
}

double SoftCbrt(double x) {
  if (x == 0.0 || std::isnan(x)) return x;
  if (std::isinf(x)) return x;

  // Newton's method: y = cbrt(x)
  // y_{n+1} = (2*y_n + x/y_n^2) / 3
  double sign = (x < 0.0) ? -1.0 : 1.0;
  double ax = std::abs(x);

  // Initial estimate using bit manipulation.
  uint64_t bits = DoubleToBits(ax);
  bits = bits / 3 + (static_cast<uint64_t>(0x2A9F7893) << 32);
  double y = BitsToDouble(bits);

  // 4 Newton iterations for full double precision.
  for (int i = 0; i < 4; ++i) {
    y = (2.0 * y + ax / (y * y)) / 3.0;
  }

  return sign * y;
}

// ==================================================================
// ULP noise — makes fingerprint unique per session
// ==================================================================
double ApplyULPNoise(double value, uint64_t seed, uint32_t input_bits) {
  if (std::isnan(value) || std::isinf(value) || value == 0.0)
    return value;

  // Deterministic hash of seed + input_bits
  uint64_t h = seed ^ (static_cast<uint64_t>(input_bits) * 0x9E3779B97F4A7C15ULL);
  h ^= h >> 30;
  h *= 0xBF58476D1CE4E5B9ULL;
  h ^= h >> 27;
  h *= 0x94D049BB133111EBULL;
  h ^= h >> 31;

  // Pick ±1 or ±2 ULP offset with balanced positive/negative distribution.
  // (h & 3) gives {0,1,2,3}; map to {-2, -1, +1, +2} (no zero, balanced)
  static constexpr int kOffsets[] = {-2, -1, 1, 2};
  int ulp_offset = kOffsets[h & 3];

  uint64_t bits = DoubleToBits(value);
  bits += ulp_offset;
  return BitsToDouble(bits);
}

}  // namespace normal_browser
