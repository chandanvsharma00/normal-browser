// Copyright 2024 Normal Browser Authors. All rights reserved.
// ghost_profile_client.h — Renderer-side client for reading device profile.
//
// Each renderer process requests the profile from the browser process once
// on startup, then caches it locally. All Blink/V8 patches read from this
// cached copy to avoid IPC on every JS API call.
//
// File location: third_party/blink/renderer/core/ghost_profile_client.h
//
// IMPORTANT: Field names here MUST match ghost_profile.mojom exactly.
// All Blink patch files read from RendererProfile via GetProfile().

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_BLINK_GHOST_PROFILE_CLIENT_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_BLINK_GHOST_PROFILE_CLIENT_H_

#include <string>
#include <vector>

#include "base/no_destructor.h"

namespace normal_browser {

// Lightweight renderer-side profile cache.
// Populated once from Mojo IPC, then read-only.
struct RendererProfile {
  // === Identity ===
  std::string manufacturer;
  std::string model;
  std::string device_name;
  std::string user_agent;
  int android_version = 14;
  int sdk_version = 34;
  std::string build_fingerprint;
  std::string build_display;
  std::string build_id;
  std::string security_patch;
  std::string android_id;
  std::string serial_number;

  // === Client Hints ===
  std::string ch_ua;
  std::string ch_ua_model;
  std::string ch_ua_platform_version;
  std::string ch_ua_full_version_list;

  // === Hardware ===
  int cpu_cores = 8;
  std::string hardware_string;
  std::string soc_name;
  int soc_tier = 1;
  int ram_mb = 8192;
  float device_memory_gb = 8.0f;

  // === Screen ===
  int screen_width = 1080;
  int screen_height = 2400;
  float density = 2.75f;
  int dpi = 440;

  // === GPU / WebGL ===
  std::string gl_renderer;
  std::string gl_vendor;
  std::string gl_version;
  std::string gl_shading_language_version;
  std::vector<std::string> gl_extensions;
  int gl_max_texture_size = 8192;
  int gl_max_renderbuffer_size = 8192;
  int gl_max_viewport_dims[2] = {8192, 8192};
  int gl_max_combined_texture_units = 16;
  int gl_max_vertex_attribs = 32;
  int gl_max_varying_vectors = 32;
  float gl_aliased_point_size[2] = {1.0f, 1.0f};
  float gl_aliased_line_width[2] = {1.0f, 1.0f};

  // === Audio ===
  int audio_sample_rate = 48000;
  int audio_buffer_size = 256;

  // === Fingerprint seeds ===
  float canvas_noise_seed = 0.0f;
  float audio_context_seed = 0.0f;
  float webgl_noise_seed = 0.0f;
  float client_rect_noise_seed = 0.0f;
  float sensor_noise_seed = 0.0f;
  uint64_t math_seed = 0;

  // === Language ===
  std::string primary_language = "en-IN";
  std::vector<std::string> languages = {"en-IN", "en", "hi"};

  // === Camera ===
  int front_cameras = 1;
  int back_cameras = 3;

  // === Battery ===
  float battery_level = 0.72f;
  bool battery_charging = false;

  // === Network ===
  std::string connection_type = "wifi";
  double network_downlink_mbps = 45.0;
  uint32_t network_rtt_ms = 10;
  std::string effective_type = "4g";

  // === Storage & Memory ===
  int64_t storage_total_bytes = 0;
  int64_t storage_used_bytes = 0;
  int64_t js_heap_size_limit = 0;
  int64_t total_js_heap_size = 0;
  int64_t used_js_heap_size = 0;

  // === Fonts ===
  std::vector<std::string> system_fonts;
  std::string system_ui_font;
  std::string emoji_font_name;

  // === Sensors ===
  std::string accelerometer_name;
  std::string accelerometer_vendor;
  std::string gyroscope_name;
  std::string gyroscope_vendor;
  std::string magnetometer_name;
  std::string magnetometer_vendor;
  bool has_gyroscope = true;
  bool has_barometer = false;

  // === TTS ===
  std::vector<std::string> tts_voices;

  // === WebGPU ===
  bool supports_webgpu = false;
  std::string webgpu_vendor;
  std::string webgpu_arch;
  std::string webgpu_device;
  std::string webgpu_description;

  // === CSS ===
  bool prefers_dark_mode = true;
  bool supports_p3_gamut = false;

  // === WebRTC ===
  std::string fake_local_ip;
};

class GhostProfileClient {
 public:
  static GhostProfileClient* Get();

  // Called once on renderer startup to fetch profile from browser process.
  void Initialize();

  // Returns the cached profile (fast, no IPC).
  const RendererProfile& GetProfile() const { return profile_; }

  // Returns true if ghost mode is active and profile is loaded.
  bool IsReady() const { return ready_; }

  // Performance timing helpers based on soc_tier.
  int GetCanvasMinMs() const;
  int GetCanvasMaxMs() const;
  int GetWebGLMinMs() const;
  int GetWebGLMaxMs() const;

  GhostProfileClient(const GhostProfileClient&) = delete;
  GhostProfileClient& operator=(const GhostProfileClient&) = delete;

 private:
  friend class base::NoDestructor<GhostProfileClient>;
  GhostProfileClient();
  ~GhostProfileClient();

  RendererProfile profile_;
  bool ready_ = false;
};

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_CHROMIUM_PATCHES_BLINK_GHOST_PROFILE_CLIENT_H_
