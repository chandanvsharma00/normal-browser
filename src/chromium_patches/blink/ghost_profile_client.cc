// Copyright 2024 Normal Browser Authors. All rights reserved.
// ghost_profile_client.cc — Renderer-side profile cache implementation.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstdlib>
#include <cstring>
#include <ctime>

#include "base/no_destructor.h"
#include "base/logging.h"
#include "base/rand_util.h"

namespace normal_browser {

GhostProfileClient::GhostProfileClient() = default;
GhostProfileClient::~GhostProfileClient() = default;

// static
GhostProfileClient* GhostProfileClient::Get() {
  static base::NoDestructor<GhostProfileClient> instance;
  return instance.get();
}

namespace {

// Deterministic hash from session seed for per-session noise values.
uint64_t SplitMix64(uint64_t seed) {
  seed += 0x9E3779B97F4A7C15ULL;
  seed = (seed ^ (seed >> 30)) * 0xBF58476D1CE4E5B9ULL;
  seed = (seed ^ (seed >> 27)) * 0x94D049BB133111EBULL;
  return seed ^ (seed >> 31);
}

float SeedToFloat(uint64_t hash) {
  // Convert to float in range [0.001, 0.999]
  return 0.001f + static_cast<float>(hash & 0xFFFFFF) / 16777216.0f * 0.998f;
}

}  // namespace

void GhostProfileClient::Initialize() {
  if (ready_) return;  // Already initialized

  // Generate a session seed for per-session fingerprint variation.
  // Uses base::RandUint64() which is cryptographically random.
  uint64_t session_seed = base::RandUint64();

  // Populate profile with realistic values for the actual device.
  // Device: OPPO CPH2583 (A3 Pro 5G) — Dimensity 7050 / Mali-G610 MC3
  // These values match what the REAL hardware reports via Chrome 130.

  // === GPU / WebGL (CRITICAL — was empty, causing FPJS anomaly) ===
  profile_.gl_renderer = "Mali-G610 MC3";
  profile_.gl_vendor = "ARM";
  profile_.gl_version = "OpenGL ES 3.2 v1.r38p0";
  profile_.gl_shading_language_version = "OpenGL ES GLSL ES 3.20";
  profile_.gl_max_texture_size = 8192;
  profile_.gl_max_renderbuffer_size = 8192;
  profile_.gl_max_viewport_dims[0] = 8192;
  profile_.gl_max_viewport_dims[1] = 8192;
  profile_.gl_max_combined_texture_units = 48;
  profile_.gl_max_vertex_attribs = 16;
  profile_.gl_max_varying_vectors = 32;
  profile_.gl_aliased_line_width[0] = 1.0f;
  profile_.gl_aliased_line_width[1] = 1.0f;
  profile_.gl_aliased_point_size[0] = 1.0f;
  profile_.gl_aliased_point_size[1] = 2048.0f;

  // === Identity (match device user agent) ===
  profile_.manufacturer = "OPPO";
  profile_.model = "CPH2583";
  profile_.device_name = "CPH2583";
  profile_.soc_name = "Dimensity 7050";
  profile_.soc_tier = 2;
  profile_.cpu_cores = 8;
  profile_.hardware_string = "mt6878";
  profile_.android_version = 14;
  profile_.sdk_version = 34;

  // === Screen (match real device: 6.67" FHD+ LCD) ===
  profile_.screen_width = 1080;
  profile_.screen_height = 2400;
  profile_.density = 2.75f;
  profile_.dpi = 440;

  // === Fingerprint noise seeds (per-session variation) ===
  uint64_t h1 = SplitMix64(session_seed);
  uint64_t h2 = SplitMix64(h1);
  uint64_t h3 = SplitMix64(h2);
  uint64_t h4 = SplitMix64(h3);
  uint64_t h5 = SplitMix64(h4);

  profile_.canvas_noise_seed = SeedToFloat(h1);
  profile_.audio_context_seed = SeedToFloat(h2);
  profile_.webgl_noise_seed = SeedToFloat(h3);
  profile_.client_rect_noise_seed = SeedToFloat(h4);
  profile_.sensor_noise_seed = SeedToFloat(h5);

  // === Audio ===
  profile_.audio_sample_rate = 48000;
  profile_.audio_buffer_size = 512;

  // === RAM / Memory ===
  profile_.ram_mb = 8192;
  profile_.device_memory_gb = 8.0f;

  // === Camera ===
  profile_.front_cameras = 1;
  profile_.back_cameras = 3;

  // === Sensors (real Dimensity 7050 sensor set) ===
  profile_.accelerometer_name = "ICM4N607 Accelerometer";
  profile_.accelerometer_vendor = "InvenSense";
  profile_.gyroscope_name = "ICM4N607 Gyroscope";
  profile_.gyroscope_vendor = "InvenSense";
  profile_.has_gyroscope = true;
  profile_.has_barometer = false;

  // === Network (realistic defaults) ===
  profile_.connection_type = "wifi";
  profile_.network_downlink_mbps = 45.0;
  profile_.network_rtt_ms = 10;
  profile_.effective_type = "4g";

  // === CSS ===
  profile_.prefers_dark_mode = true;
  profile_.supports_p3_gamut = false;  // LCD panel = sRGB only

  // === Language ===
  profile_.primary_language = "en-IN";
  profile_.languages = {"en-IN", "en", "hi"};

  LOG(INFO) << "[GhostMode] Profile initialized with per-session seeds. "
            << "GL: " << profile_.gl_renderer
            << " canvas_seed=" << profile_.canvas_noise_seed
            << " rect_seed=" << profile_.client_rect_noise_seed;

  ready_ = true;
}

int GhostProfileClient::GetCanvasMinMs() const {
  // Performance timing by SoC tier
  static const int kMin[] = {5, 12, 25, 50};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMin[tier];
}

int GhostProfileClient::GetCanvasMaxMs() const {
  static const int kMax[] = {12, 25, 50, 100};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMax[tier];
}

int GhostProfileClient::GetWebGLMinMs() const {
  static const int kMin[] = {3, 8, 15, 30};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMin[tier];
}

int GhostProfileClient::GetWebGLMaxMs() const {
  static const int kMax[] = {8, 15, 30, 60};
  int tier = profile_.soc_tier;
  if (tier < 0 || tier > 3) tier = 2;
  return kMax[tier];
}

}  // namespace normal_browser
