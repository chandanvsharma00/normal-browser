// Copyright 2024 Normal Browser Authors. All rights reserved.
// canvas_spoofing.cc — Canvas fingerprint perturbation at the rendering level.
//
// Strategy: Modify text rendering with per-session sub-pixel offsets at the
// Skia level. This produces genuine rendering variation (not post-processing),
// making it indistinguishable from real device differences.
//
// Files to modify in Chromium:
//   third_party/skia/src/core/SkScalerContext.cpp
//   third_party/blink/renderer/platform/fonts/shaping/harfbuzz_shaper.cc
//   third_party/blink/renderer/modules/canvas/canvas2d/
//       canvas_rendering_context_2d.cc

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstring>

namespace {

// =================================================================
// Canvas noise application — modifies glyph sub-pixel positioning.
// Called from SkScalerContext during text rasterization.
// =================================================================

// Deterministic noise from seed + glyph index.
float GetGlyphOffset(float canvas_seed, uint32_t glyph_id, int axis) {
  // Use memcpy to extract bits (avoids strict aliasing violation).
  uint32_t hash = 0;
  memcpy(&hash, &canvas_seed, sizeof(hash));
  hash ^= glyph_id * 0x9E3779B9u;
  hash ^= (axis * 0x517CC1B7u);
  hash = ((hash >> 16) ^ hash) * 0x45D9F3Bu;
  hash = ((hash >> 16) ^ hash) * 0x45D9F3Bu;
  hash = (hash >> 16) ^ hash;

  // Convert to ±0.001 to ±0.01 pixel range.
  float normalized = static_cast<float>(hash & 0xFFFF) / 65536.0f;
  return (normalized - 0.5f) * 0.02f;  // ±0.01 pixels max
}

}  // namespace

// =================================================================
// PATCH 1: SkScalerContext — Glyph position perturbation
// File: third_party/skia/src/core/SkScalerContext.cpp
// Function: SkScalerContext::getGlyphMetrics()
//
// Add after computing glyph advance width:
// =================================================================
/*
  APPLY IN: SkScalerContext::getGlyphMetrics(SkGlyph* glyph)
  AFTER:    glyph->fAdvanceX = ...;
            glyph->fAdvanceY = ...;
  ADD:
*/
void ApplyCanvasNoise_GlyphMetrics(float* advance_x, float* advance_y,
                                    uint32_t glyph_id) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  float seed = client->GetProfile().canvas_noise_seed;
  if (seed == 0.0f) return;

  *advance_x += GetGlyphOffset(seed, glyph_id, 0);
  *advance_y += GetGlyphOffset(seed, glyph_id, 1);
}

// =================================================================
// PATCH 2: Canvas2D — getImageData / toDataURL noise
// File: third_party/blink/renderer/modules/canvas/canvas2d/
//       canvas_rendering_context_2d.cc
//
// Since we perturb the text at render time, toDataURL() and
// getImageData() naturally produce unique output without any
// post-processing. The sub-pixel glyph offsets cause different
// anti-aliasing patterns at the GPU level.
//
// However, for non-text draws (shapes, gradients), we also need
// minor perturbation. Apply gamma shift at the compositing stage.
// =================================================================

// Apply per-session gamma shift to canvas compositing.
// Called from CanvasResourceProvider::ProduceCanvasResource().
void ApplyCanvasNoise_Compositing(uint8_t* pixels,
                                   int width,
                                   int height,
                                   int stride) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  float seed = client->GetProfile().canvas_noise_seed;
  if (seed == 0.0f) return;

  // Apply deterministic 1-bit noise to a sparse selection of pixels.
  uint32_t hash = 0;
  memcpy(&hash, &seed, sizeof(hash));

  for (int y = 0; y < height; y += 17) {      // Every 17th row
    for (int x = 0; x < width; x += 13) {     // Every 13th pixel
      uint32_t idx = y * stride + x * 4;       // RGBA, 4 bytes per pixel
      if (idx + 3 >= static_cast<uint32_t>(height * stride)) break;

      // Deterministic channel selection.
      hash = ((hash >> 16) ^ hash) * 0x45D9F3Bu;
      int channel = hash & 3;       // 0=R, 1=G, 2=B, 3=skip
      if (channel == 3) continue;   // Skip alpha

      int delta = (hash & 4) ? 1 : -1;  // ±1 to color value
      int current = pixels[idx + channel];
      current += delta;
      if (current < 0) current = 0;
      if (current > 255) current = 255;
      pixels[idx + channel] = static_cast<uint8_t>(current);

      hash ^= hash << 5;
    }
  }
}

// =================================================================
// PATCH 3: ClientRect / Element sizing noise
// File: third_party/blink/renderer/core/layout/layout_box.cc
//
// Element.getBoundingClientRect() and getClientRects() are used for
// fingerprinting. Add sub-pixel noise (±0.001px) at layout level.
// =================================================================

// Apply to LayoutBox::PhysicalBorderBoxRect() or equivalent.
void ApplyClientRectNoise(float* x, float* y,
                          float* width, float* height,
                          uint32_t element_hash) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  float seed = client->GetProfile().client_rect_noise_seed;
  if (seed == 0.0f) return;

  // Deterministic per-element noise.
  uint32_t h = 0;
  memcpy(&h, &seed, sizeof(h));
  h ^= element_hash;
  auto noise = [&h]() -> float {
    h = ((h >> 16) ^ h) * 0x45D9F3Bu;
    h = ((h >> 16) ^ h) * 0x45D9F3Bu;
    h = (h >> 16) ^ h;
    return (static_cast<float>(h & 0xFFFF) / 65536.0f - 0.5f) * 0.002f;
  };

  *x += noise();
  *y += noise();
  *width += noise();
  *height += noise();
}
