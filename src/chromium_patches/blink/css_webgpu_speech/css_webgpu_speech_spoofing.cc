// Copyright 2024 Normal Browser Authors. All rights reserved.
// css_webgpu_speech_spoofing.cc — CSS media queries, WebGPU adapter,
//                                  and SpeechSynthesis voice spoofing.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/core/css/media_query_evaluator.cc
//   third_party/blink/renderer/modules/webgpu/gpu_adapter.cc
//   third_party/blink/renderer/modules/webgpu/gpu.cc
//   third_party/blink/renderer/modules/speech/speech_synthesis.cc

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <string>
#include <vector>

namespace blink {

// ====================================================================
// PART A: CSS Media Query Spoofing
// File: third_party/blink/renderer/core/css/media_query_evaluator.cc
// ====================================================================

// matchMedia('(hover: hover)') — mobile = "none"
// FPJS checks this to verify mobile claim.
bool GetSpoofedHoverCapability() {
  // Mobile Android → hover: none (no mouse pointer)
  return false;
}

// matchMedia('(pointer: fine)') — mobile = "coarse"
// fine = mouse, coarse = finger/touch
std::string GetSpoofedPointerType() {
  return "coarse";
}

// matchMedia('(any-pointer: fine)') — mobile = "coarse"
std::string GetSpoofedAnyPointerType() {
  return "coarse";
}

// matchMedia('(any-hover: hover)') — mobile = "none"
bool GetSpoofedAnyHoverCapability() {
  return false;
}

// matchMedia('(prefers-color-scheme: dark)')
// Randomized per profile: ~60% dark, ~40% light (matches Indian user stats)
bool GetSpoofedPrefersColorScheme() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return true;  // Default: dark
  return client->GetProfile().prefers_dark_mode;
}

// matchMedia('(prefers-reduced-motion: reduce)')
// Most users: no-preference. 5% have reduce.
bool GetSpoofedPrefersReducedMotion() {
  // Most users: no-preference (false). Only ~5% have reduce.
  // We always return false since RendererProfile doesn't track this.
  return false;
}

// matchMedia('(prefers-contrast: more)')
// Most users: no-preference.
std::string GetSpoofedPrefersContrast() {
  return "no-preference";
}

// matchMedia('(forced-colors: active)')
// Always none on Android.
bool GetSpoofedForcedColors() {
  return false;
}

// matchMedia('(color-gamut: p3)')
// Flagship AMOLED: P3, Budget LCD: sRGB
bool GetSpoofedColorGamutP3() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;
  return client->GetProfile().supports_p3_gamut;
}

// matchMedia('(color-gamut: srgb)') — always true on Android
bool GetSpoofedColorGamutSRGB() {
  return true;
}

// matchMedia('(dynamic-range: high)') — HDR support
// Flagship: true, Budget: false
bool GetSpoofedDynamicRangeHigh() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;
  // HDR support correlates with flagship/upper-mid AMOLED displays.
  // soc_tier: 0=flagship, 1=upper-mid → HDR; 2=mid, 3=budget → no HDR.
  return client->GetProfile().soc_tier <= 1;
}

// matchMedia('(display-mode: standalone)') — PWA detection
// Always browser mode (not installed as PWA).
std::string GetSpoofedDisplayMode() {
  return "browser";
}

// matchMedia('(inverted-colors: inverted)')
// Always: none.
bool GetSpoofedInvertedColors() {
  return false;
}

// Screen color depth for CSS media queries
int GetSpoofedColorIndex() {
  return 24;  // 24-bit color on Android
}

// matchMedia('(max-resolution: Xdppx)')
// Returns spoofed density as dppx value (e.g. 2.75 → 2.75dppx)
float GetSpoofedMaxResolutionDppx() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 2.625f;  // safe default
  return client->GetProfile().density;
}

// ====================================================================
// PART B: WebGPU Adapter Info Spoofing
// File: third_party/blink/renderer/modules/webgpu/gpu_adapter.cc
//       third_party/blink/renderer/modules/webgpu/gpu.cc
// ====================================================================

struct SpoofedWebGPUAdapterInfo {
  std::string vendor;        // "qualcomm", "arm", "google", "samsung"
  std::string architecture;  // "adreno-700", "valhall", "mali-g7xx"
  std::string device;        // "adreno-750", "immortalis-g720"
  std::string description;   // Full description string
  bool is_available;          // Budget SoCs don't support WebGPU
};

SpoofedWebGPUAdapterInfo GetSpoofedWebGPUInfo() {
  auto* client = normal_browser::GhostProfileClient::Get();
  SpoofedWebGPUAdapterInfo info;

  if (!client || !client->IsReady()) {
    // Default: high-end Qualcomm
    info.vendor = "qualcomm";
    info.architecture = "adreno-700";
    info.device = "adreno-740";
    info.description = "Qualcomm Adreno 740";
    info.is_available = true;
    return info;
  }

  const auto& p = client->GetProfile();

  info.vendor = p.webgpu_vendor;
  info.architecture = p.webgpu_arch;
  info.device = p.webgpu_device;
  info.description = p.webgpu_description;
  info.is_available = p.supports_webgpu;

  return info;
}

// navigator.gpu availability check
// Budget SoCs (Helio G36, Snapdragon 4 Gen 1, etc.) → WebGPU disabled
bool IsWebGPUAvailableForProfile() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return true;
  return client->GetProfile().supports_webgpu;
}

// WebGPU feature limits — constrain to claimed GPU's capabilities
struct SpoofedWebGPULimits {
  uint32_t max_texture_dimension_1d;
  uint32_t max_texture_dimension_2d;
  uint32_t max_texture_dimension_3d;
  uint32_t max_texture_array_layers;
  uint32_t max_bind_groups;
  uint32_t max_storage_buffer_binding_size;
  uint32_t max_buffer_size;
  uint32_t max_compute_workgroup_size_x;
  uint32_t max_compute_workgroup_size_y;
  uint32_t max_compute_workgroup_size_z;
  uint32_t max_compute_invocations_per_workgroup;
};

SpoofedWebGPULimits GetSpoofedWebGPULimits() {
  auto* client = normal_browser::GhostProfileClient::Get();
  SpoofedWebGPULimits limits;

  // soc_tier: 0=flagship, 1=upper-mid, 2=mid, 3=budget
  int soc_tier = 2;  // Default: mid
  if (client && client->IsReady()) {
    soc_tier = client->GetProfile().soc_tier;
  }

  if (soc_tier == 0) {
    // Flagship
    limits.max_texture_dimension_1d = 16384;
    limits.max_texture_dimension_2d = 16384;
    limits.max_texture_dimension_3d = 2048;
    limits.max_texture_array_layers = 2048;
    limits.max_bind_groups = 4;
    limits.max_storage_buffer_binding_size = 134217728;  // 128MB
    limits.max_buffer_size = 268435456;                  // 256MB
    limits.max_compute_workgroup_size_x = 1024;
    limits.max_compute_workgroup_size_y = 1024;
    limits.max_compute_workgroup_size_z = 64;
    limits.max_compute_invocations_per_workgroup = 1024;
  } else if (soc_tier == 1) {
    // Upper-mid
    limits.max_texture_dimension_1d = 8192;
    limits.max_texture_dimension_2d = 8192;
    limits.max_texture_dimension_3d = 2048;
    limits.max_texture_array_layers = 1024;
    limits.max_bind_groups = 4;
    limits.max_storage_buffer_binding_size = 67108864;   // 64MB
    limits.max_buffer_size = 134217728;                  // 128MB
    limits.max_compute_workgroup_size_x = 512;
    limits.max_compute_workgroup_size_y = 512;
    limits.max_compute_workgroup_size_z = 64;
    limits.max_compute_invocations_per_workgroup = 512;
  } else {
    // Mid / Budget — WebGPU might not even be available
    limits.max_texture_dimension_1d = 4096;
    limits.max_texture_dimension_2d = 4096;
    limits.max_texture_dimension_3d = 1024;
    limits.max_texture_array_layers = 256;
    limits.max_bind_groups = 4;
    limits.max_storage_buffer_binding_size = 33554432;   // 32MB
    limits.max_buffer_size = 67108864;                   // 64MB
    limits.max_compute_workgroup_size_x = 256;
    limits.max_compute_workgroup_size_y = 256;
    limits.max_compute_workgroup_size_z = 64;
    limits.max_compute_invocations_per_workgroup = 256;
  }

  return limits;
}

// ====================================================================
// PART C: SpeechSynthesis Voice Spoofing
// File: third_party/blink/renderer/modules/speech/speech_synthesis.cc
// ====================================================================

struct SpoofedVoice {
  std::string name;
  std::string lang;
  bool is_default;
  bool is_local;
};

// Return manufacturer-appropriate TTS voice list
std::vector<SpoofedVoice> GetSpoofedVoiceList() {
  auto* client = normal_browser::GhostProfileClient::Get();
  std::vector<SpoofedVoice> voices;

  std::string manufacturer = "Generic";
  if (client && client->IsReady()) {
    manufacturer = client->GetProfile().manufacturer;
  }

  if (manufacturer == "Samsung") {
    // Samsung has Samsung TTS + Google TTS
    voices.push_back({"Samsung English (United States)", "en-US", false, true});
    voices.push_back({"Samsung English (United Kingdom)", "en-GB", false, true});
    voices.push_back({"Samsung English (India)", "en-IN", false, true});
    voices.push_back(
        {"Samsung \xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80 (\xe0\xa4\xad\xe0\xa4\xbe\xe0\xa4\xb0\xe0\xa4\xa4)",
         "hi-IN", false, true});  // Samsung Hindi
    voices.push_back({"Google English (India)", "en-IN", true, true});
    voices.push_back(
        {"Google \xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80",
         "hi-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa6\xac\xe0\xa6\xbe\xe0\xa6\x82\xe0\xa6\xb2\xe0\xa6\xbe",
         "bn-IN", false, true});
    voices.push_back({"Google English (United States)", "en-US", false, true});
    voices.push_back({"Google English (United Kingdom)", "en-GB", false, true});
    voices.push_back(
        {"Google \xe0\xa4\xa4\xe0\xa4\xae\xe0\xa4\xbf\xe0\xa4\xb3",
         "ta-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa4\xa4\xe0\xa5\x87\xe0\xa4\xb2\xe0\xa5\x81\xe0\xa4\x97\xe0\xa5\x81",
         "te-IN", false, true});
  } else {
    // All other manufacturers: Google TTS only (stock Android)
    voices.push_back({"Google English (India)", "en-IN", true, true});
    voices.push_back({"Google English (United States)", "en-US", false, true});
    voices.push_back({"Google English (United Kingdom)", "en-GB", false, true});
    voices.push_back(
        {"Google \xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x82\xe0\xa4\xa6\xe0\xa5\x80",
         "hi-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa6\xac\xe0\xa6\xbe\xe0\xa6\x82\xe0\xa6\xb2\xe0\xa6\xbe",
         "bn-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa4\xa4\xe0\xa4\xae\xe0\xa4\xbf\xe0\xa4\xb3",
         "ta-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa4\xa4\xe0\xa5\x87\xe0\xa4\xb2\xe0\xa5\x81\xe0\xa4\x97\xe0\xa5\x81",
         "te-IN", false, true});
    voices.push_back(
        {"Google \xe0\xa4\x95\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa8\xe0\xa4\xa1",
         "kn-IN", false, true});

    // Xiaomi adds Mi AI voice on newer MIUI
    if (manufacturer == "Xiaomi" || manufacturer == "Redmi" ||
        manufacturer == "POCO") {
      voices.push_back({"Mi AI English", "en-US", false, true});
    }
  }

  return voices;
}

}  // namespace blink
