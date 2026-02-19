// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_validator.h — Validates generated profiles for hardware coherence.

#ifndef NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_VALIDATOR_H_
#define NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_VALIDATOR_H_

#include <string>
#include <vector>

namespace normal_browser {

struct DeviceProfile;  // forward declaration

struct ValidationError {
  std::string field;
  std::string message;
};

// Returns an empty vector if the profile is valid.
// Returns a list of issues if any field is incoherent.
std::vector<ValidationError> ValidateProfile(const DeviceProfile& profile);

// Quick check — returns true if no errors.
bool IsProfileValid(const DeviceProfile& profile);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_PROFILE_VALIDATOR_H_
