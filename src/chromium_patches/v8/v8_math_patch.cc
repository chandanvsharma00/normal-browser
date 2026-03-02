// Copyright 2024 Normal Browser Authors. All rights reserved.
// v8_math_patch.cc — V8 Math builtin override for CPU-independent results.
//
// FILES TO MODIFY:
//   v8/src/builtins/builtins-math-gen.cc   (TurboFan compiled builtins)
//   v8/src/codegen/code-stub-assembler.cc  (CSA helpers)
//   v8/src/runtime/runtime-math.cc         (runtime fallbacks)
//   v8/src/compiler/js-call-reducer.cc     (inlining reductions)
//
// STRATEGY:
//   Replace all V8 transcendental Math.* implementations with calls to
//   our SoftwareMath module (FDLIBM-based), then apply per-session ULP
//   noise via ApplyULPNoise().
//
//   The key insight: V8 normally calls hardware FPU instructions like
//   __ieee754_sin(), which produce CPU-dependent results (Cortex-X4 ≠ A78).
//   By replacing with software FDLIBM, we get identical base results on
//   ALL architectures, then add session-specific noise.

#include "software_math/software_math.h"
#include "chromium_patches/v8/v8_math_patch.h"

// NOTE: V8 runs in the RENDERER process. DeviceProfileStore is browser-only
// and would always be empty here. We must use GhostProfileClient which is
// the renderer-side profile cache, populated via Mojo from the browser.
#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstdint>
#include <cmath>
#include <cstring>

namespace v8_math_patch {

// ====================================================================
// Helper: Get the math noise seed from profile
//
// IMPORTANT CHANGE: ULP noise has been DISABLED.
//
// REASON: FPJS Pro hashes the output of Math.sin, cos, tan, atan2, exp,
// expm1, log, log1p, cbrt, sinh, cosh, tanh = 13+ functions across
// specific inputs (e.g., Math.tan(-1e300), Math.atan2(1,2)).
// The combined hash is compared against a database of known results
// for EVERY real CPU (Cortex-X4, A78, A55, Exynos M1, etc.).
//
// Our FDLIBM implementation produces outputs matching Java's StrictMath,
// which corresponds to a KNOWN fingerprint ("FDLIBM/Java pattern").
// Many real devices (older Android with software FP) produce this exact
// pattern. So pure FDLIBM output is SAFE — it looks like a real device.
//
// Adding ±1-2 ULP noise on top produces a hash that matches NO known
// CPU, which is an immediate anomaly signal. Removing the noise means
// all sessions produce the same Math hash — but that hash matches
// real devices, so FPJS won't flag it.
// ====================================================================
static uint64_t GetMathSeed() {
  // Disabled — always return 0 so no ULP noise is applied.
  // Pure FDLIBM results match known device patterns.
  return 0;
}

// ====================================================================
// Helper: Combine function ID with actual input bits to produce
// unique noise per (function, input) pair.
// Without this, sin(0.5) and sin(1.5) would get identical ULP shifts,
// which fingerprinters can detect.
// ====================================================================
static uint32_t MakeInputBits(double x, uint32_t func_id) {
  uint64_t x_bits;
  std::memcpy(&x_bits, &x, sizeof(x));
  return static_cast<uint32_t>(x_bits ^ (x_bits >> 32)) ^ func_id;
}

static uint32_t MakeInputBits2(double y, double x, uint32_t func_id) {
  uint64_t y_bits, x_bits;
  std::memcpy(&y_bits, &y, sizeof(y));
  std::memcpy(&x_bits, &x, sizeof(x));
  return static_cast<uint32_t>(y_bits ^ (y_bits >> 32) ^
                               x_bits ^ (x_bits >> 17)) ^ func_id;
}

// ====================================================================
// PATCH: Math.sin(x)
// File: v8/src/builtins/builtins-math-gen.cc
//   Replace: Float64Sin node → call to SoftSin + ULP noise
// File: v8/src/runtime/runtime-math.cc
//   Replace: RUNTIME_FUNCTION(Runtime_MathSin)
// ====================================================================
double PatchedMathSin(double x) {
  double result = normal_browser::SoftSin(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0001));
  }
  return result;
}

// ====================================================================
// PATCH: Math.cos(x)
// ====================================================================
double PatchedMathCos(double x) {
  double result = normal_browser::SoftCos(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0002));
  }
  return result;
}

// ====================================================================
// PATCH: Math.tan(x)
// This is the PRIMARY fingerprinting vector — Math.tan(-1e300) differs
// across CPU architectures more than any other function.
// ====================================================================
double PatchedMathTan(double x) {
  double result = normal_browser::SoftTan(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0003));
  }
  return result;
}

// ====================================================================
// PATCH: Math.asin(x)
// ====================================================================
double PatchedMathAsin(double x) {
  double result = normal_browser::SoftAsin(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0004));
  }
  return result;
}

// ====================================================================
// PATCH: Math.acos(x)
// ====================================================================
double PatchedMathAcos(double x) {
  double result = normal_browser::SoftAcos(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0005));
  }
  return result;
}

// ====================================================================
// PATCH: Math.atan(x)
// ====================================================================
double PatchedMathAtan(double x) {
  double result = normal_browser::SoftAtan(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0006));
  }
  return result;
}

// ====================================================================
// PATCH: Math.atan2(y, x)
// FPJS uses atan2(1, 2) as a hardware fingerprint.
// ====================================================================
double PatchedMathAtan2(double y, double x) {
  double result = normal_browser::SoftAtan2(y, x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits2(y, x, 0x0007));
  }
  return result;
}

// ====================================================================
// PATCH: Math.exp(x)
// ====================================================================
double PatchedMathExp(double x) {
  double result = normal_browser::SoftExp(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0008));
  }
  return result;
}

// ====================================================================
// PATCH: Math.expm1(x) — exp(x) - 1 with better precision near 0
// FPJS fingerprints this specifically.
// ====================================================================
double PatchedMathExpm1(double x) {
  double result = normal_browser::SoftExpm1(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0009));
  }
  return result;
}

// ====================================================================
// PATCH: Math.log(x)
// ====================================================================
double PatchedMathLog(double x) {
  double result = normal_browser::SoftLog(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000A));
  }
  return result;
}

// ====================================================================
// PATCH: Math.log2(x)
// ====================================================================
double PatchedMathLog2(double x) {
  double result = normal_browser::SoftLog2(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000B));
  }
  return result;
}

// ====================================================================
// PATCH: Math.log10(x)
// ====================================================================
double PatchedMathLog10(double x) {
  double result = normal_browser::SoftLog10(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000C));
  }
  return result;
}

// ====================================================================
// PATCH: Math.log1p(x) — log(1 + x) with better precision near 0
// FPJS fingerprints this specifically.
// ====================================================================
double PatchedMathLog1p(double x) {
  double result = normal_browser::SoftLog1p(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000D));
  }
  return result;
}

// ====================================================================
// PATCH: Math.cbrt(x) — cube root
// FPJS fingerprints this specifically.
// ====================================================================
double PatchedMathCbrt(double x) {
  double result = normal_browser::SoftCbrt(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000E));
  }
  return result;
}

// ====================================================================
// PATCH: Math.sinh(x)
// ====================================================================
double PatchedMathSinh(double x) {
  double result = normal_browser::SoftSinh(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x000F));
  }
  return result;
}

// ====================================================================
// PATCH: Math.cosh(x)
// ====================================================================
double PatchedMathCosh(double x) {
  double result = normal_browser::SoftCosh(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0010));
  }
  return result;
}

// ====================================================================
// PATCH: Math.tanh(x)
// ====================================================================
double PatchedMathTanh(double x) {
  double result = normal_browser::SoftTanh(x);
  uint64_t seed = GetMathSeed();
  if (seed != 0) {
    result = normal_browser::ApplyULPNoise(result, seed, MakeInputBits(x, 0x0011));
  }
  return result;
}

// ====================================================================
// INTEGRATION: V8 TurboFan Compiler Reduction
// File: v8/src/compiler/js-call-reducer.cc
//
// V8's optimizing compiler (TurboFan) inlines Math.* calls to
// Float64Sin, Float64Cos, etc. machine nodes. We need to prevent
// this inlining and force calls through our patched functions.
//
// In ReduceMathUnaryBuiltin():
//   - For kMathSin: call PatchedMathSin instead of Float64Sin
//   - For kMathCos: call PatchedMathCos instead of Float64Cos
//   - etc.
//
// Alternative approach: Replace the external references in
// v8/src/codegen/external-reference.cc:
//   ExternalReference::ieee754_sin_function() → PatchedMathSin
//   ExternalReference::ieee754_cos_function() → PatchedMathCos
//   etc.
// This is cleaner because it catches ALL codepaths (interpreter,
// Sparkplug, Maglev, TurboFan).
// ====================================================================

// Function pointer table for V8 integration.
// Uses V8MathOverrides struct from v8_math_patch.h.
// V8's external-reference.cc reads from this.
const V8MathOverrides& GetMathOverrides() {
  static V8MathOverrides overrides = {
      PatchedMathSin,
      PatchedMathCos,
      PatchedMathTan,
      PatchedMathAsin,
      PatchedMathAcos,
      PatchedMathAtan,
      PatchedMathAtan2,
      PatchedMathExp,
      PatchedMathExpm1,
      PatchedMathLog,
      PatchedMathLog2,
      PatchedMathLog10,
      PatchedMathLog1p,
      PatchedMathCbrt,
      PatchedMathSinh,
      PatchedMathCosh,
      PatchedMathTanh,
  };
  return overrides;
}

// ====================================================================
// PATCH POINT in v8/src/codegen/external-reference.cc:
//
// BEFORE:
//   ExternalReference ExternalReference::ieee754_sin_function() {
//     return ExternalReference(Redirect(FUNCTION_ADDR(base::ieee754::sin)));
//   }
//
// AFTER:
//   ExternalReference ExternalReference::ieee754_sin_function() {
//     return ExternalReference(
//       Redirect(FUNCTION_ADDR(v8_math_patch::PatchedMathSin)));
//   }
//
// Repeat for all 17 transcendental functions.
// ====================================================================

}  // namespace v8_math_patch
