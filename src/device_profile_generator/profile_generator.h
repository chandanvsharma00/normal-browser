// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_generator.h — Main 14-step device profile generation engine.

#ifndef NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_GENERATOR_H_
#define NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_GENERATOR_H_

#include <cstdint>

#include "device_profile_generator/device_profile.h"

namespace normal_browser {

// Generates a fully populated DeviceProfile.
// |master_seed|: If 0, a cryptographically random seed is created.
// Non-zero seeds produce deterministic profiles (useful for testing).
DeviceProfile GenerateDeviceProfile(uint64_t master_seed = 0);

// Re-generate only session-varying fields (TLS, noise seeds) keeping the
// same device identity. Used when user wants to keep device but rotate
// fingerprints.
void RotateSessionSeeds(DeviceProfile& profile);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_GENERATOR_H_
