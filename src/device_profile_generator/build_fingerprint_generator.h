// Copyright 2024 Normal Browser Authors. All rights reserved.
// build_fingerprint_generator.h — AOSP Build ID / fingerprint generation.

#ifndef NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_BUILD_FINGERPRINT_GENERATOR_H_
#define NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_BUILD_FINGERPRINT_GENERATOR_H_

#include <cstdint>
#include <string>

namespace normal_browser {

struct BuildInfo {
  int android_version;       // 12, 13, 14, 15
  int sdk_version;           // 31, 33, 34, 35
  std::string build_id;      // e.g. "UP1A.231005.007"
  std::string security_patch; // e.g. "2025-06-01"
  std::string fingerprint;   // Full Build.FINGERPRINT
  std::string display;       // Build.DISPLAY
  std::string incremental;   // Build.VERSION.INCREMENTAL
  std::string release;       // Build.VERSION.RELEASE  ("14", "15", etc.)
};

// Generates a complete BuildInfo.
// |manufacturer| : "Samsung", "Xiaomi", etc.
// |model|        : "SM-S936B", "RMX3780", etc.
// |device_codename| : "s936b", "RMX3780", etc.
// |seed|         : deterministic random seed.
BuildInfo GenerateBuildInfo(const std::string& manufacturer,
                            const std::string& model,
                            const std::string& device_codename,
                            uint64_t seed);

// Generate only the security_patch string (within last 3 months).
std::string GenerateSecurityPatch(uint64_t seed);

// Generate the AOSP build ID (e.g. "UP1A.240305.019").
std::string GenerateBuildId(int android_version, uint64_t seed);

// Generate android_id (16-char hex).
std::string GenerateAndroidId(uint64_t seed);

// Generate serial number in OEM format.
std::string GenerateSerialNumber(const std::string& manufacturer,
                                 uint64_t seed);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_BUILD_FINGERPRINT_GENERATOR_H_
