// Copyright 2024 Normal Browser Authors. All rights reserved.
// model_name_generator.h — Per-OEM model number + marketing name generator.

#ifndef NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_MODEL_NAME_GENERATOR_H_
#define NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_MODEL_NAME_GENERATOR_H_

#include <string>

#include "device_profile_generator/device_profile.h"

namespace normal_browser {

// Uses Manufacturer enum from device_profile.h.

struct GeneratedModel {
  std::string model_number;     // e.g. "SM-S936B"
  std::string marketing_name;   // e.g. "Galaxy S24 Ultra"
  std::string device_codename;  // e.g. "s936b"
};

// |tier|: 0=flagship, 1=upper-mid, 2=mid, 3=budget
// |seed|: deterministic random seed
GeneratedModel GenerateModelName(Manufacturer manufacturer,
                                 int tier,
                                 uint64_t seed);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_DEVICE_PROFILE_GENERATOR_MODEL_NAME_GENERATOR_H_
