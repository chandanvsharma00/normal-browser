// Copyright 2024 Normal Browser Authors. All rights reserved.
// ghost_profile_host.cc — Browser-side Mojo host implementation.
//
// Maps DeviceProfile (device_profile.h) → GhostDeviceProfile (mojom).
// The mojom field names match RendererProfile in ghost_profile_client.h.

#include "content/browser/ghost_profile_host.h"

#include "device_profile_generator/device_profile.h"
#include "device_profile_generator/gpu_database.h"
#include "device_profile_generator/profile_store.h"

namespace normal_browser {

GhostProfileHost::GhostProfileHost() = default;
GhostProfileHost::~GhostProfileHost() = default;

void GhostProfileHost::BindReceiver(
    mojo::PendingReceiver<content::mojom::GhostProfileProvider> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void GhostProfileHost::GetProfile(GetProfileCallback callback) {
  std::move(callback).Run(BuildMojoProfile());
}

void GhostProfileHost::RendererReady() {
  // No-op for now. Can be used for sequencing.
}

content::mojom::GhostDeviceProfilePtr GhostProfileHost::BuildMojoProfile() {
  const auto& p = DeviceProfileStore::Get()->GetProfile();
  auto m = content::mojom::GhostDeviceProfile::New();

  // === Identity ===
  m->manufacturer = ManufacturerToString(p.manufacturer);
  m->model = p.model;
  m->device_name = p.device_name;
  m->build_fingerprint = p.build_fingerprint;
  m->build_display = p.build_display;
  m->build_id = p.build_id;
  m->android_version = p.android_version;
  m->sdk_version = p.sdk_version;
  m->security_patch = p.security_patch;
  m->android_id = p.android_id;
  m->serial_number = p.serial_number;

  // === User-Agent & Client Hints ===
  m->user_agent = p.user_agent;
  m->ch_ua = p.ch_ua;
  m->ch_ua_model = p.ch_ua_model;
  m->ch_ua_platform_version = p.ch_ua_platform_version;
  m->ch_ua_full_version_list = p.ch_ua_full_version_list;

  // === Hardware ===
  m->cpu_cores = p.cpu_cores;
  m->hardware_string = p.hardware_string;
  m->soc_name = p.soc_name;
  m->soc_tier = p.soc_tier;
  m->ram_mb = p.ram_mb;
  m->device_memory_gb = p.device_memory_gb;

  // === Screen ===
  m->screen_width = p.screen_width;
  m->screen_height = p.screen_height;
  m->density = p.density;
  m->dpi = p.dpi;

  // === GPU / WebGL ===
  m->gl_renderer = p.gl_renderer;
  m->gl_vendor = p.gl_vendor;
  m->gl_version = p.gl_version;
  m->gl_shading_language_version = p.gl_shading_language_version;
  m->gl_extensions = p.gl_extensions;
  m->gl_max_texture_size = p.gl_max_texture_size;
  m->gl_max_renderbuffer_size = p.gl_max_renderbuffer_size;
  m->gl_max_viewport_dims = {
      p.gl_max_viewport_dims[0], p.gl_max_viewport_dims[1]};
  m->gl_max_combined_texture_units = p.gl_max_combined_texture_units;
  m->gl_max_vertex_attribs = p.gl_max_vertex_attribs;
  m->gl_max_varying_vectors = p.gl_max_varying_vectors;
  m->gl_aliased_point_size = {
      p.gl_aliased_point_size[0], p.gl_aliased_point_size[1]};
  m->gl_aliased_line_width = {
      p.gl_aliased_line_width[0], p.gl_aliased_line_width[1]};

  // === Audio ===
  // DeviceProfile doesn't store audio_sample_rate/buffer_size directly.
  // Derive from the SoC database using the profile's soc_codename.
  m->audio_sample_rate = 48000;   // Default
  m->audio_buffer_size = 256;     // Default
  const auto* soc = FindSoCProfile(p.soc_codename);
  if (soc) {
    m->audio_sample_rate = soc->preferred_sample_rate;
    m->audio_buffer_size = soc->preferred_buffer_size;
  }

  // === Fingerprint Seeds ===
  m->canvas_noise_seed = p.canvas_noise_seed;
  m->audio_context_seed = p.audio_bias;  // DeviceProfile calls it audio_bias
  m->webgl_noise_seed = p.webgl_noise_seed;
  m->client_rect_noise_seed = p.client_rect_noise_seed;
  m->sensor_noise_seed = p.sensor_noise_seed;
  m->math_seed = p.math_seed;

  // === Language ===
  m->primary_language = p.primary_language;
  m->languages = p.languages;

  // === Camera ===
  m->front_cameras = p.front_cameras;
  m->back_cameras = p.back_cameras;

  // === Battery ===
  m->battery_level = p.battery_level;
  m->battery_charging = p.battery_charging;

  // === Network ===
  // DeviceProfile connection_type is an enum, Mojo expects string.
  switch (p.connection_type) {
    case ConnectionType::kWifi:
      m->connection_type = "wifi";
      break;
    case ConnectionType::k4G:
      m->connection_type = "4g";
      break;
  }
  m->network_downlink_mbps = p.network_downlink_mbps;
  m->network_rtt_ms = p.network_rtt_ms;
  m->effective_type = p.effective_type;

  // === Storage & Memory ===
  m->storage_total_bytes = p.storage_total_bytes;
  m->storage_used_bytes = p.storage_used_bytes;
  m->js_heap_size_limit = p.js_heap_size_limit;
  m->total_js_heap_size = p.total_js_heap_size;
  m->used_js_heap_size = p.used_js_heap_size;

  // === Fonts ===
  m->system_fonts = p.system_fonts;
  m->system_ui_font = p.system_ui_font;
  m->emoji_font_name = p.emoji_font_name;

  // === Sensors ===
  m->accelerometer_name = p.accelerometer_name;
  m->accelerometer_vendor = p.accelerometer_vendor;
  m->gyroscope_name = p.gyroscope_name;
  m->gyroscope_vendor = p.gyroscope_vendor;
  m->magnetometer_name = p.magnetometer_name;
  m->magnetometer_vendor = p.magnetometer_vendor;
  m->has_gyroscope = p.has_gyroscope;
  m->has_barometer = p.has_barometer;

  // === TTS Voices ===
  // DeviceProfile.tts_voices is vector<VoiceInfo>, Mojo sends vector<string>.
  // Flatten to "name|lang" strings for the renderer to parse.
  m->tts_voices.reserve(p.tts_voices.size());
  for (const auto& voice : p.tts_voices) {
    m->tts_voices.push_back(voice.name + "|" + voice.lang);
  }

  // === WebGPU ===
  m->supports_webgpu = p.supports_webgpu;
  m->webgpu_vendor = p.webgpu_vendor;
  m->webgpu_arch = p.webgpu_arch;
  m->webgpu_device = p.webgpu_device;
  m->webgpu_description = p.webgpu_description;

  // === CSS ===
  m->prefers_dark_mode = p.prefers_dark_mode;
  m->supports_p3_gamut = p.supports_p3_gamut;

  // === WebRTC ===
  m->fake_local_ip = p.fake_local_ip;

  return m;
}

}  // namespace normal_browser
