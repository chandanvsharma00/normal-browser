// Copyright 2024 Normal Browser Authors. All rights reserved.
// v8_math_patch.h — V8 Math builtin overrides header.

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_V8_MATH_PATCH_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_V8_MATH_PATCH_H_

namespace v8_math_patch {

// All patched Math.* functions — FDLIBM base + per-session ULP noise.
double PatchedMathSin(double x);
double PatchedMathCos(double x);
double PatchedMathTan(double x);
double PatchedMathAsin(double x);
double PatchedMathAcos(double x);
double PatchedMathAtan(double x);
double PatchedMathAtan2(double y, double x);
double PatchedMathExp(double x);
double PatchedMathExpm1(double x);
double PatchedMathLog(double x);
double PatchedMathLog2(double x);
double PatchedMathLog10(double x);
double PatchedMathLog1p(double x);
double PatchedMathCbrt(double x);
double PatchedMathSinh(double x);
double PatchedMathCosh(double x);
double PatchedMathTanh(double x);

// Function pointer table struct for V8 external reference integration.
using MathFunc = double (*)(double);
using MathFunc2 = double (*)(double, double);

struct V8MathOverrides {
  MathFunc sin_func;
  MathFunc cos_func;
  MathFunc tan_func;
  MathFunc asin_func;
  MathFunc acos_func;
  MathFunc atan_func;
  MathFunc2 atan2_func;
  MathFunc exp_func;
  MathFunc expm1_func;
  MathFunc log_func;
  MathFunc log2_func;
  MathFunc log10_func;
  MathFunc log1p_func;
  MathFunc cbrt_func;
  MathFunc sinh_func;
  MathFunc cosh_func;
  MathFunc tanh_func;
};

const V8MathOverrides& GetMathOverrides();

}  // namespace v8_math_patch

#endif  // NORMAL_BROWSER_CHROMIUM_PATCHES_V8_MATH_PATCH_H_
