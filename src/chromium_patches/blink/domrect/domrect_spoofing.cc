// Copyright 2024 Normal Browser Authors. All rights reserved.
// domrect_spoofing.cc — DOMRect/ClientRect noise for fingerprint variation.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/core/dom/element.cc
//     → In Element::getBoundingClientRect(), apply noise to the result:
//       #include "third_party/blink/renderer/core/dom/domrect_spoofing.cc"
//       After computing the DOMRect, call:
//         blink::domrect_spoofing::ApplyDOMRectNoise(rect);
//
//   third_party/blink/renderer/core/dom/range.cc
//     → In Range::getBoundingClientRect(), same pattern.
//
// WHY THIS IS NEEDED:
//   FPJS measures emoji rendering (and other text/element) dimensions via
//   getBoundingClientRect(). Since all Android devices use the same
//   NotoColorEmoji.ttf, the bounding rect is identical across devices.
//   Per-profile noise (±0.001px) makes measurements unique per session
//   without visible rendering changes.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cmath>
#include <cstring>
#include <string>

namespace blink {
namespace domrect_spoofing {

namespace {

// Deterministic hash for noise generation — same inputs always produce
// same noise, ensuring consistent measurements within a single session.
uint64_t DOMRectHash(uint64_t seed, double x, double y, double w, double h) {
  // Mix the rect dimensions into the seed
  uint64_t hx, hy, hw, hh;
  memcpy(&hx, &x, sizeof(hx));
  memcpy(&hy, &y, sizeof(hy));
  memcpy(&hw, &w, sizeof(hw));
  memcpy(&hh, &h, sizeof(hh));

  uint64_t hash = seed;
  hash ^= hx * 0x9E3779B97F4A7C15ULL;
  hash ^= hash >> 33;
  hash *= 0xFF51AFD7ED558CCDULL;
  hash ^= hy * 0x517CC1B727220A95ULL;
  hash ^= hash >> 33;
  hash *= 0xC4CEB9FE1A85EC53ULL;
  hash ^= hw * 0x6C62272E07BB0142ULL;
  hash ^= hash >> 33;
  hash *= 0xFF51AFD7ED558CCDULL;
  hash ^= hh * 0x94D049BB133111EBULL;
  hash ^= hash >> 33;
  return hash;
}

// Convert hash to a tiny noise value in range [-magnitude, +magnitude]
double HashToNoise(uint64_t hash, double magnitude) {
  // Use lower 32 bits to generate [-1, +1] range
  double norm = static_cast<double>(hash & 0xFFFFFFFF) /
                static_cast<double>(0xFFFFFFFF) * 2.0 - 1.0;
  return norm * magnitude;
}

}  // namespace

// ====================================================================
// ApplyDOMRectNoise — modifies x, y, width, height by ±0.001px
//
// The noise is:
//   - Deterministic: same element + same profile → same noise
//   - Tiny: ±0.001px is sub-pixel, invisible to users
//   - Unique: different profiles produce different noise
//   - Consistent: calling getBoundingClientRect() twice on the same
//     element returns the same noised values (deterministic hash)
//
// Parameters are modified in-place.
// ====================================================================
void ApplyDOMRectNoise(double& x, double& y,
                       double& width, double& height) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  float seed_f = client->GetProfile().client_rect_noise_seed;
  if (seed_f == 0.0f) return;

  // Convert float seed to uint64_t for hashing
  uint32_t seed_bits;
  memcpy(&seed_bits, &seed_f, sizeof(seed_bits));
  uint64_t seed = static_cast<uint64_t>(seed_bits) * 0x9E3779B97F4A7C15ULL;

  // Generate deterministic noise based on the original rect values
  uint64_t h = DOMRectHash(seed, x, y, width, height);

  // Apply sub-pixel noise to each dimension
  // Magnitude: 0.001px — imperceptible but changes fingerprint hash
  constexpr double kMagnitude = 0.001;

  x += HashToNoise(h, kMagnitude);
  y += HashToNoise(h >> 8, kMagnitude);
  width += HashToNoise(h >> 16, kMagnitude);
  height += HashToNoise(h >> 24, kMagnitude);
}

// ====================================================================
// ApplyDOMRectNoiseToTop/Right/Bottom/Left — for getClientRects()
// which returns DOMRectList with individual rects.
// ====================================================================
void ApplyDOMRectListNoise(double& x, double& y,
                           double& width, double& height,
                           int index) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  float seed_f = client->GetProfile().client_rect_noise_seed;
  if (seed_f == 0.0f) return;

  uint32_t seed_bits;
  memcpy(&seed_bits, &seed_f, sizeof(seed_bits));
  uint64_t seed = static_cast<uint64_t>(seed_bits) * 0x9E3779B97F4A7C15ULL;
  // Mix in the rect index for per-rect uniqueness
  seed ^= static_cast<uint64_t>(index) * 0x517CC1B727220A95ULL;

  uint64_t h = DOMRectHash(seed, x, y, width, height);

  constexpr double kMagnitude = 0.001;
  x += HashToNoise(h, kMagnitude);
  y += HashToNoise(h >> 8, kMagnitude);
  width += HashToNoise(h >> 16, kMagnitude);
  height += HashToNoise(h >> 24, kMagnitude);
}

}  // namespace domrect_spoofing
}  // namespace blink
