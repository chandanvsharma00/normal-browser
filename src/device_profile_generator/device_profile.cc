// Copyright 2024 Normal Browser Authors. All rights reserved.
// device_profile.cc — DeviceProfile serialization and helper implementations.

#include "device_profile_generator/device_profile.h"

#include <sstream>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace normal_browser {

// ============================================================
// String Conversion Helpers
// ============================================================

std::string ManufacturerToString(Manufacturer m) {
  switch (m) {
    case Manufacturer::kSamsung:   return "Samsung";
    case Manufacturer::kXiaomi:    return "Xiaomi";
    case Manufacturer::kRealme:    return "Realme";
    case Manufacturer::kOnePlus:   return "OnePlus";
    case Manufacturer::kVivo:      return "Vivo";
    case Manufacturer::kOPPO:      return "OPPO";
    case Manufacturer::kMotorola:  return "Motorola";
    case Manufacturer::kIQOO:      return "iQOO";
    case Manufacturer::kNothing:   return "Nothing";
    case Manufacturer::kGoogle:    return "Google";
    case Manufacturer::kTecno:     return "Tecno";
  }
  return "Unknown";
}

Manufacturer StringToManufacturer(const std::string& s) {
  if (s == "Samsung")   return Manufacturer::kSamsung;
  if (s == "Xiaomi")    return Manufacturer::kXiaomi;
  if (s == "Realme")    return Manufacturer::kRealme;
  if (s == "OnePlus")   return Manufacturer::kOnePlus;
  if (s == "Vivo")      return Manufacturer::kVivo;
  if (s == "OPPO")      return Manufacturer::kOPPO;
  if (s == "Motorola")  return Manufacturer::kMotorola;
  if (s == "iQOO")      return Manufacturer::kIQOO;
  if (s == "Nothing")   return Manufacturer::kNothing;
  if (s == "Google")    return Manufacturer::kGoogle;
  if (s == "Tecno")     return Manufacturer::kTecno;
  return Manufacturer::kSamsung;
}

std::string PriceTierToString(PriceTier t) {
  switch (t) {
    case PriceTier::kBudget:   return "Budget";
    case PriceTier::kMidRange: return "Mid-Range";
    case PriceTier::kFlagship: return "Flagship";
    case PriceTier::kPremium:  return "Premium";
  }
  return "Unknown";
}

// ============================================================
// DeviceProfile Derived Helpers
// ============================================================

int DeviceProfile::GetAvailHeight() const {
  int status_bar_px = static_cast<int>(24 * density);
  int nav_bar_px = static_cast<int>(48 * density);
  return screen_height - status_bar_px - nav_bar_px;
}

int DeviceProfile::GetCanvasMinMs() const {
  static const int kMin[] = {5, 12, 25, 50};
  return kMin[soc_tier];
}

int DeviceProfile::GetCanvasMaxMs() const {
  static const int kMax[] = {12, 25, 50, 100};
  return kMax[soc_tier];
}

int DeviceProfile::GetWebGLMinMs() const {
  static const int kMin[] = {3, 8, 15, 30};
  return kMin[soc_tier];
}

int DeviceProfile::GetWebGLMaxMs() const {
  static const int kMax[] = {8, 15, 30, 60};
  return kMax[soc_tier];
}

int DeviceProfile::GetCryptoMinMs() const {
  static const int kMin[] = {1, 3, 5, 10};
  return kMin[soc_tier];
}

int DeviceProfile::GetCryptoMaxMs() const {
  static const int kMax[] = {3, 5, 10, 20};
  return kMax[soc_tier];
}

// ============================================================
// TLSProfile Serialization
// ============================================================

base::Value::Dict TLSProfile::ToDict() const {
  base::Value::Dict dict;
  dict.Set("cipher_shuffle_seed", static_cast<int>(cipher_shuffle_seed));
  dict.Set("cipher_drop_count", cipher_drop_count);
  dict.Set("extension_shuffle_seed", static_cast<int>(extension_shuffle_seed));
  dict.Set("num_grease_extensions", num_grease_extensions);
  dict.Set("swap_x25519_p256", swap_x25519_p256);
  dict.Set("include_p521", include_p521);
  dict.Set("include_kyber", include_kyber);
  dict.Set("sig_alg_shuffle_seed", static_cast<int>(sig_alg_shuffle_seed));
  dict.Set("include_http2", include_http2);
  dict.Set("include_http3", include_http3);
  dict.Set("send_session_ticket", send_session_ticket);
  dict.Set("padding_target", padding_target);
  dict.Set("ja3_hash", ja3_hash);
  dict.Set("ja4_hash", ja4_hash);

  base::Value::List grease_c;
  for (int pos : grease_cipher_positions)
    grease_c.Append(pos);
  dict.Set("grease_cipher_positions", std::move(grease_c));

  base::Value::List grease_e;
  for (int pos : grease_extension_positions)
    grease_e.Append(pos);
  dict.Set("grease_extension_positions", std::move(grease_e));

  return dict;
}

TLSProfile TLSProfile::FromDict(const base::Value::Dict& dict) {
  TLSProfile t;
  if (auto v = dict.FindInt("cipher_shuffle_seed"))
    t.cipher_shuffle_seed = static_cast<uint32_t>(*v);
  if (auto v = dict.FindInt("cipher_drop_count"))
    t.cipher_drop_count = *v;
  if (auto v = dict.FindInt("extension_shuffle_seed"))
    t.extension_shuffle_seed = static_cast<uint32_t>(*v);
  if (auto v = dict.FindInt("num_grease_extensions"))
    t.num_grease_extensions = *v;
  if (auto v = dict.FindBool("swap_x25519_p256"))
    t.swap_x25519_p256 = *v;
  if (auto v = dict.FindBool("include_p521"))
    t.include_p521 = *v;
  if (auto v = dict.FindBool("include_kyber"))
    t.include_kyber = *v;
  if (auto v = dict.FindInt("sig_alg_shuffle_seed"))
    t.sig_alg_shuffle_seed = static_cast<uint32_t>(*v);
  if (auto v = dict.FindBool("include_http2"))
    t.include_http2 = *v;
  if (auto v = dict.FindBool("include_http3"))
    t.include_http3 = *v;
  if (auto v = dict.FindBool("send_session_ticket"))
    t.send_session_ticket = *v;
  if (auto v = dict.FindInt("padding_target"))
    t.padding_target = *v;
  if (auto* s = dict.FindString("ja3_hash"))
    t.ja3_hash = *s;
  if (auto* s = dict.FindString("ja4_hash"))
    t.ja4_hash = *s;

  if (const auto* list = dict.FindList("grease_cipher_positions")) {
    for (const auto& item : *list)
      if (item.is_int()) t.grease_cipher_positions.push_back(item.GetInt());
  }
  if (const auto* list = dict.FindList("grease_extension_positions")) {
    for (const auto& item : *list)
      if (item.is_int()) t.grease_extension_positions.push_back(item.GetInt());
  }

  return t;
}

// ============================================================
// DeviceProfile Serialization
// ============================================================

base::Value::Dict DeviceProfile::ToDict() const {
  base::Value::Dict dict;

  // Identity
  dict.Set("manufacturer_name", manufacturer_name);
  dict.Set("manufacturer_id", static_cast<int>(manufacturer));
  dict.Set("model", model);
  dict.Set("device_name", device_name);
  dict.Set("device_codename", device_codename);

  // Android Build
  dict.Set("android_version", android_version);
  dict.Set("sdk_version", sdk_version);
  dict.Set("build_id", build_id);
  dict.Set("build_fingerprint", build_fingerprint);
  dict.Set("build_display", build_display);
  dict.Set("security_patch", security_patch);
  dict.Set("android_id", android_id);
  dict.Set("serial_number", serial_number);
  dict.Set("bootloader", bootloader);

  // SoC
  dict.Set("price_tier", static_cast<int>(price_tier));
  dict.Set("soc_name", soc_name);
  dict.Set("soc_codename", soc_codename);
  dict.Set("cpu_cores", cpu_cores);
  dict.Set("cpu_architecture", cpu_architecture);
  dict.Set("hardware_string", hardware_string);
  dict.Set("soc_tier", soc_tier);

  // GPU
  dict.Set("gl_renderer", gl_renderer);
  dict.Set("gl_vendor", gl_vendor);
  dict.Set("gl_version", gl_version);
  dict.Set("gl_max_texture_size", gl_max_texture_size);
  dict.Set("gl_max_renderbuffer_size", gl_max_renderbuffer_size);
  dict.Set("gl_max_vertex_attribs", gl_max_vertex_attribs);
  dict.Set("gl_max_varying_vectors", gl_max_varying_vectors);
  dict.Set("gl_max_combined_texture_units", gl_max_combined_texture_units);
  dict.Set("gl_shading_language_version", gl_shading_language_version);

  base::Value::List ext_list;
  for (const auto& ext : gl_extensions)
    ext_list.Append(ext);
  dict.Set("gl_extensions", std::move(ext_list));

  // Screen
  dict.Set("screen_width", screen_width);
  dict.Set("screen_height", screen_height);
  dict.Set("density", static_cast<double>(density));
  dict.Set("dpi", dpi);

  // RAM
  dict.Set("ram_mb", ram_mb);
  dict.Set("device_memory_gb", static_cast<double>(device_memory_gb));

  // Cameras
  dict.Set("front_cameras", front_cameras);
  dict.Set("back_cameras", back_cameras);

  // Battery
  dict.Set("battery_level", static_cast<double>(battery_level));
  dict.Set("battery_charging", battery_charging);

  // Uptime
  dict.Set("uptime_seconds", uptime_seconds);

  // Network
  dict.Set("connection_type", static_cast<int>(connection_type));
  dict.Set("network_downlink_mbps", network_downlink_mbps);
  dict.Set("network_rtt_ms", static_cast<int>(network_rtt_ms));
  dict.Set("effective_type", effective_type);

  // Language
  dict.Set("primary_language", primary_language);
  base::Value::List lang_list;
  for (const auto& l : languages)
    lang_list.Append(l);
  dict.Set("languages", std::move(lang_list));

  // User Agent
  dict.Set("user_agent", user_agent);
  dict.Set("ch_ua", ch_ua);
  dict.Set("ch_ua_model", ch_ua_model);
  dict.Set("ch_ua_platform_version", ch_ua_platform_version);
  dict.Set("ch_ua_full_version_list", ch_ua_full_version_list);
  dict.Set("chrome_version", chrome_version);
  dict.Set("chrome_major_version", chrome_major_version);

  // Seeds
  dict.Set("canvas_noise_seed", static_cast<double>(canvas_noise_seed));
  dict.Set("audio_bias", static_cast<double>(audio_bias));
  dict.Set("webgl_noise_seed", static_cast<double>(webgl_noise_seed));
  dict.Set("client_rect_noise_seed",
           static_cast<double>(client_rect_noise_seed));
  dict.Set("sensor_noise_seed", static_cast<double>(sensor_noise_seed));
  dict.Set("math_seed", base::NumberToString(math_seed));

  // Fonts
  base::Value::List font_list;
  for (const auto& f : system_fonts)
    font_list.Append(f);
  dict.Set("system_fonts", std::move(font_list));
  dict.Set("emoji_font_name", emoji_font_name);
  dict.Set("system_ui_font", system_ui_font);

  // Sensors
  dict.Set("accelerometer_name", accelerometer_name);
  dict.Set("accelerometer_vendor", accelerometer_vendor);
  dict.Set("gyroscope_name", gyroscope_name);
  dict.Set("gyroscope_vendor", gyroscope_vendor);
  dict.Set("magnetometer_name", magnetometer_name);
  dict.Set("magnetometer_vendor", magnetometer_vendor);
  dict.Set("has_gyroscope", has_gyroscope);
  dict.Set("has_barometer", has_barometer);

  // TTS Voices
  base::Value::List voice_list;
  for (const auto& v : tts_voices) {
    base::Value::Dict vd;
    vd.Set("name", v.name);
    vd.Set("lang", v.lang);
    vd.Set("is_default", v.is_default);
    voice_list.Append(std::move(vd));
  }
  dict.Set("tts_voices", std::move(voice_list));

  // WebGPU
  dict.Set("supports_webgpu", supports_webgpu);
  dict.Set("webgpu_vendor", webgpu_vendor);
  dict.Set("webgpu_arch", webgpu_arch);
  dict.Set("webgpu_device", webgpu_device);
  dict.Set("webgpu_description", webgpu_description);

  // CSS
  dict.Set("prefers_dark_mode", prefers_dark_mode);
  dict.Set("supports_p3_gamut", supports_p3_gamut);

  // Storage
  dict.Set("storage_total_bytes",
           base::NumberToString(storage_total_bytes));
  dict.Set("storage_used_bytes",
           base::NumberToString(storage_used_bytes));

  // Performance Memory
  dict.Set("js_heap_size_limit",
           base::NumberToString(js_heap_size_limit));
  dict.Set("total_js_heap_size",
           base::NumberToString(total_js_heap_size));
  dict.Set("used_js_heap_size",
           base::NumberToString(used_js_heap_size));

  // WebRTC
  dict.Set("fake_local_ip", fake_local_ip);

  // TLS
  dict.Set("tls_profile", tls_profile.ToDict());

  return dict;
}

DeviceProfile DeviceProfile::FromDict(const base::Value::Dict& dict) {
  DeviceProfile p;

  // Identity
  if (auto* v = dict.FindString("manufacturer_name"))
    p.manufacturer_name = *v;
  if (auto v = dict.FindInt("manufacturer_id"))
    p.manufacturer = static_cast<Manufacturer>(*v);
  if (auto* v = dict.FindString("model")) p.model = *v;
  if (auto* v = dict.FindString("device_name")) p.device_name = *v;
  if (auto* v = dict.FindString("device_codename")) p.device_codename = *v;

  // Android Build
  if (auto v = dict.FindInt("android_version")) p.android_version = *v;
  if (auto v = dict.FindInt("sdk_version")) p.sdk_version = *v;
  if (auto* v = dict.FindString("build_id")) p.build_id = *v;
  if (auto* v = dict.FindString("build_fingerprint"))
    p.build_fingerprint = *v;
  if (auto* v = dict.FindString("build_display")) p.build_display = *v;
  if (auto* v = dict.FindString("security_patch")) p.security_patch = *v;
  if (auto* v = dict.FindString("android_id")) p.android_id = *v;
  if (auto* v = dict.FindString("serial_number")) p.serial_number = *v;
  if (auto* v = dict.FindString("bootloader")) p.bootloader = *v;

  // SoC
  if (auto v = dict.FindInt("price_tier"))
    p.price_tier = static_cast<PriceTier>(*v);
  if (auto* v = dict.FindString("soc_name")) p.soc_name = *v;
  if (auto* v = dict.FindString("soc_codename")) p.soc_codename = *v;
  if (auto v = dict.FindInt("cpu_cores")) p.cpu_cores = *v;
  if (auto* v = dict.FindString("cpu_architecture"))
    p.cpu_architecture = *v;
  if (auto* v = dict.FindString("hardware_string")) p.hardware_string = *v;
  if (auto v = dict.FindInt("soc_tier")) p.soc_tier = *v;

  // GPU
  if (auto* v = dict.FindString("gl_renderer")) p.gl_renderer = *v;
  if (auto* v = dict.FindString("gl_vendor")) p.gl_vendor = *v;
  if (auto* v = dict.FindString("gl_version")) p.gl_version = *v;
  if (auto v = dict.FindInt("gl_max_texture_size"))
    p.gl_max_texture_size = *v;
  if (auto v = dict.FindInt("gl_max_renderbuffer_size"))
    p.gl_max_renderbuffer_size = *v;
  if (auto v = dict.FindInt("gl_max_vertex_attribs"))
    p.gl_max_vertex_attribs = *v;
  if (auto v = dict.FindInt("gl_max_varying_vectors"))
    p.gl_max_varying_vectors = *v;
  if (auto v = dict.FindInt("gl_max_combined_texture_units"))
    p.gl_max_combined_texture_units = *v;
  if (auto* v = dict.FindString("gl_shading_language_version"))
    p.gl_shading_language_version = *v;
  if (const auto* list = dict.FindList("gl_extensions")) {
    for (const auto& item : *list)
      if (item.is_string()) p.gl_extensions.push_back(item.GetString());
  }

  // Screen
  if (auto v = dict.FindInt("screen_width")) p.screen_width = *v;
  if (auto v = dict.FindInt("screen_height")) p.screen_height = *v;
  if (auto v = dict.FindDouble("density")) p.density = static_cast<float>(*v);
  if (auto v = dict.FindInt("dpi")) p.dpi = *v;

  // RAM
  if (auto v = dict.FindInt("ram_mb")) p.ram_mb = *v;
  if (auto v = dict.FindDouble("device_memory_gb"))
    p.device_memory_gb = static_cast<float>(*v);

  // Cameras
  if (auto v = dict.FindInt("front_cameras")) p.front_cameras = *v;
  if (auto v = dict.FindInt("back_cameras")) p.back_cameras = *v;

  // Battery
  if (auto v = dict.FindDouble("battery_level"))
    p.battery_level = static_cast<float>(*v);
  if (auto v = dict.FindBool("battery_charging")) p.battery_charging = *v;

  // Uptime
  if (auto v = dict.FindInt("uptime_seconds")) p.uptime_seconds = *v;

  // Network
  if (auto v = dict.FindInt("connection_type"))
    p.connection_type = static_cast<ConnectionType>(*v);
  if (auto v = dict.FindDouble("network_downlink_mbps"))
    p.network_downlink_mbps = *v;
  if (auto v = dict.FindInt("network_rtt_ms"))
    p.network_rtt_ms = static_cast<unsigned long>(*v);
  if (auto* v = dict.FindString("effective_type")) p.effective_type = *v;

  // Language
  if (auto* v = dict.FindString("primary_language"))
    p.primary_language = *v;
  if (const auto* list = dict.FindList("languages")) {
    for (const auto& item : *list)
      if (item.is_string()) p.languages.push_back(item.GetString());
  }

  // User Agent
  if (auto* v = dict.FindString("user_agent")) p.user_agent = *v;
  if (auto* v = dict.FindString("ch_ua")) p.ch_ua = *v;
  if (auto* v = dict.FindString("ch_ua_model")) p.ch_ua_model = *v;
  if (auto* v = dict.FindString("ch_ua_platform_version"))
    p.ch_ua_platform_version = *v;
  if (auto* v = dict.FindString("ch_ua_full_version_list"))
    p.ch_ua_full_version_list = *v;
  if (auto* v = dict.FindString("chrome_version")) p.chrome_version = *v;
  if (auto v = dict.FindInt("chrome_major_version"))
    p.chrome_major_version = *v;

  // Seeds
  if (auto v = dict.FindDouble("canvas_noise_seed"))
    p.canvas_noise_seed = static_cast<float>(*v);
  if (auto v = dict.FindDouble("audio_bias"))
    p.audio_bias = static_cast<float>(*v);
  if (auto v = dict.FindDouble("webgl_noise_seed"))
    p.webgl_noise_seed = static_cast<float>(*v);
  if (auto v = dict.FindDouble("client_rect_noise_seed"))
    p.client_rect_noise_seed = static_cast<float>(*v);
  if (auto v = dict.FindDouble("sensor_noise_seed"))
    p.sensor_noise_seed = static_cast<float>(*v);
  if (auto* v = dict.FindString("math_seed"))
    base::StringToUint64(*v, &p.math_seed);

  // Fonts
  if (const auto* list = dict.FindList("system_fonts")) {
    for (const auto& item : *list)
      if (item.is_string()) p.system_fonts.push_back(item.GetString());
  }
  if (auto* v = dict.FindString("emoji_font_name")) p.emoji_font_name = *v;
  if (auto* v = dict.FindString("system_ui_font")) p.system_ui_font = *v;

  // Sensors
  if (auto* v = dict.FindString("accelerometer_name"))
    p.accelerometer_name = *v;
  if (auto* v = dict.FindString("accelerometer_vendor"))
    p.accelerometer_vendor = *v;
  if (auto* v = dict.FindString("gyroscope_name")) p.gyroscope_name = *v;
  if (auto* v = dict.FindString("gyroscope_vendor"))
    p.gyroscope_vendor = *v;
  if (auto* v = dict.FindString("magnetometer_name"))
    p.magnetometer_name = *v;
  if (auto* v = dict.FindString("magnetometer_vendor"))
    p.magnetometer_vendor = *v;
  if (auto v = dict.FindBool("has_gyroscope")) p.has_gyroscope = *v;
  if (auto v = dict.FindBool("has_barometer")) p.has_barometer = *v;

  // TTS Voices
  if (const auto* list = dict.FindList("tts_voices")) {
    for (const auto& item : *list) {
      if (item.is_dict()) {
        DeviceProfile::VoiceInfo vi;
        if (auto* n = item.GetDict().FindString("name")) vi.name = *n;
        if (auto* l = item.GetDict().FindString("lang")) vi.lang = *l;
        if (auto d = item.GetDict().FindBool("is_default"))
          vi.is_default = *d;
        p.tts_voices.push_back(std::move(vi));
      }
    }
  }

  // WebGPU
  if (auto v = dict.FindBool("supports_webgpu")) p.supports_webgpu = *v;
  if (auto* v = dict.FindString("webgpu_vendor")) p.webgpu_vendor = *v;
  if (auto* v = dict.FindString("webgpu_arch")) p.webgpu_arch = *v;
  if (auto* v = dict.FindString("webgpu_device")) p.webgpu_device = *v;
  if (auto* v = dict.FindString("webgpu_description"))
    p.webgpu_description = *v;

  // CSS
  if (auto v = dict.FindBool("prefers_dark_mode")) p.prefers_dark_mode = *v;
  if (auto v = dict.FindBool("supports_p3_gamut")) p.supports_p3_gamut = *v;

  // Storage
  if (auto* v = dict.FindString("storage_total_bytes"))
    base::StringToInt64(*v, &p.storage_total_bytes);
  if (auto* v = dict.FindString("storage_used_bytes"))
    base::StringToInt64(*v, &p.storage_used_bytes);

  // Performance Memory
  if (auto* v = dict.FindString("js_heap_size_limit"))
    base::StringToInt64(*v, &p.js_heap_size_limit);
  if (auto* v = dict.FindString("total_js_heap_size"))
    base::StringToInt64(*v, &p.total_js_heap_size);
  if (auto* v = dict.FindString("used_js_heap_size"))
    base::StringToInt64(*v, &p.used_js_heap_size);

  // WebRTC
  if (auto* v = dict.FindString("fake_local_ip")) p.fake_local_ip = *v;

  // TLS
  if (const auto* tls_dict = dict.FindDict("tls_profile"))
    p.tls_profile = TLSProfile::FromDict(*tls_dict);

  return p;
}

std::string DeviceProfile::ToJSON() const {
  std::string json;
  base::JSONWriter::WriteWithOptions(
      ToDict(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  return json;
}

DeviceProfile DeviceProfile::FromJSON(const std::string& json) {
  auto parsed = base::JSONReader::ReadAndReturnValueWithError(json);
  if (!parsed.has_value() || !parsed->is_dict()) {
    LOG(ERROR) << "Failed to parse DeviceProfile JSON";
    return DeviceProfile();
  }
  return FromDict(parsed->GetDict());
}

}  // namespace normal_browser
