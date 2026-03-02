// Copyright 2024 Normal Browser Authors. All rights reserved.
// font_spoofing.cc — Font enumeration and emoji font spoofing.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/platform/fonts/font_cache_android.cc
//     → #include this file
//     → In FontCache::GetFontPlatformData(), BEFORE the font lookup:
//         if (blink::ShouldBlockFontFamily(family_name.Utf8()))
//           return nullptr;  // Force fallback
//
//   third_party/blink/renderer/platform/fonts/font_fallback_list.cc
//     → #include this file
//     → In DeterminePrimarySimpleFontData(), after getting family name:
//         if (blink::ShouldBlockFontFamily(family.Utf8()))
//           return GetLastResortFallbackFont();  // Use fallback
//
//   third_party/skia/src/ports/SkFontMgr_android.cpp
//     → In onMatchFamilyStyle(), check ShouldBlockFontFamily() first
//
// WHY THIS IS CRITICAL:
//   FPJS detects fonts by RENDERING test strings (not enumeration API).
//   It sets CSS font-family to a probe font + fallback, measures
//   the rendered text width, and compares. If widths differ, the font
//   exists. To prevent detection of fonts we don't want to expose,
//   we must block them at the FONT MATCHING level so they fall back
//   to the default font → same width → FPJS thinks font doesn't exist.
//
//   The FPJS log showed `"fonts": ["sans-serif-thin"]` because this
//   was the only font that rendered differently from fallback. All
//   other fonts either don't exist on the device or were already
//   matching the fallback width. Blocking `sans-serif-thin` eliminates
//   this fingerprint signal.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

namespace blink {

// ====================================================================
// Font Database: Per-manufacturer font lists
// ====================================================================
namespace {

// Common fonts present on ALL Android devices
const std::vector<std::string> kCommonFonts = {
    "Roboto",
    "Roboto Condensed",
    "NotoSansCJK",
    "NotoColorEmoji",
    "NotoSansDevanagari",
    "NotoSansBengali",
    "NotoSansTamil",
    "NotoSansTelugu",
    "NotoSansKannada",
    "NotoSansMalayalam",
    "NotoSansGujarati",
    "NotoSansGurmukhi",
    "NotoSansOriya",
    "NotoSansSinhala",
    "NotoSansMyanmar",
    "NotoSansThai",
    "NotoSansArabic",
    "NotoSansHebrew",
    "NotoSansArmenian",
    "NotoSansGeorgian",
    "NotoSansEthiopic",
    "NotoSansSymbols",
    "NotoSansSymbols2",
    "NotoSerif",
    "DroidSansFallback",
    "DroidSansMono",
    "Courier New",
    "Georgia",
    "Verdana",
    "Times New Roman",
    "Comic Sans MS",
    "serif",
    "sans-serif",
    "monospace",
    "cursive",
    "fantasy",
};

// Samsung-exclusive fonts
const std::vector<std::string> kSamsungFonts = {
    "SamsungOne",
    "SamsungSans",
    "SECRobotoLight",
    "SECRoboto",
    "SamsungOneUI",
    "SamsungColorEmoji",
    "GothicA1",
};

// Xiaomi-exclusive fonts
const std::vector<std::string> kXiaomiFonts = {
    "MiSans",
    "MiSans VF",
    "MiSansLatin",
    "MiLanPro",
    "Clock",
};

// OnePlus-exclusive fonts
const std::vector<std::string> kOnePlusFonts = {
    "OnePlusSans",
    "OnePlusSansMono",
    "OnePlusSansDisplay",
    "SlateForOnePlus",
};

// Google Pixel-exclusive fonts
const std::vector<std::string> kGoogleFonts = {
    "Google Sans",
    "Google Sans Display",
    "Google Sans Text",
    "Product Sans",
    "CutiveMono",
    "DancingScript",
    "CarroisGothicSC",
    "ComingSoon",
};

// Fonts that MUST be blocked because they leak device identity.
// These exist on real Android devices but should not be detectable
// because they create a constant fingerprint across all profiles.
const std::vector<std::string> kBlockedFonts = {
    "sans-serif-thin",        // Android-specific, FPJS probes this
    "sans-serif-light",       // Android-specific weight variant
    "sans-serif-medium",      // Android-specific weight variant
    "sans-serif-black",       // Android-specific weight variant
    "sans-serif-condensed",   // Android-specific condensed variant
    "sans-serif-smallcaps",   // Android-specific variant
    "serif-monospace",        // Android system alias
    "casual",                 // Android system alias
    "elegant",                // Android-specific alias
};

// Generic CSS families that must always resolve (never block)
const std::unordered_set<std::string> kGenericFamilies = {
    "serif", "sans-serif", "monospace", "cursive", "fantasy",
    "system-ui", "math", "emoji", "fangsong",
};

// Build the allowed font set for the current profile
void BuildAllowedFontCache(std::unordered_set<std::string>& cache,
                           const std::string& manufacturer) {
  cache.clear();

  // Add common fonts
  for (const auto& f : kCommonFonts) {
    cache.insert(f);
  }

  // Add generic families
  for (const auto& f : kGenericFamilies) {
    cache.insert(f);
  }

  // Add manufacturer-specific fonts
  if (manufacturer == "Samsung") {
    for (const auto& f : kSamsungFonts) cache.insert(f);
  } else if (manufacturer == "Xiaomi" || manufacturer == "Redmi" ||
             manufacturer == "POCO") {
    for (const auto& f : kXiaomiFonts) cache.insert(f);
  } else if (manufacturer == "OnePlus") {
    for (const auto& f : kOnePlusFonts) cache.insert(f);
  } else if (manufacturer == "Google") {
    for (const auto& f : kGoogleFonts) cache.insert(f);
  }
}

}  // namespace

// ====================================================================
// GetSpoofedFontList — Returns manufacturer-appropriate font set
// ====================================================================
std::vector<std::string> GetSpoofedFontList() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return kCommonFonts;

  const auto& p = client->GetProfile();
  std::vector<std::string> fonts = kCommonFonts;

  if (p.manufacturer == "Samsung") {
    fonts.insert(fonts.end(), kSamsungFonts.begin(), kSamsungFonts.end());
  } else if (p.manufacturer == "Xiaomi" ||
             p.manufacturer == "Redmi" ||
             p.manufacturer == "POCO") {
    fonts.insert(fonts.end(), kXiaomiFonts.begin(), kXiaomiFonts.end());
  } else if (p.manufacturer == "OnePlus") {
    fonts.insert(fonts.end(), kOnePlusFonts.begin(), kOnePlusFonts.end());
  } else if (p.manufacturer == "Google") {
    fonts.insert(fonts.end(), kGoogleFonts.begin(), kGoogleFonts.end());
  }

  return fonts;
}

// ====================================================================
// ShouldBlockFontFamily — THE PRIMARY HOOK FUNCTION
//
// This is the main function that MUST be called from the font matching
// path. When it returns true, the font matcher should skip this family
// and use the fallback instead.
//
// HOOK POINT (font_cache_android.cc):
//   const SimpleFontData* FontCache::GetFontPlatformData(...) {
//     // --- Normal Browser: block non-profile fonts ---
//     if (blink::ShouldBlockFontFamily(
//             description.Family().FamilyName().Utf8())) {
//       return nullptr;
//     }
//     // --- original code continues ---
//   }
//
// This prevents FPJS from detecting fonts via rendering measurement.
// ====================================================================
bool ShouldBlockFontFamily(const std::string& font_family) {
  // Never block empty names
  if (font_family.empty()) return false;

  // Never block generic CSS families
  if (kGenericFamilies.count(font_family) > 0) return false;

  // Always block known fingerprint-leaking fonts
  for (const auto& blocked : kBlockedFonts) {
    if (font_family == blocked) return true;
  }

  // Check against profile's allowed list
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;  // Allow all if no profile

  static std::unordered_set<std::string> allowed_cache;
  static std::string cached_manufacturer;

  const auto& p = client->GetProfile();

  if (cached_manufacturer != p.manufacturer) {
    BuildAllowedFontCache(allowed_cache, p.manufacturer);
    cached_manufacturer = p.manufacturer;
  }

  // If the font is in our allowed list, don't block
  if (allowed_cache.count(font_family) > 0) return false;

  // Font not in allowed list → block it (force fallback)
  return true;
}

// ====================================================================
// IsFontAvailableForProfile — legacy API, wraps ShouldBlockFontFamily
// Returns true if font should be available, false if blocked.
// ====================================================================
bool IsFontAvailableForProfile(const std::string& font_family) {
  return !ShouldBlockFontFamily(font_family);
}

// ====================================================================
// GetSystemEmojiFontPath — Returns emoji font for the claimed OEM
// File: third_party/skia/src/ports/SkFontMgr_android.cpp
//
// Samsung → SamsungColorEmoji.ttf  (bundled in APK assets)
// Others  → NotoColorEmoji.ttf      (Android system font)
// ====================================================================
std::string GetSpoofedEmojiFontPath() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return "/system/fonts/NotoColorEmoji.ttf";

  const auto& p = client->GetProfile();
  if (p.manufacturer == "Samsung") {
    // Bundled Samsung emoji in our APK's assets directory
    return "/data/data/com.nicebrowser.app/files/fonts/SamsungColorEmoji.ttf";
  }
  // All other manufacturers use Google Noto Emoji
  return "/system/fonts/NotoColorEmoji.ttf";
}

// ====================================================================
// GetSystemUIFontFamily — Returns the default system font
// File: third_party/blink/renderer/platform/fonts/font_cache_android.cc
//
// Samsung → SamsungOne
// Xiaomi  → MiSans (MIUI 13+)
// OnePlus → OnePlusSans
// Others  → Roboto (stock Android)
// ====================================================================
std::string GetSpoofedSystemFontFamily() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return "Roboto";

  const auto& p = client->GetProfile();
  if (p.manufacturer == "Samsung") return "SamsungOne";
  if (p.manufacturer == "Xiaomi" || p.manufacturer == "Redmi" ||
      p.manufacturer == "POCO") return "MiSans";
  if (p.manufacturer == "OnePlus") return "OnePlusSans";
  if (p.manufacturer == "Google") return "Google Sans";
  return "Roboto";
}

// ====================================================================
// Font Metrics Noise — Sub-pixel variations per session
// File: third_party/blink/renderer/platform/fonts/shaping/
//       harfbuzz_shaper.cc  OR  font_platform_data.cc
//
// Different devices produce slightly different glyph advance widths
// due to font hinting, rendering engine differences, etc.
// We inject per-session noise into advance widths.
// ====================================================================
namespace {

uint64_t FontMetricHash(uint64_t seed, uint32_t glyph_id, uint32_t font_id) {
  uint64_t h = seed ^ (static_cast<uint64_t>(glyph_id) * 0x9E3779B97F4A7C15ULL);
  h ^= static_cast<uint64_t>(font_id) * 0x517CC1B727220A95ULL;
  h ^= h >> 33;
  h *= 0xFF51AFD7ED558CCDULL;
  h ^= h >> 33;
  h *= 0xC4CEB9FE1A85EC53ULL;
  h ^= h >> 33;
  return h;
}

}  // namespace

// Apply sub-pixel noise to glyph advance width.
// magnitude: ±0.002px (imperceptible but changes layout hash)
float ApplyFontMetricNoise(float advance_width,
                           uint32_t glyph_id,
                           uint32_t font_id) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return advance_width;

  uint64_t seed = 0;
  {
    float fs = client->GetProfile().canvas_noise_seed;
    uint32_t bits = 0;
    memcpy(&bits, &fs, sizeof(bits));
    seed = static_cast<uint64_t>(bits);
  }
  if (seed == 0) return advance_width;

  uint64_t h = FontMetricHash(seed, glyph_id, font_id);
  // Map to [-1, +1]
  double noise_norm = static_cast<double>(h & 0xFFFFFFFF) /
                      static_cast<double>(0xFFFFFFFF) * 2.0 - 1.0;
  // ±0.002 pixel noise
  float noise = static_cast<float>(noise_norm * 0.002);
  return advance_width + noise;
}

// Apply noise to font ascent/descent metrics for layout fingerprinting
float ApplyFontAscentNoise(float ascent, uint32_t font_id) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return ascent;

  uint64_t seed = 0;
  {
    float fs = client->GetProfile().canvas_noise_seed;
    uint32_t bits = 0;
    memcpy(&bits, &fs, sizeof(bits));
    seed = static_cast<uint64_t>(bits);
  }
  if (seed == 0) return ascent;

  uint64_t h = FontMetricHash(seed, 0xA5C07, font_id);
  double noise_norm = static_cast<double>(h & 0xFFFFFFFF) /
                      static_cast<double>(0xFFFFFFFF) * 2.0 - 1.0;
  return ascent + static_cast<float>(noise_norm * 0.001);
}

}  // namespace blink
