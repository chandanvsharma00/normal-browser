// Copyright 2024 Normal Browser Authors. All rights reserved.
// software_math.h — FDLIBM-based software math for CPU-independent results.
//
// V8 normally delegates Math.sin/cos/tan/etc. to the platform libm which
// can vary by CPU micro-architecture, producing a fingerprint-able signal.
// This module provides bit-exact software implementations (FDLIBM) plus
// optional per-session ULP perturbation for fingerprint uniqueness.

#ifndef NORMAL_BROWSER_SOFTWARE_MATH_SOFTWARE_MATH_H_
#define NORMAL_BROWSER_SOFTWARE_MATH_SOFTWARE_MATH_H_

#include <cstdint>

namespace normal_browser {

// Pure software math (FDLIBM) — identical output on every CPU.
double SoftSin(double x);
double SoftCos(double x);
double SoftTan(double x);
double SoftAsin(double x);
double SoftAcos(double x);
double SoftAtan(double x);
double SoftAtan2(double y, double x);
double SoftSinh(double x);
double SoftCosh(double x);
double SoftTanh(double x);
double SoftExp(double x);
double SoftLog(double x);
double SoftLog2(double x);
double SoftLog10(double x);
double SoftExpm1(double x);
double SoftLog1p(double x);
double SoftCbrt(double x);

// Apply per-session ULP noise to a software-math result.
// |value|: the FDLIBM result.
// |seed|: session seed (profile.math_seed).
// |input_bits|: lower 32 bits of the input value, for deterministic noise.
// Returns value with ±1-2 ULP perturbation.
double ApplyULPNoise(double value, uint64_t seed, uint32_t input_bits);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_SOFTWARE_MATH_SOFTWARE_MATH_H_
