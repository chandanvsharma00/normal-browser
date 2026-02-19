// Copyright 2024 Normal Browser Authors. All rights reserved.
// device_profile.h — Core DeviceProfile data structure.
// This is the central identity representation used throughout the browser.

#ifndef DEVICE_PROFILE_GENERATOR_DEVICE_PROFILE_H_
#define DEVICE_PROFILE_GENERATOR_DEVICE_PROFILE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/values.h"

namespace normal_browser {

// ============================================================
// Enumerations
// ============================================================

enum class Manufacturer {
  kSamsung = 0,
  kXiaomi = 1,
  kRealme = 2,
  kOnePlus = 3,
  kVivo = 4,
  kOPPO = 5,
  kMotorola = 6,
  kIQOO = 7,
  kNothing = 8,
  kGoogle = 9,
  kTecno = 10,
  kMaxValue = kTecno,
};

enum class PriceTier {
  kBudget = 0,      // 40% weight
  kMidRange = 1,    // 35% weight
  kFlagship = 2,    // 15% weight
  kPremium = 3,     // 10% weight
};

enum class ConnectionType {
  kWifi = 0,
  k4G = 1,
};

// ============================================================
// TLS Profile (for Cloudflare JA3/JA4 bypass)
// ============================================================

struct TLSProfile {
  // Cipher suite configuration
  uint32_t cipher_shuffle_seed = 0;
  int cipher_drop_count = 0;  // 0-3
  std::vector<int> grease_cipher_positions;

  // Extension configuration
  uint32_t extension_shuffle_seed = 0;
  std::vector<int> grease_extension_positions;
  int num_grease_extensions = 2;  // 2-4

  // Supported groups (elliptic curves)
  bool swap_x25519_p256 = false;
  bool include_p521 = false;
  bool include_kyber = false;

  // Signature algorithms
  uint32_t sig_alg_shuffle_seed = 0;

  // ALPN
  bool include_http2 = true;
  bool include_http3 = false;

  // Session ticket
  bool send_session_ticket = true;

  // Padding
  int padding_target = 512;  // 512 or 1024

  // Computed hashes (for debugging/display)
  std::string ja3_hash;
  std::string ja4_hash;

  base::Value::Dict ToDict() const;
  static TLSProfile FromDict(const base::Value::Dict& dict);
};

// ============================================================
// Main Device Profile
// ============================================================

struct DeviceProfile {
  // --- Identity ---
  Manufacturer manufacturer = Manufacturer::kSamsung;
  std::string manufacturer_name;         // "Samsung", "Xiaomi", etc.
  std::string model;                     // "SM-S936B", "24031PN0DC", etc.
  std::string device_name;               // "Galaxy S24 Ultra", "Redmi Note 13 Pro"
  std::string device_codename;           // "s936b", "tapas", etc.

  // --- Android Build ---
  int android_version = 14;              // 12, 13, 14, 15
  int sdk_version = 34;                  // Derived from android_version
  std::string build_id;                  // "UP1A.231005.007"
  std::string build_fingerprint;         // Full Build.FINGERPRINT
  std::string build_display;             // Build.DISPLAY
  std::string security_patch;            // "2025-12-01"
  std::string android_id;               // Random 16-char hex
  std::string serial_number;             // OEM-format serial
  std::string bootloader;                // Bootloader version string

  // --- SoC / Hardware ---
  PriceTier price_tier = PriceTier::kMidRange;
  std::string soc_name;                 // "Snapdragon 8 Gen 3"
  std::string soc_codename;             // "sm8650"
  int cpu_cores = 8;
  std::string cpu_architecture;          // "1x Cortex-X4 + 3x A720 + 4x A520"
  std::string hardware_string;           // "qcom" / "mt6789" / "exynos"
  int soc_tier = 1;                      // 0=flagship, 1=upper-mid, 2=mid, 3=budget

  // --- GPU ---
  std::string gl_renderer;              // "Adreno (TM) 750"
  std::string gl_vendor;                // "Qualcomm"
  std::string gl_version;               // "OpenGL ES 3.2 V@0702.0"
  int gl_max_texture_size = 16384;
  int gl_max_renderbuffer_size = 16384;
  int gl_max_viewport_dims[2] = {16384, 16384};
  int gl_max_vertex_attribs = 32;
  int gl_max_varying_vectors = 32;
  int gl_max_combined_texture_units = 96;
  float gl_aliased_line_width[2] = {1.0f, 1.0f};
  float gl_aliased_point_size[2] = {1.0f, 1024.0f};
  std::vector<std::string> gl_extensions;
  std::string gl_shading_language_version;

  // --- Screen ---
  int screen_width = 1080;
  int screen_height = 2400;
  float density = 2.625f;               // devicePixelRatio
  int dpi = 420;                         // density * 160

  // --- RAM / Memory ---
  int ram_mb = 8192;                     // Device RAM in MB
  float device_memory_gb = 8.0f;         // navigator.deviceMemory

  // --- Cameras ---
  int front_cameras = 1;
  int back_cameras = 3;

  // --- Battery ---
  float battery_level = 0.72f;           // 0.15 - 0.95
  bool battery_charging = false;

  // --- Uptime ---
  int uptime_seconds = 86400;            // Random 1000-500000

  // --- Network ---
  ConnectionType connection_type = ConnectionType::kWifi;
  double network_downlink_mbps = 45.0;
  unsigned long network_rtt_ms = 12;
  std::string effective_type;             // "4g"

  // --- Language / Locale ---
  std::string primary_language;           // "en-IN"
  std::vector<std::string> languages;     // ["en-IN", "en", "hi"]

  // --- User Agent ---
  std::string user_agent;
  std::string ch_ua;                     // Sec-CH-UA
  std::string ch_ua_model;              // Sec-CH-UA-Model
  std::string ch_ua_platform_version;   // Sec-CH-UA-Platform-Version
  std::string ch_ua_full_version_list;  // Sec-CH-UA-Full-Version-List
  std::string chrome_version;            // "122.0.6261.64"
  int chrome_major_version = 122;

  // --- Fingerprint Seeds ---
  float canvas_noise_seed = 0.0f;
  float audio_bias = 0.0f;
  float webgl_noise_seed = 0.0f;
  float client_rect_noise_seed = 0.0f;
  float sensor_noise_seed = 0.0f;
  uint64_t math_seed = 0;

  // --- Fonts ---
  std::vector<std::string> system_fonts;
  std::string emoji_font_name;           // "SamsungColorEmoji" or "NotoColorEmoji"
  std::string system_ui_font;            // "SamsungOne" / "MiSans" / "Roboto"

  // --- Sensors ---
  std::string accelerometer_name;
  std::string accelerometer_vendor;
  std::string gyroscope_name;
  std::string gyroscope_vendor;
  std::string magnetometer_name;
  std::string magnetometer_vendor;
  bool has_gyroscope = true;
  bool has_barometer = false;

  // --- Speech Synthesis ---
  struct VoiceInfo {
    std::string name;
    std::string lang;
    bool is_default;
  };
  std::vector<VoiceInfo> tts_voices;

  // --- WebGPU ---
  bool supports_webgpu = true;
  std::string webgpu_vendor;
  std::string webgpu_arch;
  std::string webgpu_device;
  std::string webgpu_description;

  // --- CSS Media Queries ---
  bool prefers_dark_mode = true;
  bool supports_p3_gamut = false;

  // --- Storage ---
  int64_t storage_total_bytes = 0;
  int64_t storage_used_bytes = 0;

  // --- Performance Memory ---
  int64_t js_heap_size_limit = 0;
  int64_t total_js_heap_size = 0;
  int64_t used_js_heap_size = 0;

  // --- WebRTC ---
  std::string fake_local_ip;             // "192.168.x.x"

  // --- TLS ---
  TLSProfile tls_profile;

  // --- Serialization ---
  base::Value::Dict ToDict() const;
  static DeviceProfile FromDict(const base::Value::Dict& dict);
  std::string ToJSON() const;
  static DeviceProfile FromJSON(const std::string& json);

  // --- Derived Helpers ---
  std::string GetPlatformString() const { return "Linux armv8l"; }
  int GetAvailHeight() const;
  int GetAvailWidth() const { return screen_width; }

  // Performance timing helpers
  int GetCanvasMinMs() const;
  int GetCanvasMaxMs() const;
  int GetWebGLMinMs() const;
  int GetWebGLMaxMs() const;
  int GetCryptoMinMs() const;
  int GetCryptoMaxMs() const;
};

// String conversion helpers
std::string ManufacturerToString(Manufacturer m);
Manufacturer StringToManufacturer(const std::string& s);
std::string PriceTierToString(PriceTier t);

}  // namespace normal_browser

#endif  // DEVICE_PROFILE_GENERATOR_DEVICE_PROFILE_H_
