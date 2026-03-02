// Copyright 2024 Normal Browser Authors. All rights reserved.
// profile_generator.cc — 14-step device profile generation algorithm.
//
// On every "New Identity" or first launch:
// 1.  PICK MANUFACTURER   → weighted random (Samsung 28%, Xiaomi 22%, …)
// 2.  PICK PRICE TIER     → weighted random (budget 40%, mid 35%, …)
// 3.  PICK SoC            → from manufacturer's compatible SoCs for that tier
// 4.  GENERATE MODEL      → using OEM-specific model number rules
// 5.  PICK ANDROID VER    → weighted (14>13>15>12)
// 6.  GENERATE BUILD INFO → build ID, fingerprint, security patch
// 7.  PICK SCREEN         → from manufacturer's screen ranges
// 8.  PICK RAM            → from manufacturer's RAM options
// 9.  GENERATE SEEDS      → canvas, audio, clientRect, WebGL, math, sensor
// 10. GENERATE TLS        → per-session unique TLS profile
// 11. DERIVE MISC         → UA, Client Hints, fonts, sensors, TTS, language
// 12. GENERATE IDS        → android_id, serial, WebRTC fake IPs
// 13. VALIDATE            → ensure all values are internally consistent
// 14. RETURN              → fully populated DeviceProfile

#include "device_profile_generator/profile_generator.h"

#include "base/containers/span.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "base/rand_util.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "crypto/random.h"

#include "device_profile_generator/build_fingerprint_generator.h"
#include "device_profile_generator/device_profile.h"
#include "device_profile_generator/gpu_database.h"
#include "device_profile_generator/manufacturer_traits.h"
#include "device_profile_generator/model_name_generator.h"
#include "device_profile_generator/profile_validator.h"

namespace normal_browser {

namespace {

// ==========================================================
// Deterministic seeded PRNG (xorshift64)
// ==========================================================
struct RNG {
  uint64_t state;

  explicit RNG(uint64_t seed) : state(seed ? seed : 0xDEADBEEF1234ULL) {}

  uint64_t Next() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }

  int Range(int lo, int hi) {
    return lo + static_cast<int>(Next() % (hi - lo + 1));
  }

  float RangeF(float lo, float hi) {
    float t = static_cast<float>(Next() & 0xFFFFFF) / 16777216.0f;
    return lo + t * (hi - lo);
  }

  double RangeD(double lo, double hi) {
    double t = static_cast<double>(Next() & 0xFFFFFFFF) / 4294967296.0;
    return lo + t * (hi - lo);
  }

  bool RandBool() { return (Next() & 1) == 0; }

  uint64_t SubSeed() { return Next(); }

  // Weighted pick from int weights.
  int WeightedPick(const std::vector<int>& weights) {
    int total = 0;
    for (int w : weights) total += w;
    int roll = Range(1, total);
    int cum = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
      cum += weights[i];
      if (roll <= cum) return static_cast<int>(i);
    }
    return static_cast<int>(weights.size() - 1);
  }

  // Weighted pick from float weights (normalized internally).
  int WeightedPickF(const std::vector<float>& weights) {
    float total = 0.0f;
    for (float w : weights) total += w;
    float roll = RangeF(0.0f, total);
    float cum = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
      cum += weights[i];
      if (roll <= cum) return static_cast<int>(i);
    }
    return static_cast<int>(weights.size() - 1);
  }
};

// ==========================================================
// Step 1: Pick Manufacturer
// ==========================================================
Manufacturer PickManufacturer(RNG& rng) {
  // Samsung 28%, Xiaomi 22%, Realme 10%, OnePlus 8%, Vivo 8%,
  // OPPO 6%, Motorola 5%, iQOO 4%, Nothing 3%, Google 3%, Tecno 3%
  std::vector<int> weights = {28, 22, 10, 8, 8, 6, 5, 4, 3, 3, 3};
  int idx = rng.WeightedPick(weights);
  return static_cast<Manufacturer>(idx);
}

// ==========================================================
// Step 2: Pick Price Tier
// ==========================================================
int PickPriceTier(RNG& rng) {
  // PriceTier enum: kBudget=0 (40%), kMidRange=1 (35%),
  //                 kFlagship=2 (15%), kPremium=3 (10%)
  std::vector<int> weights = {40, 35, 15, 10};
  return rng.WeightedPick(weights);
}

// ==========================================================
// Step 3: Pick SoC from manufacturer's compatible list for tier
// ==========================================================
std::string PickSoC(RNG& rng, Manufacturer manufacturer, int tier) {
  const ManufacturerTraits& traits = GetManufacturerTraits(manufacturer);

  // Convert tier int to PriceTier enum for matching.
  PriceTier target_tier = static_cast<PriceTier>(tier);

  std::vector<std::string> candidates;
  std::vector<float> weights;

  for (const auto& compat : traits.compatible_socs) {
    if (compat.tier == target_tier) {
      candidates.push_back(compat.soc_codename);
      weights.push_back(compat.weight);
    }
  }

  // If no candidates for exact tier, pick from nearest tier.
  if (candidates.empty()) {
    PriceTier search_tier = (tier > 0) ? static_cast<PriceTier>(tier - 1)
                                       : static_cast<PriceTier>(tier + 1);
    for (const auto& compat : traits.compatible_socs) {
      if (compat.tier == search_tier) {
        candidates.push_back(compat.soc_codename);
        weights.push_back(compat.weight);
      }
    }
  }

  // Fallback: pick any SoC from this manufacturer.
  if (candidates.empty()) {
    for (const auto& compat : traits.compatible_socs) {
      candidates.push_back(compat.soc_codename);
      weights.push_back(compat.weight);
    }
  }

  if (candidates.empty()) return "snapdragon_695";

  return candidates[rng.WeightedPickF(weights)];
}

// ==========================================================
// Step 7: Pick Screen
// ==========================================================
struct ScreenSpec {
  int width;
  int height;
  float density;
  int dpi;
};

ScreenSpec PickScreen(RNG& rng, int tier) {
  ScreenSpec spec;

  struct ScreenOption {
    int w, h;
    float density;
  };

  std::vector<ScreenOption> options;
  std::vector<int> weights;

  // tier maps to PriceTier enum: 0=budget, 1=mid, 2=flagship, 3=premium
  switch (tier) {
    case 0:  // budget (kBudget)
      options = {{720, 1600, 2.0f}, {720, 1612, 2.0f},
                 {1080, 2400, 2.0f}, {720, 1560, 1.5f}};
      weights = {35, 20, 25, 20};
      break;
    case 1:  // mid-range (kMidRange)
      options = {{1080, 2400, 2.75f}, {1080, 2340, 2.5f},
                 {1080, 2408, 2.5f}, {720, 1600, 2.0f}};
      weights = {35, 25, 20, 20};
      break;
    case 2:  // flagship (kFlagship)
      options = {{1080, 2400, 2.75f}, {1080, 2340, 2.625f},
                 {1080, 2412, 2.75f}, {1240, 2772, 3.0f}};
      weights = {40, 25, 20, 15};
      break;
    case 3:  // premium (kPremium)
    default:
      options = {{1440, 3200, 3.5f}, {1440, 3120, 3.5f},
                 {1080, 2400, 2.75f}, {1440, 3088, 4.0f},
                 {1080, 2340, 2.625f}};
      weights = {30, 20, 25, 15, 10};
      break;
  }

  int idx = rng.WeightedPick(weights);
  spec.width = options[idx].w;
  spec.height = options[idx].h;
  spec.density = options[idx].density;
  spec.dpi = static_cast<int>(spec.density * 160.0f);
  return spec;
}

// ==========================================================
// Step 8: Pick RAM
// ==========================================================
int PickRAM(RNG& rng, int tier) {
  std::vector<int> options;
  std::vector<int> weights;

  // tier maps to PriceTier enum: 0=budget, 1=mid, 2=flagship, 3=premium
  switch (tier) {
    case 0:  // budget
      options = {3072, 4096, 6144};
      weights = {35, 40, 25};
      break;
    case 1:  // mid-range
      options = {4096, 6144, 8192};
      weights = {30, 45, 25};
      break;
    case 2:  // flagship
      options = {6144, 8192, 12288};
      weights = {25, 45, 30};
      break;
    case 3:  // premium
    default:
      options = {8192, 12288, 16384};
      weights = {20, 50, 30};
      break;
  }

  return options[rng.WeightedPick(weights)];
}

// ==========================================================
// Step 11: Derive language
// ==========================================================
std::string PickLanguage(RNG& rng) {
  std::vector<int> weights = {60, 20, 15, 5};
  const char* langs[] = {"en-IN", "en-US", "hi-IN", "en-GB"};
  return langs[rng.WeightedPick(weights)];
}

std::vector<std::string> DeriveLanguages(const std::string& primary) {
  if (primary == "en-IN") return {"en-IN", "en", "hi"};
  if (primary == "en-US") return {"en-US", "en"};
  if (primary == "hi-IN") return {"hi-IN", "hi", "en-IN", "en"};
  if (primary == "en-GB") return {"en-GB", "en"};
  return {"en-US", "en"};
}

// ==========================================================
// Build User-Agent string
// ==========================================================
std::string BuildUserAgent(const std::string& model,
                           int android_version,
                           const std::string& chrome_version) {
  return base::StringPrintf(
      "Mozilla/5.0 (Linux; Android %d; %s) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/%s Mobile Safari/537.36",
      android_version, model.c_str(), chrome_version.c_str());
}

// ==========================================================
// Pick Chrome version
//
// CRITICAL FIX (2026-03-02):
// Previously this generated random versions between 125-131.
// This is WRONG because:
// 1. The ACTUAL built Chromium binary is version 130 (from
//    kChromeVersionFull = "130.0.6723.58" in http_header_patch.cc)
// 2. V8's error message format changes between Chrome versions
// 3. FPJS probes error messages (e.g., "Cannot read properties of null")
//    and compares against the claimed Chrome version
// 4. Internal browser behavior (feature flags, API availability)
//    reflects the REAL built version, not a fake one
//
// The Chrome version MUST ALWAYS match the built binary.
// Only the build/patch numbers can be randomized within the same major.
// ==========================================================
std::string PickChromeVersion(RNG& rng) {
  // MUST match the actual built Chromium major version.
  // Only randomize the build/patch within the same release branch.
  int major = 130;  // Must match kChromeVersionMajor in http_header_patch.cc
  int build = rng.Range(6700, 6750);  // Realistic range for Chrome 130
  int patch = rng.Range(50, 150);
  return base::StringPrintf("%d.0.%d.%d", major, build, patch);
}

// ==========================================================
// WebRTC: Generate fake local IPs
// ==========================================================
std::string GenerateFakeLocalIP(RNG& rng) {
  if (rng.RandBool()) {
    return base::StringPrintf("192.168.%d.%d",
                              rng.Range(0, 255), rng.Range(2, 254));
  } else {
    return base::StringPrintf("10.%d.%d.%d",
                              rng.Range(0, 255), rng.Range(0, 255),
                              rng.Range(2, 254));
  }
}

// ==========================================================
// Generate hex string of given length
// ==========================================================
std::string GenHex(RNG& rng, int len) {
  const char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(len);
  for (int i = 0; i < len; ++i) {
    out += kHex[rng.Next() % 16];
  }
  return out;
}

// Derive navigator.deviceMemory from RAM MB.
// Returns nearest power-of-2 in {2, 4, 6, 8, 12, 16}.
float RamToDeviceMemory(int ram_mb) {
  if (ram_mb >= 16384) return 16.0f;
  if (ram_mb >= 12288) return 12.0f;
  if (ram_mb >= 8192) return 8.0f;
  if (ram_mb >= 6144) return 6.0f;
  if (ram_mb >= 4096) return 4.0f;
  return 2.0f;
}

}  // namespace

// ==========================================================
// MAIN: GenerateDeviceProfile
// ==========================================================
DeviceProfile GenerateDeviceProfile(uint64_t master_seed) {
  // Step 0: Create master seed.
  if (master_seed == 0) {
    crypto::RandBytes(base::as_writable_bytes(base::make_span(&master_seed, 1u)));
  }
  RNG rng(master_seed);

  DeviceProfile p;

  // ===== Step 1: Pick Manufacturer =====
  p.manufacturer = PickManufacturer(rng);
  p.manufacturer_name = ManufacturerToString(p.manufacturer);

  // ===== Step 2: Pick Price Tier =====
  int tier = PickPriceTier(rng);
  p.price_tier = static_cast<PriceTier>(tier);

  // ===== Step 3: Pick SoC =====
  std::string soc_codename = PickSoC(rng, p.manufacturer, tier);
  const SoCProfile* soc = FindSoCProfile(soc_codename);
  if (!soc) {
    soc_codename = "snapdragon_695";
    soc = FindSoCProfile(soc_codename);
  }

  // Fill SoC/GPU fields from database.
  p.soc_name = soc->display_name;
  p.soc_codename = soc->codename;
  p.cpu_cores = soc->cpu_cores;
  p.cpu_architecture = soc->cpu_architecture;
  p.hardware_string = soc->hardware_string;
  p.soc_tier = soc->soc_tier;
  p.gl_renderer = soc->gl_renderer;
  p.gl_vendor = soc->gl_vendor;
  p.gl_version = soc->gl_version;
  p.gl_shading_language_version = soc->gl_shading_language_version;
  p.gl_extensions = soc->gl_extensions;
  p.gl_max_texture_size = soc->gl_max_texture_size;
  p.gl_max_renderbuffer_size = soc->gl_max_renderbuffer_size;
  p.gl_max_viewport_dims[0] = soc->gl_max_viewport_dims[0];
  p.gl_max_viewport_dims[1] = soc->gl_max_viewport_dims[1];
  p.gl_max_vertex_attribs = soc->gl_max_vertex_attribs;
  p.gl_max_varying_vectors = soc->gl_max_varying_vectors;
  p.gl_max_combined_texture_units = soc->gl_max_combined_texture_units;
  p.gl_aliased_line_width[0] = soc->gl_aliased_line_width[0];
  p.gl_aliased_line_width[1] = soc->gl_aliased_line_width[1];
  p.gl_aliased_point_size[0] = soc->gl_aliased_point_size[0];
  p.gl_aliased_point_size[1] = soc->gl_aliased_point_size[1];

  // WebGPU from SoC.
  p.supports_webgpu = soc->supports_webgpu;
  p.webgpu_vendor = soc->webgpu_vendor;
  p.webgpu_arch = soc->webgpu_arch;
  p.webgpu_device = soc->webgpu_device;
  p.webgpu_description = soc->webgpu_description;

  // ===== Step 4: Generate Model =====
  GeneratedModel model = GenerateModelName(p.manufacturer, tier, rng.SubSeed());
  p.model = model.model_number;
  p.device_name = model.marketing_name;
  p.device_codename = model.device_codename;

  // ===== Step 5+6: Generate Build Info =====
  BuildInfo build_info = GenerateBuildInfo(
      p.manufacturer_name, p.model, p.device_codename, rng.SubSeed());
  p.android_version = build_info.android_version;
  p.sdk_version = build_info.sdk_version;
  p.build_id = build_info.build_id;
  p.build_fingerprint = build_info.fingerprint;
  p.build_display = build_info.display;
  p.security_patch = build_info.security_patch;
  p.bootloader = p.manufacturer_name + "_" + p.model + "_BL1";

  // ===== Step 7: Pick Screen =====
  ScreenSpec screen = PickScreen(rng, tier);
  p.screen_width = screen.width;
  p.screen_height = screen.height;
  p.density = screen.density;
  p.dpi = screen.dpi;

  // ===== Step 8: Pick RAM =====
  p.ram_mb = PickRAM(rng, tier);
  p.device_memory_gb = RamToDeviceMemory(p.ram_mb);

  // ===== Step 9: Generate Fingerprint Seeds =====
  p.canvas_noise_seed = rng.RangeF(-0.01f, 0.01f);
  if (p.canvas_noise_seed == 0.0f) p.canvas_noise_seed = 0.00123f;

  p.audio_bias = rng.RangeF(-0.0001f, 0.0001f);
  if (p.audio_bias == 0.0f) p.audio_bias = 0.0000123f;

  p.webgl_noise_seed = rng.RangeF(-0.001f, 0.001f);
  if (p.webgl_noise_seed == 0.0f) p.webgl_noise_seed = 0.000123f;

  p.client_rect_noise_seed = rng.RangeF(-0.01f, 0.01f);
  if (p.client_rect_noise_seed == 0.0f) p.client_rect_noise_seed = 0.005f;

  p.sensor_noise_seed = rng.RangeF(-0.02f, 0.02f);
  if (p.sensor_noise_seed == 0.0f) p.sensor_noise_seed = 0.01f;

  p.math_seed = rng.SubSeed();

  // ===== Step 10: TLS Profile seeds =====
  p.tls_profile.cipher_shuffle_seed = static_cast<uint32_t>(rng.Next());
  p.tls_profile.extension_shuffle_seed = static_cast<uint32_t>(rng.Next());
  p.tls_profile.sig_alg_shuffle_seed = static_cast<uint32_t>(rng.Next());
  p.tls_profile.cipher_drop_count = rng.Range(0, 3);
  p.tls_profile.num_grease_extensions = rng.Range(2, 4);
  p.tls_profile.swap_x25519_p256 = rng.RandBool();
  p.tls_profile.include_p521 = (rng.Range(1, 10) <= 3);
  p.tls_profile.include_kyber = (rng.Range(1, 10) <= 2);
  p.tls_profile.include_http2 = true;
  p.tls_profile.include_http3 = (rng.Range(1, 10) <= 3);
  p.tls_profile.send_session_ticket = rng.RandBool();
  p.tls_profile.padding_target = rng.RandBool() ? 512 : 1024;
  // GREASE positions
  for (int i = 0; i < 3; ++i)
    p.tls_profile.grease_cipher_positions.push_back(rng.Range(0, 5));
  for (int i = 0; i < p.tls_profile.num_grease_extensions; ++i)
    p.tls_profile.grease_extension_positions.push_back(rng.Range(0, 8));

  // ===== Step 11: Derive Misc =====

  // Language.
  p.primary_language = PickLanguage(rng);
  p.languages = DeriveLanguages(p.primary_language);

  // Chrome version.
  p.chrome_version = PickChromeVersion(rng);
  // Extract major version.
  p.chrome_major_version = 0;
  if (!p.chrome_version.empty()) {
    size_t dot = p.chrome_version.find('.');
    if (dot != std::string::npos) {
      p.chrome_major_version = std::stoi(p.chrome_version.substr(0, dot));
    }
  }

  // User-Agent.
  p.user_agent = BuildUserAgent(p.model, p.android_version, p.chrome_version);

  // Client Hints.
  p.ch_ua_model = p.model;
  p.ch_ua_platform_version =
      std::to_string(p.android_version) + ".0.0";
  p.ch_ua_full_version_list = base::StringPrintf(
      "\"Chromium\";v=\"%s\", \"Google Chrome\";v=\"%s\", \"Not-A.Brand\";v=\"99.0.0.0\"",
      p.chrome_version.c_str(), p.chrome_version.c_str());

  // Sec-CH-UA header.
  p.ch_ua = base::StringPrintf(
      "\"Chromium\";v=\"%d\", \"Google Chrome\";v=\"%d\", \"Not-A.Brand\";v=\"99\"",
      p.chrome_major_version, p.chrome_major_version);

  // Fonts — combine base Android fonts + OEM fonts.
  const ManufacturerTraits& traits = GetManufacturerTraits(p.manufacturer);
  p.system_fonts = {"Roboto", "Noto Sans", "Droid Sans"};
  for (const auto& font : traits.custom_fonts) {
    p.system_fonts.push_back(font);
  }
  p.system_ui_font = traits.system_ui_font;
  p.emoji_font_name = traits.emoji_font;

  // Sensors — pick a random sensor set from manufacturer's options.
  if (!traits.sensor_sets.empty()) {
    int sensor_idx = rng.Range(0, static_cast<int>(traits.sensor_sets.size()) - 1);
    const auto& ss = traits.sensor_sets[sensor_idx];
    p.accelerometer_name = ss.accelerometer_name;
    p.accelerometer_vendor = ss.accelerometer_vendor;
    p.gyroscope_name = ss.gyroscope_name;
    p.gyroscope_vendor = ss.gyroscope_vendor;
    p.magnetometer_name = ss.magnetometer_name;
    p.magnetometer_vendor = ss.magnetometer_vendor;
    p.has_gyroscope = !ss.gyroscope_name.empty();
    p.has_barometer = (tier <= 1);  // Only flagship/upper-mid have barometers.
  }

  // TTS voices.
  p.tts_voices.clear();
  // All Android devices have Google TTS.
  p.tts_voices.push_back({"Google US English", "en-US", true});
  p.tts_voices.push_back({"Google UK English Male", "en-GB", false});
  p.tts_voices.push_back({"Google Hindi", "hi-IN", false});
  if (p.manufacturer == Manufacturer::kSamsung) {
    p.tts_voices.push_back({"Samsung English US", "en-US", false});
    p.tts_voices.push_back({"Samsung Hindi India", "hi-IN", false});
  }
  if (rng.RandBool())
    p.tts_voices.push_back({"Google Español", "es-ES", false});

  // Cameras — from manufacturer traits.
  if (!traits.camera_configs.empty()) {
    int cam_idx = rng.Range(0, static_cast<int>(traits.camera_configs.size()) - 1);
    p.front_cameras = traits.camera_configs[cam_idx].front;
    p.back_cameras = traits.camera_configs[cam_idx].back;
  } else {
    p.front_cameras = 1;
    p.back_cameras = rng.Range(2, 4);
  }

  // Battery.
  p.battery_level = rng.RangeF(0.15f, 0.95f);
  p.battery_charging = rng.RandBool();

  // Uptime.
  p.uptime_seconds = rng.Range(1000, 500000);

  // Network.
  if (rng.RandBool()) {
    p.connection_type = ConnectionType::kWifi;
    p.network_downlink_mbps = rng.RangeD(20.0, 80.0);
    p.network_rtt_ms = static_cast<unsigned long>(rng.Range(5, 20));
    p.effective_type = "4g";
  } else {
    p.connection_type = ConnectionType::k4G;
    p.network_downlink_mbps = rng.RangeD(5.0, 20.0);
    p.network_rtt_ms = static_cast<unsigned long>(rng.Range(30, 80));
    p.effective_type = "4g";
  }

  // Storage.
  const int64_t kGB = 1024LL * 1024LL * 1024LL;
  int64_t storage_options[] = {64 * kGB, 128 * kGB, 256 * kGB, 512 * kGB};
  int storage_idx;
  // tier: 0=budget, 1=mid, 2=flagship, 3=premium
  switch (tier) {
    case 0: storage_idx = rng.Range(0, 1); break;  // budget: 64-128GB
    case 1: storage_idx = rng.Range(1, 2); break;  // mid: 128-256GB
    case 2: storage_idx = rng.Range(1, 2); break;  // flagship: 128-256GB
    case 3: default: storage_idx = rng.Range(2, 3); break;  // premium: 256-512GB
  }
  p.storage_total_bytes = storage_options[storage_idx];
  float used_pct = rng.RangeF(0.30f, 0.70f);
  p.storage_used_bytes =
      static_cast<int64_t>(p.storage_total_bytes * used_pct);

  // Memory pressure (performance.memory).
  p.js_heap_size_limit =
      static_cast<int64_t>(p.ram_mb) * 1024LL * 1024LL / 4;
  p.total_js_heap_size = p.js_heap_size_limit / 2;
  p.used_js_heap_size = p.total_js_heap_size / 3;

  // CSS Media Queries.
  p.prefers_dark_mode = (rng.Range(1, 100) <= 60);
  p.supports_p3_gamut = (tier <= 1);

  // ===== Step 12: Generate IDs =====
  p.android_id = GenerateAndroidId(rng.SubSeed());
  p.serial_number =
      GenerateSerialNumber(p.manufacturer_name, rng.SubSeed());

  // WebRTC fake IP.
  p.fake_local_ip = GenerateFakeLocalIP(rng);

  // ===== Step 13: Validate =====
  // Validate the profile and log any warnings (don't reject — still usable).
  auto errors = ValidateProfile(p);
  for (const auto& e : errors) {
    LOG(WARNING) << "Profile validation: " << e.field << ": " << e.message;
  }

  // ===== Step 14: Profile complete =====
  return p;
}

void RotateSessionSeeds(DeviceProfile& profile) {
  uint64_t new_seed;
  crypto::RandBytes(base::as_writable_bytes(base::make_span(&new_seed, 1u)));
  RNG rng(new_seed);

  profile.canvas_noise_seed = rng.RangeF(-0.01f, 0.01f);
  if (profile.canvas_noise_seed == 0.0f) profile.canvas_noise_seed = 0.00123f;

  profile.audio_bias = rng.RangeF(-0.0001f, 0.0001f);
  if (profile.audio_bias == 0.0f) profile.audio_bias = 0.0000123f;

  profile.webgl_noise_seed = rng.RangeF(-0.001f, 0.001f);
  if (profile.webgl_noise_seed == 0.0f) profile.webgl_noise_seed = 0.000123f;

  profile.client_rect_noise_seed = rng.RangeF(-0.01f, 0.01f);
  if (profile.client_rect_noise_seed == 0.0f)
    profile.client_rect_noise_seed = 0.005f;

  profile.sensor_noise_seed = rng.RangeF(-0.02f, 0.02f);
  if (profile.sensor_noise_seed == 0.0f) profile.sensor_noise_seed = 0.01f;

  profile.math_seed = rng.SubSeed();

  // Rotate TLS seeds.
  profile.tls_profile.cipher_shuffle_seed = static_cast<uint32_t>(rng.Next());
  profile.tls_profile.extension_shuffle_seed =
      static_cast<uint32_t>(rng.Next());
  profile.tls_profile.sig_alg_shuffle_seed = static_cast<uint32_t>(rng.Next());
}

}  // namespace normal_browser
