// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_validator.cc — Hardware-coherence validation for spoofed profiles.

#include "device_profile_generator/profile_validator.h"

#include <cmath>
#include <string>
#include <vector>

#include "device_profile_generator/device_profile.h"
#include "device_profile_generator/gpu_database.h"
#include "device_profile_generator/manufacturer_traits.h"

namespace normal_browser {

namespace {

void AddError(std::vector<ValidationError>& errors,
              const std::string& field,
              const std::string& msg) {
  errors.push_back({field, msg});
}

}  // namespace

std::vector<ValidationError> ValidateProfile(const DeviceProfile& profile) {
  std::vector<ValidationError> errors;

  // 1. Basic non-empty checks.
  if (profile.manufacturer_name.empty())
    AddError(errors, "manufacturer_name", "Must not be empty");
  if (profile.model.empty())
    AddError(errors, "model", "Must not be empty");
  if (profile.gl_renderer.empty())
    AddError(errors, "gl_renderer", "Must not be empty");
  if (profile.user_agent.empty())
    AddError(errors, "user_agent", "Must not be empty");
  if (profile.build_fingerprint.empty())
    AddError(errors, "build_fingerprint", "Must not be empty");

  // 2. SoC must exist in GPU database (lookup by codename).
  const SoCProfile* soc = FindSoCProfile(profile.soc_codename);
  if (!soc) {
    AddError(errors, "soc_codename",
             "SoC '" + profile.soc_codename + "' not found in GPU database");
  } else {
    // 3. GL renderer must match SoC.
    if (profile.gl_renderer != soc->gl_renderer)
      AddError(errors, "gl_renderer",
               "GL renderer '" + profile.gl_renderer +
                   "' doesn't match SoC expected '" + soc->gl_renderer + "'");

    // 4. GL vendor must match SoC.
    if (profile.gl_vendor != soc->gl_vendor)
      AddError(errors, "gl_vendor",
               "GL vendor mismatch for SoC " + profile.soc_codename);

    // 5. GL max texture size must match SoC.
    if (profile.gl_max_texture_size != soc->gl_max_texture_size)
      AddError(errors, "gl_max_texture_size",
               "Texture size mismatch: profile=" +
                   std::to_string(profile.gl_max_texture_size) +
                   " soc=" + std::to_string(soc->gl_max_texture_size));

    // 6. CPU cores must match SoC.
    if (profile.cpu_cores != soc->cpu_cores)
      AddError(errors, "cpu_cores",
               "CPU cores mismatch: profile=" +
                   std::to_string(profile.cpu_cores) +
                   " soc=" + std::to_string(soc->cpu_cores));

    // 7. SoC tier must match.
    if (profile.soc_tier != soc->soc_tier)
      AddError(errors, "soc_tier",
               "SoC tier mismatch: profile=" +
                   std::to_string(profile.soc_tier) +
                   " soc=" + std::to_string(soc->soc_tier));

    // 8. WebGPU — if SoC doesn't support it, profile must not advertise it.
    if (!soc->supports_webgpu && profile.supports_webgpu)
      AddError(errors, "supports_webgpu",
               "SoC does not support WebGPU but profile claims support");

    // 9. If SoC supports WebGPU, check vendor/arch match.
    if (soc->supports_webgpu && profile.supports_webgpu) {
      if (profile.webgpu_vendor != soc->webgpu_vendor)
        AddError(errors, "webgpu_vendor", "WebGPU vendor mismatch");
      if (profile.webgpu_arch != soc->webgpu_arch)
        AddError(errors, "webgpu_arch", "WebGPU architecture mismatch");
    }
  }

  // 10. SoC↔Manufacturer compatibility check.
  const ManufacturerTraits& traits =
      GetManufacturerTraits(profile.manufacturer);
  bool soc_compatible = false;
  for (const auto& compat : traits.compatible_socs) {
    if (compat.soc_codename == profile.soc_codename) {
      soc_compatible = true;
      break;
    }
  }
  if (!soc_compatible)
    AddError(errors, "soc_codename",
             "SoC '" + profile.soc_codename +
                 "' is not in compatible list for manufacturer '" +
                 profile.manufacturer_name + "'");

  // 11. Android version range.
  if (profile.android_version < 12 || profile.android_version > 15)
    AddError(errors, "android_version",
             "Android version " +
                 std::to_string(profile.android_version) +
                 " outside supported range [12,15]");

  // 12. SDK must match Android version.
  int expected_sdk;
  switch (profile.android_version) {
    case 12: expected_sdk = 31; break;
    case 13: expected_sdk = 33; break;
    case 14: expected_sdk = 34; break;
    case 15: expected_sdk = 35; break;
    default: expected_sdk = 34; break;
  }
  if (profile.sdk_version != expected_sdk)
    AddError(errors, "sdk_version",
             "SDK " + std::to_string(profile.sdk_version) +
                 " doesn't match Android " +
                 std::to_string(profile.android_version));

  // 13. Screen resolution sanity.
  if (profile.screen_width < 720 || profile.screen_width > 3200)
    AddError(errors, "screen_width", "Outside realistic range [720, 3200]");
  if (profile.screen_height < 1280 || profile.screen_height > 3200)
    AddError(errors, "screen_height", "Outside realistic range [1280, 3200]");
  if (profile.screen_height < profile.screen_width)
    AddError(errors, "screen_height", "Height should be >= width (portrait)");

  // 14. DPI must be consistent with density.
  int expected_dpi = static_cast<int>(profile.density * 160.0f);
  if (std::abs(profile.dpi - expected_dpi) > 5)
    AddError(errors, "dpi",
             "DPI " + std::to_string(profile.dpi) +
                 " inconsistent with density " +
                 std::to_string(profile.density));

  // 15. RAM sanity by tier.
  int min_ram_mb[] = {8 * 1024, 6 * 1024, 4 * 1024, 3 * 1024};
  int max_ram_mb[] = {16 * 1024, 12 * 1024, 8 * 1024, 6 * 1024};
  int tier = profile.soc_tier;
  if (tier >= 0 && tier <= 3) {
    if (profile.ram_mb < min_ram_mb[tier] || profile.ram_mb > max_ram_mb[tier])
      AddError(errors, "ram_mb",
               "RAM " + std::to_string(profile.ram_mb) +
                   "MB outside expected range for tier " +
                   std::to_string(tier));
  }

  // 16. Battery level range.
  if (profile.battery_level < 0.0f || profile.battery_level > 1.0f)
    AddError(errors, "battery_level", "Battery must be in [0, 1]");

  // 17. Fingerprint seeds should be non-zero.
  if (profile.canvas_noise_seed == 0.0f)
    AddError(errors, "canvas_noise_seed", "Should be non-zero for uniqueness");
  if (profile.audio_bias == 0.0f)
    AddError(errors, "audio_bias", "Should be non-zero for uniqueness");

  // 18. GL extensions must not be empty for any SoC.
  if (profile.gl_extensions.empty())
    AddError(errors, "gl_extensions", "Must have GL extensions");

  // 19. User-Agent must contain Android version and Chrome.
  if (profile.user_agent.find("Chrome/") == std::string::npos)
    AddError(errors, "user_agent", "Missing Chrome/ in User-Agent");
  if (profile.user_agent.find("Android") == std::string::npos)
    AddError(errors, "user_agent", "Missing Android in User-Agent");

  // 20. Build fingerprint format check.
  if (profile.build_fingerprint.find("/") == std::string::npos ||
      profile.build_fingerprint.find(":") == std::string::npos)
    AddError(errors, "build_fingerprint", "Malformed fingerprint format");

  return errors;
}

bool IsProfileValid(const DeviceProfile& profile) {
  return ValidateProfile(profile).empty();
}

}  // namespace normal_browser
