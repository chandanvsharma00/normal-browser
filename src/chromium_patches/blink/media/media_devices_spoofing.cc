// Copyright 2024 Normal Browser Authors. All rights reserved.
// media_devices_spoofing.cc — navigator.mediaDevices.enumerateDevices() patch.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/modules/mediastream/media_devices.cc
//   third_party/blink/renderer/modules/mediastream/media_device_info.cc

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace blink {

// ====================================================================
// Media Device Info Structure
// ====================================================================
struct SpoofedMediaDevice {
  std::string device_id;
  std::string kind;       // "videoinput", "audioinput", "audiooutput"
  std::string label;
  std::string group_id;
};

// ====================================================================
// Deterministic UUID generation from seed + index
// ====================================================================
namespace {

std::string GenerateDeviceId(uint64_t seed, uint32_t index) {
  // Hash seed + index to produce a stable UUID-like string
  uint64_t h = seed ^ (static_cast<uint64_t>(index) * 0x9E3779B97F4A7C15ULL);
  h ^= h >> 33;
  h *= 0xFF51AFD7ED558CCDULL;
  h ^= h >> 33;
  h *= 0xC4CEB9FE1A85EC53ULL;
  h ^= h >> 33;

  uint64_t h2 = h * 0x517CC1B727220A95ULL;
  h2 ^= h2 >> 33;

  char buf[48];
  snprintf(buf, sizeof(buf),
           "%08x-%04x-%04x-%04x-%012llx",
           static_cast<uint32_t>(h & 0xFFFFFFFF),
           static_cast<uint16_t>((h >> 32) & 0xFFFF),
           static_cast<uint16_t>(((h >> 48) & 0x0FFF) | 0x4000),  // v4 UUID
           static_cast<uint16_t>(((h2) & 0x3FFF) | 0x8000),
           static_cast<unsigned long long>(h2 >> 16) & 0xFFFFFFFFFFFFULL);
  return std::string(buf);
}

std::string GenerateGroupId(uint64_t seed, uint32_t group_index) {
  return GenerateDeviceId(seed ^ 0x62500000ULL, group_index);
}

}  // namespace

// ====================================================================
// GetSpoofedMediaDevices
// File: third_party/blink/renderer/modules/mediastream/media_devices.cc
//
// Called when JS calls navigator.mediaDevices.enumerateDevices().
// Returns camera/mic count matching the profile's device.
// ====================================================================
std::vector<SpoofedMediaDevice> GetSpoofedMediaDevices() {
  auto* client = normal_browser::GhostProfileClient::Get();
  std::vector<SpoofedMediaDevice> devices;

  uint64_t seed = 0x4D45444941ULL;  // "MEDIA" fallback seed
  int front_cameras = 1;
  int back_cameras = 1;

  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    // Convert float seed to uint64 (bit-preserving) for hashing.
    uint32_t seed_bits = 0;
    memcpy(&seed_bits, &p.canvas_noise_seed, sizeof(seed_bits));
    seed = static_cast<uint64_t>(seed_bits);
    front_cameras = p.front_cameras;
    back_cameras = p.back_cameras;
  }

  uint32_t idx = 0;

  // Back cameras
  for (int i = 0; i < back_cameras; ++i) {
    SpoofedMediaDevice cam;
    cam.device_id = GenerateDeviceId(seed, idx);
    cam.kind = "videoinput";
    if (i == 0) {
      cam.label = "Back Camera";
    } else if (i == 1) {
      cam.label = "Back Ultra-Wide Camera";
    } else if (i == 2) {
      cam.label = "Back Telephoto Camera";
    } else {
      cam.label = "Back Camera " + std::to_string(i + 1);
    }
    cam.group_id = GenerateGroupId(seed, 0);  // All cameras in group 0
    devices.push_back(cam);
    ++idx;
  }

  // Front cameras
  for (int i = 0; i < front_cameras; ++i) {
    SpoofedMediaDevice cam;
    cam.device_id = GenerateDeviceId(seed, idx);
    cam.kind = "videoinput";
    cam.label = (i == 0) ? "Front Camera" : "Front Camera " + std::to_string(i + 1);
    cam.group_id = GenerateGroupId(seed, 1);
    devices.push_back(cam);
    ++idx;
  }

  // Microphone (always 1 on mobile)
  {
    SpoofedMediaDevice mic;
    mic.device_id = GenerateDeviceId(seed, idx++);
    mic.kind = "audioinput";
    mic.label = "Default";
    mic.group_id = GenerateGroupId(seed, 2);
    devices.push_back(mic);
  }

  // Speaker (always 1 on mobile)
  {
    SpoofedMediaDevice spk;
    spk.device_id = GenerateDeviceId(seed, idx++);
    spk.kind = "audiooutput";
    spk.label = "Default";
    spk.group_id = GenerateGroupId(seed, 3);
    devices.push_back(spk);
  }

  return devices;
}

}  // namespace blink
