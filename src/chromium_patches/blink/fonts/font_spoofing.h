// Copyright 2024 Normal Browser Authors. All rights reserved.
// font_spoofing.h — Font enumeration and emoji font spoofing declarations.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SPOOFING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SPOOFING_H_

#include <cstdint>
#include <string>
#include <vector>

namespace blink {

std::vector<std::string> GetSpoofedFontList();
bool IsFontAvailableForProfile(const std::string& font_family);
std::string GetSpoofedEmojiFontPath();
std::string GetSpoofedSystemFontFamily();
bool ShouldBlockFontFamily(const std::string& family_name);

float ApplyFontMetricNoise(float advance_width,
                           uint32_t glyph_id,
                           uint32_t font_id);
float ApplyFontAscentNoise(float ascent, uint32_t font_id);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_FONTS_FONT_SPOOFING_H_
