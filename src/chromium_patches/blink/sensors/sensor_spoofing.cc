// Copyright 2024 Normal Browser Authors. All rights reserved.
// sensor_spoofing.cc — Sensor hardware name + data noise spoofing.
//
// FILES TO MODIFY:
//   services/device/generic_sensor/platform_sensor_android.cc
//   third_party/blink/renderer/modules/sensor/sensor.cc
//   third_party/blink/renderer/modules/sensor/accelerometer.cc
//   third_party/blink/renderer/modules/sensor/gyroscope.cc
//   third_party/blink/renderer/modules/sensor/magnetometer.cc

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace blink {

// ====================================================================
// Sensor Noise Utility
// ====================================================================
namespace {

double SensorHash(uint64_t seed, uint64_t channel, uint64_t timestamp) {
  uint64_t h = seed ^ (channel * 0x9E3779B97F4A7C15ULL);
  h ^= timestamp * 0x517CC1B727220A95ULL;
  h ^= h >> 33;
  h *= 0xFF51AFD7ED558CCDULL;
  h ^= h >> 33;
  h *= 0xC4CEB9FE1A85EC53ULL;
  h ^= h >> 33;
  double normalized = static_cast<double>(h & 0xFFFFFFFF) /
                      static_cast<double>(0xFFFFFFFF) * 2.0 - 1.0;
  return normalized;
}

// Convert float seed to uint64 for hashing (bit-preserving).
uint64_t SeedToU64(float seed) {
  uint32_t bits = 0;
  memcpy(&bits, &seed, sizeof(bits));
  return static_cast<uint64_t>(bits);
}

}  // namespace

// ====================================================================
// Sensor Hardware Names
// ====================================================================

struct SpoofedSensorInfo {
  std::string accelerometer_name;
  std::string accelerometer_vendor;
  std::string gyroscope_name;
  std::string gyroscope_vendor;
  std::string magnetometer_name;
  std::string magnetometer_vendor;
  bool has_gyroscope;
  bool has_barometer;
  float max_accelerometer_range;
  float accelerometer_resolution;
  float max_gyroscope_range;
  float gyroscope_resolution;
};

SpoofedSensorInfo GetSpoofedSensorInfo() {
  auto* client = normal_browser::GhostProfileClient::Get();
  SpoofedSensorInfo info;

  if (!client || !client->IsReady()) {
    info.accelerometer_name = "BMI260 Accelerometer";
    info.accelerometer_vendor = "Bosch";
    info.gyroscope_name = "BMI260 Gyroscope";
    info.gyroscope_vendor = "Bosch";
    info.magnetometer_name = "AK09918 Magnetometer";
    info.magnetometer_vendor = "AKM";
    info.has_gyroscope = true;
    info.has_barometer = false;
    info.max_accelerometer_range = 156.9064f;
    info.accelerometer_resolution = 0.0023956299f;
    info.max_gyroscope_range = 34.906586f;
    info.gyroscope_resolution = 0.001065264f;
    return info;
  }

  const auto& p = client->GetProfile();

  // Read sensor names from RendererProfile (matches DeviceProfile fields)
  info.accelerometer_name = p.accelerometer_name;
  info.accelerometer_vendor = p.accelerometer_vendor;
  info.gyroscope_name = p.gyroscope_name;
  info.gyroscope_vendor = p.gyroscope_vendor;
  info.magnetometer_name = p.magnetometer_name;
  info.magnetometer_vendor = p.magnetometer_vendor;
  info.has_gyroscope = p.has_gyroscope;
  info.has_barometer = p.has_barometer;

  // Sensor ranges vary by hardware
  if (info.accelerometer_vendor == "Bosch") {
    info.max_accelerometer_range = 156.9064f;
    info.accelerometer_resolution = 0.0023956299f;
  } else if (info.accelerometer_vendor == "STMicroelectronics") {
    info.max_accelerometer_range = 78.4532f;
    info.accelerometer_resolution = 0.0023928226f;
  } else if (info.accelerometer_vendor == "InvenSense") {
    info.max_accelerometer_range = 156.9064f;
    info.accelerometer_resolution = 0.004791259f;
  } else {
    info.max_accelerometer_range = 78.4532f;
    info.accelerometer_resolution = 0.009576807f;
  }

  if (info.has_gyroscope) {
    if (info.gyroscope_vendor == "Bosch") {
      info.max_gyroscope_range = 34.906586f;
      info.gyroscope_resolution = 0.001065264f;
    } else if (info.gyroscope_vendor == "STMicroelectronics") {
      info.max_gyroscope_range = 34.906586f;
      info.gyroscope_resolution = 0.001064225f;
    } else {
      info.max_gyroscope_range = 34.906586f;
      info.gyroscope_resolution = 0.00106526f;
    }
  }

  return info;
}

// ====================================================================
// Sensor Data Noise Injection
// ====================================================================

void ApplyAccelerometerNoise(double& x, double& y, double& z,
                              uint64_t timestamp_us) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  uint64_t seed = SeedToU64(client->GetProfile().sensor_noise_seed);
  if (seed == 0) return;

  double magnitude = 0.02;
  x += SensorHash(seed, 0x0AC1, timestamp_us) * magnitude;
  y += SensorHash(seed, 0x0AC2, timestamp_us) * magnitude;
  z += SensorHash(seed, 0x0AC3, timestamp_us) * magnitude;
}

void ApplyGyroscopeNoise(double& x, double& y, double& z,
                          uint64_t timestamp_us) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  uint64_t seed = SeedToU64(client->GetProfile().sensor_noise_seed);
  if (seed == 0) return;

  double magnitude = 0.001;
  x += SensorHash(seed, 0x06E1, timestamp_us) * magnitude;
  y += SensorHash(seed, 0x06E2, timestamp_us) * magnitude;
  z += SensorHash(seed, 0x06E3, timestamp_us) * magnitude;
}

void ApplyMagnetometerNoise(double& x, double& y, double& z,
                             uint64_t timestamp_us) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return;

  uint64_t seed = SeedToU64(client->GetProfile().sensor_noise_seed);
  if (seed == 0) return;

  double magnitude = 0.5;
  x += SensorHash(seed, 0x0E61, timestamp_us) * magnitude;
  y += SensorHash(seed, 0x0E62, timestamp_us) * magnitude;
  z += SensorHash(seed, 0x0E63, timestamp_us) * magnitude;
}

}  // namespace blink
