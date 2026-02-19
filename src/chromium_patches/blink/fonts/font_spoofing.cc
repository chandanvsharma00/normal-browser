// Copyright 2024 Normal Browser Authors. All rights reserved.
// font_spoofing.cc — Font enumeration and emoji font spoofing.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/platform/fonts/font_cache_android.cc
//   third_party/blink/renderer/platform/fonts/font_fallback_list.cc
//   third_party/blink/renderer/platform/fonts/font_platform_data.cc
//   third_party/skia/src/ports/SkFontMgr_android.cpp
//
// STRATEGY:
//   1. Filter font enumeration to only return fonts matching the
//      claimed manufacturer (Samsung fonts for Samsung, etc.).
//   2. Override the system emoji font path based on manufacturer.
//   3. Add sub-pixel font metric noise for canvas fingerprinting.

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

// Stock Android fonts (Motorola, Realme, OPPO, Vivo, Tecno, iQOO, Nothing)
// These OEMs use near-stock Android with no custom system fonts.
const std::vector<std::string> kStockFonts = {
    // No OEM-specific additions — just the common set
};

}  // namespace

// ====================================================================
// GetSpoofedFontList — Returns manufacturer-appropriate font set
// File: third_party/blink/renderer/platform/fonts/font_cache_android.cc
//
// Called when JavaScript tries to enumerate fonts (CSS font-face
// loading, canvas text rendering, etc.)
// ====================================================================
std::vector<std::string> GetSpoofedFontList() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return kCommonFonts;

  const auto& p = client->GetProfile();
  std::vector<std::string> fonts = kCommonFonts;

  // Add manufacturer-specific fonts
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
  // Motorola, Realme, OPPO, Vivo, Tecno, iQOO, Nothing → just common fonts

  return fonts;
}

// ====================================================================
// IsFontAvailable — Check if a specific font "exists" on the device
//
// Called from font matching / CSS @font-face resolution.
// Only returns true for fonts in the profile's allowed list.
// ====================================================================
bool IsFontAvailableForProfile(const std::string& font_family) {
  static std::unordered_set<std::string> allowed_cache;
  static std::string cached_manufacturer;

  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return true;  // Allow all if no profile

  const auto& p = client->GetProfile();

  // Rebuild cache if manufacturer changed (session rotation)
  if (cached_manufacturer != p.manufacturer) {
    allowed_cache.clear();
    auto fonts = GetSpoofedFontList();
    for (const auto& f : fonts) {
      allowed_cache.insert(f);
    }
    cached_manufacturer = p.manufacturer;
  }

  return allowed_cache.count(font_family) > 0;
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
