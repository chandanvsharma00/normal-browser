// Copyright 2024 Normal Browser Authors. All rights reserved.
// performance_timing_spoofing.cc — Normalize operation timing to match claimed SoC tier.
//
// PROBLEM:
//   If we claim a budget Helio G36 but the host runs on a fast desktop CPU,
//   canvas.toDataURL() completes in 2ms instead of 50-100ms.
//   This timing discrepancy can reveal the real hardware.
//
// SOLUTION:
//   Add artificial delay to timing-sensitive operations so that performance
//   matches the claimed SoC tier. Includes jitter to prevent correlation.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/modules/canvas/canvas2d/
//     canvas_rendering_context_2d.cc → toDataURL(), toBlob(), getImageData()
//   third_party/blink/renderer/modules/webgl/
//     webgl_rendering_context_base.cc → readPixels(), getParameter()
//   third_party/blink/renderer/core/timing/performance.cc →
//     performance.now() jitter (optional)
//
// INTEGRATION POINTS:
//   Call AddTierDelay(DelayCategory::kCanvas) at the START of toDataURL().
//   Call AddTierDelay(DelayCategory::kWebGL) at the START of readPixels().

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <random>
#include <thread>

namespace blink {

namespace {

// ====================================================================
// SoC tier timing ranges (from FEATURES.md F11)
//
// soc_tier: 0 = flagship, 1 = upper-mid, 2 = mid, 3 = budget
//
// Canvas toDataURL() expected times:
//   Flagship:   5 - 12 ms
//   Upper-Mid: 12 - 25 ms
//   Mid:       25 - 50 ms
//   Budget:    50 - 100 ms
//
// WebGL readPixels() expected times:
//   Flagship:   3 - 8 ms
//   Upper-Mid:  8 - 15 ms
//   Mid:       15 - 30 ms
//   Budget:    30 - 60 ms
// ====================================================================

struct TimingRange {
  int min_ms;
  int max_ms;
};

// Canvas timing per soc_tier [0..3]
const TimingRange kCanvasTiming[] = {
    {5, 12},    // tier 0: flagship
    {12, 25},   // tier 1: upper-mid
    {25, 50},   // tier 2: mid
    {50, 100},  // tier 3: budget
};

// WebGL timing per soc_tier [0..3]
const TimingRange kWebGLTiming[] = {
    {3, 8},     // tier 0: flagship
    {8, 15},    // tier 1: upper-mid
    {15, 30},   // tier 2: mid
    {30, 60},   // tier 3: budget
};

// Crypto/WASM timing per soc_tier [0..3]
const TimingRange kCryptoTiming[] = {
    {1, 3},     // tier 0: flagship
    {3, 8},     // tier 1: upper-mid
    {8, 15},    // tier 2: mid
    {15, 30},   // tier 3: budget
};

// JSON parse/sort timing per soc_tier [0..3]
const TimingRange kGenericTiming[] = {
    {1, 2},     // tier 0: flagship
    {2, 5},     // tier 1: upper-mid
    {5, 10},    // tier 2: mid
    {10, 20},   // tier 3: budget
};

// Thread-local PRNG for jitter — seeded from profile seed.
thread_local uint64_t tl_jitter_state = 0;

uint64_t JitterNext() {
  if (tl_jitter_state == 0) {
    // Initialize from high-resolution clock for entropy.
    auto now = std::chrono::high_resolution_clock::now();
    tl_jitter_state = static_cast<uint64_t>(
        now.time_since_epoch().count()) ^ 0xCAFEBABEDEAD1234ULL;
  }
  tl_jitter_state ^= tl_jitter_state << 13;
  tl_jitter_state ^= tl_jitter_state >> 7;
  tl_jitter_state ^= tl_jitter_state << 17;
  return tl_jitter_state;
}

// Get a random delay in [min_ms, max_ms] with ±15% jitter.
int ComputeDelay(const TimingRange& range) {
  int base = range.min_ms + static_cast<int>(
      JitterNext() % (range.max_ms - range.min_ms + 1));

  // Add ±15% jitter to prevent exact correlation.
  int jitter_range = base * 15 / 100;
  if (jitter_range > 0) {
    int jitter = static_cast<int>(JitterNext() % (jitter_range * 2 + 1)) - jitter_range;
    base += jitter;
  }

  // Clamp to reasonable bounds.
  if (base < range.min_ms) base = range.min_ms;
  if (base > range.max_ms * 2) base = range.max_ms * 2;  // allow some overflow from jitter
  return base;
}

}  // namespace

// ====================================================================
// Delay categories matching different API surfaces.
// ====================================================================
enum class DelayCategory {
  kCanvas,    // canvas.toDataURL(), toBlob(), getImageData()
  kWebGL,     // WebGL readPixels(), getParameter()
  kCrypto,    // crypto.subtle.digest(), WASM compilation
  kGeneric,   // JSON.parse(), Array.sort(), regex
};

// ====================================================================
// Main API: Add artificial delay based on SoC tier.
//
// Call at the START of timing-sensitive operations.
// The delay is a blocking sleep — matches what a real slower device would do.
//
// Integration example for canvas.toDataURL():
//   String CanvasRenderingContext2D::toDataURL(...) {
//     AddTierDelay(DelayCategory::kCanvas);  // ← add this line
//     ... original code ...
//   }
// ====================================================================
void AddTierDelay(DelayCategory category) {
  auto* client = GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;
  const auto& profile = client->GetProfile();
  int tier = profile.soc_tier;
  if (tier < 0) tier = 0;
  if (tier > 3) tier = 3;

  const TimingRange* ranges = nullptr;
  switch (category) {
    case DelayCategory::kCanvas:
      ranges = kCanvasTiming;
      break;
    case DelayCategory::kWebGL:
      ranges = kWebGLTiming;
      break;
    case DelayCategory::kCrypto:
      ranges = kCryptoTiming;
      break;
    case DelayCategory::kGeneric:
      ranges = kGenericTiming;
      break;
  }

  int delay_ms = ComputeDelay(ranges[tier]);

  // Sleep for the computed duration.
  // Note: std::this_thread::sleep_for is acceptable here because
  // these operations (toDataURL, readPixels) already block the main thread
  // on real devices. We're just making them take longer.
  std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
}

// ====================================================================
// Measure and adjust: If the actual operation already took long enough,
// don't add extra delay. This handles the case where the host is already
// slow (e.g., running on an emulator).
//
// Usage pattern:
//   auto start = std::chrono::high_resolution_clock::now();
//   ... do operation ...
//   auto end = std::chrono::high_resolution_clock::now();
//   AdjustTierDelay(DelayCategory::kCanvas, start, end);
// ====================================================================
void AdjustTierDelay(
    DelayCategory category,
    std::chrono::high_resolution_clock::time_point start,
    std::chrono::high_resolution_clock::time_point end) {
  auto* client = GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;
  const auto& profile = client->GetProfile();
  int tier = profile.soc_tier;
  if (tier < 0) tier = 0;
  if (tier > 3) tier = 3;

  const TimingRange* ranges = nullptr;
  switch (category) {
    case DelayCategory::kCanvas:  ranges = kCanvasTiming; break;
    case DelayCategory::kWebGL:   ranges = kWebGLTiming; break;
    case DelayCategory::kCrypto:  ranges = kCryptoTiming; break;
    case DelayCategory::kGeneric: ranges = kGenericTiming; break;
  }

  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();

  int target_ms = ComputeDelay(ranges[tier]);

  // Only add remaining delay if operation was faster than target.
  if (elapsed_ms < target_ms) {
    int remaining = target_ms - static_cast<int>(elapsed_ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(remaining));
  }
}

// ====================================================================
// performance.now() micro-jitter (OPTIONAL).
//
// Add ±0.01ms noise to performance.now() to prevent high-resolution
// timing fingerprinting. This is subtle enough to not break anything
// but prevents correlating timing patterns.
//
// CRITICAL: Must maintain monotonicity — each returned value must be
// >= the previous value. FPJS specifically checks for this.
//
// Where to call: third_party/blink/renderer/core/timing/performance.cc
//   → Performance::now()
//
// Return value: jitter in milliseconds (add to the returned timestamp).
// ====================================================================

namespace {

// Thread-local state for monotonicity enforcement.
static thread_local double g_last_jittered_time = 0.0;
static thread_local uint64_t g_jitter_counter = 0;

}  // namespace

double GetPerformanceNowJitter() {
  auto* client = GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 0.0;
  const auto& profile = client->GetProfile();

  // Use canvas_noise_seed for deterministic jitter pattern.
  uint32_t seed_bits = 0;
  std::memcpy(&seed_bits, &profile.canvas_noise_seed, sizeof(seed_bits));

  uint64_t h = static_cast<uint64_t>(seed_bits) ^ (++g_jitter_counter);
  h *= 0x9E3779B97F4A7C15ULL;
  h ^= h >> 32;

  // Convert to +0.001ms to +0.02ms range (always positive to help monotonicity).
  double jitter = 0.001 + (static_cast<double>(h & 0xFFFF) / 65536.0) * 0.019;
  return jitter;
}

// ====================================================================
// Monotonic wrapper — call this from Performance::now() instead of
// directly adding jitter. Guarantees strictly increasing timestamps.
//
// Usage in performance.cc:
//   double Performance::now() const {
//     double raw = /* original monotonic clock calculation */;
//     return blink::EnforceMonotonicJitter(raw);
//   }
// ====================================================================
double EnforceMonotonicJitter(double raw_time_ms) {
  double jittered = raw_time_ms + GetPerformanceNowJitter();

  // Enforce monotonicity: never return less than previous value.
  if (jittered <= g_last_jittered_time) {
    // Advance by a tiny amount (5 microseconds) past last value.
    jittered = g_last_jittered_time + 0.005;
  }
  g_last_jittered_time = jittered;
  return jittered;
}

}  // namespace blink
