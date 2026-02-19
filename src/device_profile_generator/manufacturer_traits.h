// Copyright 2024 Normal Browser Authors. All rights reserved.
// manufacturer_traits.h — Defines per-OEM rules for procedural generation.

#ifndef DEVICE_PROFILE_GENERATOR_MANUFACTURER_TRAITS_H_
#define DEVICE_PROFILE_GENERATOR_MANUFACTURER_TRAITS_H_

#include <string>
#include <vector>

#include "device_profile_generator/device_profile.h"

namespace normal_browser {

// Screen resolution range for a manufacturer
struct ScreenRange {
  int min_width;
  int max_width;
  int min_height;
  int max_height;
  std::vector<float> common_densities;
};

// Camera configuration (front_count, back_count)
struct CameraConfig {
  int front;
  int back;
};

// SoC compatibility entry (soc_codename, weight for this OEM)
struct SoCCompat {
  std::string soc_codename;
  float weight;
  PriceTier tier;
};

// Complete manufacturer trait definition
struct ManufacturerTraits {
  Manufacturer id;
  std::string name;
  float market_weight;  // Probability weight for selection

  // SoC compatibility
  std::vector<SoCCompat> compatible_socs;

  // Screen ranges by tier
  ScreenRange budget_screen;
  ScreenRange mid_screen;
  ScreenRange flagship_screen;

  // RAM options (MB)
  std::vector<int> budget_ram;
  std::vector<int> mid_ram;
  std::vector<int> flagship_ram;
  std::vector<int> premium_ram;

  // Camera configs
  std::vector<CameraConfig> camera_configs;

  // Custom fonts (in addition to base Android fonts)
  std::vector<std::string> custom_fonts;

  // System UI font name
  std::string system_ui_font;

  // Emoji font
  std::string emoji_font;

  // Sensor vendors this OEM uses
  struct SensorSet {
    std::string accelerometer_name;
    std::string accelerometer_vendor;
    std::string gyroscope_name;
    std::string gyroscope_vendor;
    std::string magnetometer_name;
    std::string magnetometer_vendor;
  };
  std::vector<SensorSet> sensor_sets;

  // Build fingerprint format
  // %M = manufacturer lowercase, %m = model lowercase, %c = codename
  // %v = android version, %b = build_id, %s = security_patch
  std::string build_fp_format;
  std::string build_display_format;
};

// Get the global manufacturer traits database
const std::vector<ManufacturerTraits>& GetAllManufacturerTraits();

// Get traits for a specific manufacturer
const ManufacturerTraits& GetManufacturerTraits(Manufacturer m);

// Pick a random manufacturer by market weight
Manufacturer PickRandomManufacturer(uint32_t seed);

// Pick a random price tier by weight
PriceTier PickRandomPriceTier(uint32_t seed);

}  // namespace normal_browser

#endif  // DEVICE_PROFILE_GENERATOR_MANUFACTURER_TRAITS_H_
