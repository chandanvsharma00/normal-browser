// Copyright 2024 Normal Browser Authors. All rights reserved.
// storage_memory_spoofing.cc — Storage/Memory/Network info spoofing.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/modules/quota/storage_manager.cc
//   third_party/blink/renderer/core/timing/memory_info.cc
//   third_party/blink/renderer/modules/netinfo/network_information.cc

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstdint>
#include <cstring>

namespace blink {

// ====================================================================
// Noise utility
// ====================================================================
namespace {

// Convert float seed to uint64 for hashing (bit-preserving).
uint64_t SeedToU64(float seed) {
  uint32_t bits = 0;
  memcpy(&bits, &seed, sizeof(bits));
  return static_cast<uint64_t>(bits);
}

int64_t StorageNoise(uint64_t seed, uint64_t channel) {
  uint64_t h = seed ^ (channel * 0x517CC1B727220A95ULL);
  h ^= h >> 33;
  h *= 0xFF51AFD7ED558CCDULL;
  h ^= h >> 33;
  // Return signed noise ±500MB range
  return static_cast<int64_t>(h % 1000000000ULL) - 500000000LL;
}

}  // namespace

// ====================================================================
// navigator.storage.estimate() spoofing
// ====================================================================
struct SpoofedStorageEstimate {
  uint64_t quota;
  uint64_t usage;
};

SpoofedStorageEstimate GetSpoofedStorageEstimate() {
  auto* client = normal_browser::GhostProfileClient::Get();

  int64_t total_bytes = 128LL * 1024 * 1024 * 1024;  // 128GB default
  uint64_t seed = 0x53544F52ULL;

  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    total_bytes = p.storage_total_bytes;
    seed = SeedToU64(p.canvas_noise_seed);
  }

  SpoofedStorageEstimate est;

  // Chromium reports quota as ~60% of total storage for web apps
  est.quota = static_cast<uint64_t>(total_bytes * 0.6);

  // Add realistic noise to quota (±500MB)
  int64_t quota_noise = StorageNoise(seed, 0xA007);
  est.quota = static_cast<uint64_t>(
      static_cast<int64_t>(est.quota) + quota_noise);

  // Usage: 30-70% of quota
  uint64_t usage_base = static_cast<uint64_t>(est.quota * 0.3);
  uint64_t usage_range = static_cast<uint64_t>(est.quota * 0.4);
  uint64_t usage_hash = static_cast<uint64_t>(
      StorageNoise(seed, 0xA5A6) & 0x7FFFFFFFFFFFFFFFULL);
  est.usage = usage_base + (usage_hash % usage_range);

  return est;
}

// ====================================================================
// performance.memory spoofing
// ====================================================================
struct SpoofedMemoryInfo {
  uint64_t js_heap_size_limit;
  uint64_t total_js_heap_size;
  uint64_t used_js_heap_size;
};

SpoofedMemoryInfo GetSpoofedMemoryInfo() {
  auto* client = normal_browser::GhostProfileClient::Get();

  int ram_mb = 4096;
  uint64_t seed = 0x4D454D4FULL;

  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    ram_mb = p.ram_mb;
    seed = SeedToU64(p.canvas_noise_seed);
  }

  SpoofedMemoryInfo info;

  if (ram_mb <= 3072) {
    info.js_heap_size_limit = 1536ULL * 1024 * 1024;
  } else if (ram_mb <= 4096) {
    info.js_heap_size_limit = 2048ULL * 1024 * 1024;
  } else if (ram_mb <= 6144) {
    info.js_heap_size_limit = 2560ULL * 1024 * 1024;
  } else if (ram_mb <= 8192) {
    info.js_heap_size_limit = 4096ULL * 1024 * 1024;
  } else {
    info.js_heap_size_limit = 4096ULL * 1024 * 1024;
  }

  // Total heap: 10-30% of limit
  double usage_fraction = 0.1 +
      static_cast<double>(StorageNoise(seed, 0xBEAF) & 0xFFFF) /
      static_cast<double>(0xFFFF) * 0.2;
  info.total_js_heap_size = static_cast<uint64_t>(
      info.js_heap_size_limit * usage_fraction);

  // Used: 60-90% of total
  double used_fraction = 0.6 +
      static_cast<double>(StorageNoise(seed, 0xA5ED) & 0xFFFF) /
      static_cast<double>(0xFFFF) * 0.3;
  info.used_js_heap_size = static_cast<uint64_t>(
      info.total_js_heap_size * used_fraction);

  return info;
}

// ====================================================================
// Navigator.connection spoofing (Network Information API)
// ====================================================================

std::string GetSpoofedEffectiveType() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return "4g";
  // RendererProfile.effective_type (was effective_connection_type, now fixed)
  return client->GetProfile().effective_type;
}

double GetSpoofedDownlink() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 10.0;

  const auto& p = client->GetProfile();
  uint64_t seed = SeedToU64(p.canvas_noise_seed);
  uint64_t h = static_cast<uint64_t>(
      StorageNoise(seed, 0xDE10) & 0x7FFFFFFFFFFFFFFFULL);
  double base = (p.effective_type == "4g") ? 5.0 : 20.0;
  double range = (p.effective_type == "4g") ? 15.0 : 60.0;
  return base + static_cast<double>(h & 0xFFFF) / 65535.0 * range;
}

unsigned long GetSpoofedRtt() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 50;

  const auto& p = client->GetProfile();
  uint64_t seed = SeedToU64(p.canvas_noise_seed);
  uint64_t h = static_cast<uint64_t>(
      StorageNoise(seed, 0x0277) & 0x7FFFFFFFFFFFFFFFULL);

  unsigned long base_rtt;
  if (p.effective_type == "4g") {
    base_rtt = 30 + (h & 0xFF) % 50;
  } else {
    base_rtt = 5 + (h & 0xFF) % 15;
  }

  // Round to nearest 25ms as per Network Information API spec
  return ((base_rtt + 12) / 25) * 25;
}

}  // namespace blink
