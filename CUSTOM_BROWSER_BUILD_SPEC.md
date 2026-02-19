# Custom Chromium Anti-Fingerprint Android Browser — Build Specification

## PROJECT GOAL
Build a modified Chromium-based Android browser from source that generates a completely new, realistic device identity on every fresh launch — with **UNLIMITED unique devices** generated procedurally (NOT from a fixed list). All fingerprint values must be changed at the **C++ / native layer** — NOT via JavaScript injection — so that fingerprinting services like FingerprintJS Pro AND Cloudflare Bot Management cannot detect tampering, because:
- `Object.getOwnPropertyDescriptor()` returns normal data descriptors (not getters)
- `Function.prototype.toString()` returns `[native code]` naturally
- No JS prototype pollution or hook artifacts exist
- Canvas, WebGL, Audio fingerprints are genuinely different (rendered differently), not post-processed
- HTTP headers (User-Agent, Sec-CH-UA-*) match JS-reported values perfectly
- **TLS ClientHello is randomized per session** — unique JA3/JA4 hash every time, defeating Cloudflare fingerprinting
- Device profiles are **procedurally generated** — infinite unique combinations, never repeats

---

## ARCHITECTURE OVERVIEW

```
chromium/
├── device_profile_generator/              ← NEW MODULE: Procedural device identity
│   ├── profile_generator.h/cc             ← Procedural generation engine (infinite devices)
│   ├── profile_store.h/cc                 ← Stores current session profile in memory
│   ├── manufacturer_traits.h/cc           ← Manufacturer-specific rules & constraints
│   ├── gpu_database.h/cc                  ← SoC→GPU mapping (Snapdragon, Dimensity, Exynos, Tensor)
│   ├── model_name_generator.h/cc          ← Realistic model number generator per OEM
│   ├── build_fingerprint_generator.h/cc   ← Build ID / fingerprint string generator
│   └── BUILD.gn                           ← Build config
├── tls_randomizer/                        ← NEW MODULE: Cloudflare bypass
│   ├── ja3_randomizer.h/cc                ← JA3/JA4 hash randomization engine
│   ├── cipher_suite_shuffler.h/cc         ← Cipher suite order + selection randomizer
│   ├── extension_randomizer.h/cc          ← TLS extension order + GREASE injection
│   ├── tls_profile_database.h/cc          ← Real browser TLS profiles to mimic
│   └── BUILD.gn                           ← Build config
├── content/browser/                       ← MODIFY: Feed profile to renderer
├── third_party/blink/renderer/            ← MODIFY: Navigator, Screen, Canvas, WebGL, Audio
├── third_party/boringssl/                 ← MODIFY: TLS ClientHello randomization
├── net/http/                              ← MODIFY: User-Agent header generation
├── net/socket/                            ← MODIFY: TLS handshake parameters
├── components/embedder_support/           ← MODIFY: UA client hints
└── chrome/android/                        ← MODIFY: Android Chrome shell + UI
```

---

## PART 1: PROCEDURAL DEVICE PROFILE GENERATOR (UNLIMITED DEVICES)

### Design Philosophy: NO Fixed Device List

Instead of a database of 50 devices that could eventually be fingerprinted as "one of your 50", the generator **procedurally creates infinite unique devices** by combining real manufacturer rules, SoC constraints, and market data. Every generated device is unique but **physically plausible** — it follows the exact naming conventions, hardware pairings, and spec ranges that real OEMs use.

### File: `device_profile_generator/manufacturer_traits.h`

Defines per-manufacturer **rules** (NOT specific models) for procedural generation:

```cpp
struct ManufacturerTraits {
  std::string name;                          // "Samsung", "Xiaomi", etc.
  float market_weight;                       // Probability weight (Samsung=0.28, Xiaomi=0.22, etc.)
  
  // Model number generation rules
  std::string model_prefix_pattern;          // Regex-like pattern for model numbers
  std::vector<std::string> series_prefixes;  // e.g., Samsung: ["SM-S", "SM-A", "SM-M", "SM-F", "SM-E"]
  std::vector<std::string> series_names;     // e.g., ["Galaxy S", "Galaxy A", "Galaxy M", "Galaxy F"]
  int model_number_digits;                   // Samsung: 3-4 digits, Xiaomi: 10 chars, etc.
  std::string region_suffix;                 // "B" for India variants
  
  // Build fingerprint format
  std::string build_fp_format;               // Format string for android.os.Build.FINGERPRINT
  std::string build_display_format;          // Format for Build.DISPLAY
  
  // SoCs this manufacturer uses (weighted)
  std::vector<std::pair<std::string, float>> compatible_socs;  
  // e.g., Samsung: {"snapdragon_8_gen3", 0.15}, {"snapdragon_7_gen1", 0.20}, {"exynos_1380", 0.25}, ...
  
  // Display ranges
  struct ScreenRange {
    int min_width, max_width;     // 720-1440
    int min_height, max_height;   // 1600-3200  
    std::vector<float> common_densities;  // {2.0, 2.625, 2.75, 3.0, 3.5}
  } screen_range;
  
  // RAM ranges  
  std::vector<int> possible_ram_mb;  // {3072, 4096, 6144, 8192, 12288, 16384}
  
  // Camera configurations
  std::vector<std::pair<int,int>> camera_configs;  // {front, back} combos: {1,2}, {1,3}, {1,4}, {2,4}
  
  // Manufacturer-specific fonts
  std::vector<std::string> custom_fonts;  // Samsung: SamsungOne, SECRobotoLight...
  
  // Sensor vendors this OEM uses
  std::vector<std::string> sensor_vendors;  // ["LSM6DSO", "BMI260", "ICM-42607"]
};
```

### Manufacturer Rules (10 OEMs with weighted probability):

```
Manufacturer       | Weight | Model Pattern          | Series Names
─────────────────────────────────────────────────────────────────────
Samsung            | 0.28   | SM-{S|A|M|F|E}{3-4d}B | Galaxy S/A/M/F/Z
Xiaomi             | 0.22   | {5d}{2c}{4d}{2c}       | Xiaomi/Redmi Note/Redmi/POCO
OnePlus            | 0.08   | CPH{4d} / NE{4d}      | OnePlus/Nord
Realme             | 0.10   | RMX{4d}                | Realme/Narzo/GT
Vivo               | 0.08   | V{4d}                  | Vivo V/Y/T/X
OPPO               | 0.06   | CPH{4d}                | Reno/A/F
Motorola           | 0.05   | XT{4d}-{1d}            | Moto G/Edge/Razr
iQOO               | 0.04   | V{4d} / I{4d}          | iQOO/Neo/Z
Nothing            | 0.03   | A{3d}                  | Phone/CMF Phone
Google             | 0.03   | G{4c}                  | Pixel
Tecno              | 0.03   | {2c}{1d}{1c}           | Camon/Spark/Pova
```

### File: `device_profile_generator/gpu_database.h`

Maps SoC → GPU with **exact real specs** (this is a lookup, not procedural, because GPU specs must be accurate):

```cpp
struct SoCProfile {
  std::string soc_name;           // "Snapdragon 8 Gen 3"
  std::string soc_codename;       // "sm8650"
  int cpu_cores;                  // 8
  std::string cpu_architecture;   // "1x Cortex-X4 + 3x Cortex-A720 + 4x Cortex-A520"
  std::string hardware_string;    // "qcom" / "mt6789" / "exynos"
  
  // GPU (must be exact — fingerprinting services validate these)
  std::string gl_renderer;        // "Adreno (TM) 750"
  std::string gl_vendor;          // "Qualcomm"
  std::string gl_version;         // "OpenGL ES 3.2 V@0702.0"
  int gl_max_texture_size;        // 16384
  int gl_max_renderbuffer_size;   // 16384
  int gl_max_viewport_dims[2];    // {16384, 16384}
  int gl_max_vertex_attribs;      // 32
  int gl_max_varying_vectors;     // 32
  int gl_max_combined_texture_units; // 96
  float gl_aliased_line_width[2]; // {1.0, 1.0}
  float gl_aliased_point_size[2]; // {1.0, 1024.0}
  std::vector<std::string> gl_extensions;  // Full extension list (30-80 extensions)
  std::string gl_shading_language_version; // "OpenGL ES GLSL ES 3.20"
  
  // Audio characteristics
  int preferred_sample_rate;      // 48000
  int preferred_buffer_size;      // 256 or 512
};
```

### SoC Database (25+ SoCs covering all price tiers):

```
Flagship:    Snapdragon 8 Gen 3, 8 Gen 2, 8 Gen 1, Dimensity 9300, 9200, Exynos 2400, 2200, Tensor G3, G4
Upper-Mid:   Snapdragon 7+ Gen 2, 7 Gen 1, Dimensity 8300, 8200, 8100, Exynos 1380
Mid-Range:   Snapdragon 6 Gen 1, 695, Dimensity 7200, 7050, 6100+, Helio G99
Budget:      Snapdragon 4 Gen 2, 4 Gen 1, Dimensity 6080, 6020, Helio G85, G36
Each with EXACT GPU specs: Adreno 750/740/730/725/720/710/619/618/616, Mali-G78/G710/G615/G610/G57, 
                           Immortalis-G720/G715, PowerVR BXM-8-256
```

### File: `device_profile_generator/model_name_generator.h`

Generates realistic model numbers that follow OEM conventions:

```cpp
class ModelNameGenerator {
public:
  // Samsung: SM-{series}{3-4 digits}{region}
  // Series: S=flagship, A=mid, M=online, F=flipkart, E=economy
  // Digits: hundred-series indicates year (9xx=2024, 8xx=2023, 7xx=2022, etc.)
  // Region: B=India/global
  static std::string GenerateSamsung(int tier, int year) {
    // tier 0=flagship, 1=upper-mid, 2=mid, 3=budget
    // Returns e.g., "SM-S936B", "SM-A556B", "SM-M357B"
    char series = "SAMFE"[tier];
    int base = (2025 - year) * 100 + RandInt(10, 99);
    return StringPrintf("SM-%c%dB", series, base);
  }
  
  // Xiaomi/Redmi: {2-digit year}{2-digit month}{model}{region}
  // e.g., "24031PN0DC" for a 2024 March model
  static std::string GenerateXiaomi(int year, int month) {
    char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return StringPrintf("%02d%02d%d%c%c%02d%c", 
      year % 100, month, RandInt(1,9), 
      chars[RandInt(0,25)], chars[RandInt(0,25)],
      RandInt(0,99), "GIBY"[RandInt(0,3)]);
  }
  
  // OnePlus: CPH{4 digits} or NE{4 digits}
  static std::string GenerateOnePlus() {
    return StringPrintf("%s%04d", RandBool() ? "CPH" : "NE", RandInt(2400, 2999));
  }
  
  // Realme: RMX{4 digits}
  static std::string GenerateRealme() {
    return StringPrintf("RMX%04d", RandInt(3600, 3999));
  }
  
  // Vivo: V{4 digits}
  static std::string GenerateVivo() {
    return StringPrintf("V%04d", RandInt(2200, 2599));
  }
  
  // OPPO: CPH{4 digits}
  static std::string GenerateOPPO() {
    return StringPrintf("CPH%04d", RandInt(2400, 2799));
  }
  
  // Motorola: XT{4 digits}-{1 digit}
  static std::string GenerateMotorola() {
    return StringPrintf("XT%04d-%d", RandInt(2300, 2499), RandInt(1, 4));
  }
  
  // Nothing: A{3 digits}
  static std::string GenerateNothing() {
    return StringPrintf("A%03d", RandInt(60, 120));
  }
  
  // Google Pixel: G{4 chars}
  static std::string GeneratePixel() {
    char c[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    return StringPrintf("G%c%c%c%c", c[RandInt(0,35)], c[RandInt(0,35)],
                        c[RandInt(0,35)], c[RandInt(0,35)]);
  }
};
```

### File: `device_profile_generator/build_fingerprint_generator.h`

Generates realistic `android.os.Build.FINGERPRINT` strings:

```cpp
// Samsung format:
// samsung/{device_code}xx{region}1{build_tag}/{device_code}:{api}/{build_id}/{date}:user/release-keys
// Example: samsung/s936bxxu1axl3/s936b:14/UP1A.231005.007/S936BXXU1AXL3:user/release-keys

// Xiaomi format:
// Xiaomi/{marketing_name}/{codename}:{api}/{build_id}/{date}:user/release-keys

// Procedural build IDs:
static std::string GenerateBuildId(int android_ver) {
  // Real AOSP build ID format: {TAG}.{YYMMDD}.{SEQ}
  // Android 14: UP1A.YYMMDD.NNN or AP2A.YYMMDD.NNN  
  // Android 13: TQ3A.YYMMDD.NNN or TP1A.YYMMDD.NNN
  // Android 12: SQ3A.YYMMDD.NNN or SP1A.YYMMDD.NNN
  const char* prefixes_14[] = {"UP1A", "AP2A", "UD1A", "AD1A"};
  const char* prefixes_13[] = {"TQ3A", "TP1A", "TD1A"};
  const char* prefixes_12[] = {"SQ3A", "SP1A", "SD1A"};
  // Pick prefix based on version, random date in valid range, random seq
  int year = 2024 + RandInt(0, 1);
  int month = RandInt(1, 12);
  int day = RandInt(1, 28);
  return StringPrintf("%s.%02d%02d%02d.%03d", 
    prefix, year % 100, month, day, RandInt(1, 99));
}

// Security patch: always recent (within last 3 months)
static std::string GenerateSecurityPatch() {
  // Returns e.g., "2025-12-01", "2026-01-05"
  int year = 2025 + RandInt(0, 1);
  int month = RandInt(std::max(1, CurrentMonth()-3), CurrentMonth());
  return StringPrintf("%04d-%02d-01", year, month);
}
```

### File: `device_profile_generator/profile_generator.cc`

The **main generation engine** — creates infinite unique devices:

```cpp
struct DeviceProfile {
  // === PROCEDURALLY GENERATED (unique every time) ===
  std::string manufacturer;          // Weighted random from 11 OEMs
  std::string model;                 // Generated from OEM naming rules
  std::string device_name;           // "Galaxy S{nn} Ultra", "Redmi Note {n} Pro", etc.
  std::string build_id;              // Procedural AOSP build ID
  std::string build_fingerprint;     // Full Build.FINGERPRINT string
  std::string build_display;         // Build.DISPLAY string
  int android_version;               // Weighted: 14(50%), 13(30%), 12(15%), 15(5%)
  int sdk_version;                   // Derived from android_version
  std::string security_patch;        // Recent (within 3 months)
  std::string android_id;            // Random 16-char hex
  std::string serial_number;         // OEM-format serial
  
  // === FROM SoC DATABASE (validated real specs) ===
  std::string soc_name;              // "Snapdragon 8 Gen 3"
  int cpu_cores;                     // From SoC spec
  std::string hardware;              // "qcom" / "mt6789" / "exynos"
  std::string gl_renderer;           // From SoC→GPU mapping
  std::string gl_vendor;             // From SoC→GPU mapping  
  std::string gl_version;            // From SoC→GPU mapping
  std::vector<std::string> gl_extensions; // From SoC→GPU mapping
  
  // === PROCEDURALLY GENERATED (from manufacturer screen ranges) ===
  int screen_width;                  // Random from OEM's valid range
  int screen_height;                 // Proportional to width (16:9, 19.5:9, 20:9, etc.)
  float density;                     // From OEM's common densities
  int dpi;                           // Derived from density × 160
  
  // === PROCEDURALLY GENERATED (from ranges) ===
  int ram_mb;                        // Weighted from OEM's RAM options
  float audio_bias;                  // Random float ±0.0001 (unique audio fingerprint)
  float canvas_noise_seed;           // Random float (unique canvas fingerprint)
  float client_rect_noise_seed;      // Random float (unique element sizing)
  int front_cameras;                 // From OEM camera configs
  int back_cameras;                  // From OEM camera configs
  float battery_level;               // Random 15-95%
  bool battery_charging;             // Random bool
  int uptime_seconds;                // Random 1000-500000
  
  // === DERIVED (computed from above) ===
  std::string user_agent;            // Built from model + android_ver + chrome_ver
  std::string ch_ua_model;           // From model
  std::string ch_ua_platform_ver;    // From android_version
  std::string language;              // Weighted: en-IN(60%), en-US(20%), hi-IN(15%), en-GB(5%)
  std::vector<std::string> languages;// Derived from language
  
  // === TLS PROFILE (for Cloudflare bypass) ===
  TLSProfile tls_profile;            // Per-session unique TLS ClientHello config
  
  // === ADDED: Fields for Parts 14-22 ===
  
  // Math fingerprinting (Part 14)
  uint64_t math_seed;                // Seed for Math.* ULP perturbation
  
  // Emoji / Font (Part 15)
  std::string emoji_font_path;       // Path to OEM emoji TTF in APK assets
  std::string system_font_name;      // "SamsungOne" / "MiSans" / "Roboto" / etc.
  
  // Performance timing (Part 16)
  int soc_tier;                      // 0=flagship, 1=upper-mid, 2=mid, 3=budget
  // Methods:
  base::TimeDelta GetExpectedCanvasTime() const {
    // Flagship: 5-12ms, Upper: 12-25ms, Mid: 25-50ms, Budget: 50-100ms
    static const int kMinMs[] = {5, 12, 25, 50};
    static const int kMaxMs[] = {12, 25, 50, 100};
    return base::Milliseconds(kMinMs[soc_tier] + 
      RandInt(0, kMaxMs[soc_tier] - kMinMs[soc_tier]));
  }
  base::TimeDelta GetExpectedWebGLTime() const {
    static const int kMinMs[] = {3, 8, 15, 30};
    static const int kMaxMs[] = {8, 15, 30, 60};
    return base::Milliseconds(kMinMs[soc_tier] + 
      RandInt(0, kMaxMs[soc_tier] - kMinMs[soc_tier]));
  }
  base::TimeDelta GetExpectedCryptoTime() const {
    static const int kMinMs[] = {1, 3, 5, 10};
    static const int kMaxMs[] = {3, 5, 10, 20};
    return base::Milliseconds(kMinMs[soc_tier] + 
      RandInt(0, kMaxMs[soc_tier] - kMinMs[soc_tier]));
  }
  
  // Storage / Memory (Part 17)
  int64_t storage_total_bytes;       // 64/128/256/512 GB based on tier
  int64_t storage_used_bytes;        // 30-70% of total
  double network_downlink_mbps;      // Realistic: WiFi 20-80, 4G 5-20
  unsigned long network_rtt_ms;      // WiFi 5-20ms, 4G 30-80ms
  std::string connection_type;       // "wifi" or "4g"
  int64_t GetStorageBytes() const { return storage_total_bytes; }
  double GetNetworkDownlink() const { return network_downlink_mbps; }
  unsigned long GetNetworkRtt() const { return network_rtt_ms; }
  
  // Speech synthesis (Part 18)
  std::vector<std::string> tts_voices;  // Pre-built voice list for this OEM
  
  // WebGPU (Part 19)
  bool supports_webgpu;              // false for budget SoCs
  std::string webgpu_vendor;         // "qualcomm" / "arm" / "google"
  std::string webgpu_arch;           // "adreno-700" / "valhall" / "mali-g7xx"
  std::string webgpu_device;         // "adreno-750" / "immortalis-g720"
  std::string webgpu_description;    // Human-readable GPU name
  
  // CSS Media Queries (Part 20)
  bool prefers_dark_mode;            // 60% true, 40% false
  bool supports_p3_gamut;            // true for flagship, false for budget
  
  // Anti-bot markers (Part 22.5)
  // navigator.webdriver MUST be false (not undefined, not missing — false)
  // window.chrome object MUST exist with app, csi, loadTimes sub-objects
  // navigator.plugins MUST be empty PluginArray (length 0, mobile Chrome)
  // navigator.mimeTypes MUST be empty MimeTypeArray (length 0)
  // navigator.pdfViewerEnabled MUST be false (mobile)
  // navigator.doNotTrack MUST be null (default, not "1")
  // navigator.globalPrivacyControl MUST be undefined (not set)
  // 'ontouchstart' in window MUST be true (mobile)
  // navigator.permissions.query({name:'notifications'}) → 'prompt' (default)
};
```

### Generation Algorithm:

```
On every "New Identity" or first launch:

1. PICK MANUFACTURER → Weighted random (Samsung 28%, Xiaomi 22%, Realme 10%, ...)
2. PICK PRICE TIER   → Weighted random (budget 40%, mid 35%, flagship 15%, premium 10%)
3. PICK SoC          → From manufacturer's compatible SoCs for that tier
4. GENERATE MODEL    → Using OEM-specific model number rules
5. GENERATE NAME     → "Galaxy {series} {number}" or "Redmi Note {n} {suffix}"
6. PICK ANDROID VER  → Weighted (14>13>15>12), constrained by SoC year
7. GENERATE BUILD    → Procedural build ID + fingerprint + display
8. PICK SCREEN       → From manufacturer's screen ranges, snapped to real resolutions
9. PICK RAM          → From manufacturer's RAM options, constrained by tier
10. GENERATE SEEDS   → Random noise seeds for canvas, audio, clientRect, WebGL
11. GENERATE TLS     → Random JA3-valid TLS profile (see Part 14)
12. DERIVE ALL       → User-Agent, Client Hints, font list, sensor names, etc.
13. VALIDATE         → Ensure all values are internally consistent
14. STORE            → Save to ProfileStore singleton

Result: A device that has never existed before, but COULD exist.
         No two presses of "New Identity" will ever produce the same device.
         No fingerprinting service can build a list of "known fake devices".
```

### Mathematical Uniqueness:

```
Manufacturers:     11
× Model variants:  ~500 per OEM (procedural)
× SoCs:            25+
× Android versions: 4
× Build IDs:       ~10,000 (procedural date + seq)
× Screen combos:   ~30
× RAM options:     5-7
× Noise seeds:     2^128 (random floats)
× TLS profiles:    ~10,000+ (cipher/extension combos)
────────────────────────────────
Total combinations: Effectively INFINITE (>10^20)
Repeat probability: Essentially zero
```

---

## PART 2: NAVIGATOR PROPERTIES (Blink Renderer)

### Files to modify in `third_party/blink/renderer/core/frame/`

#### `navigator.cc` / `navigator_base.cc`

These report navigator properties. Modify to read from ProfileStore:

```
navigator.userAgent        → Build from profile (Linux; Android {ver}; {model})
navigator.platform         → "Linux armv8l" (always, for Android)
navigator.hardwareConcurrency → profile.cpu_cores
navigator.deviceMemory     → profile.ram_mb / 1024 (as double: 4, 6, 8, 12)
navigator.language         → Randomize from ["en-IN", "en-US", "hi-IN", "en-GB"]
navigator.languages        → Matching array like ["en-IN", "en", "hi"]
navigator.maxTouchPoints   → 5 (always, for mobile)
navigator.vendor           → "Google Inc." (always for Chrome)
navigator.appVersion       → Derived from userAgent
navigator.connection.effectiveType → Randomize "4g" or "wifi"
navigator.getBattery()     → Return profile battery level, charging: random bool
```

**CRITICAL**: These must remain **data properties** on the prototype, NOT accessor properties. Modify the **IDL bindings** or the C++ getter implementation directly so that the V8 binding layer exposes them as `value` descriptors, not `get/set`.

In `navigator.idl`:
```idl
// The binding already defines these as readonly attributes
// Ensure the C++ implementation returns the spoofed value directly
[HighEntropy] readonly attribute DOMString userAgent;
readonly attribute unsigned long long hardwareConcurrency;
```

In the C++ implementation (e.g., `NavigatorBase::hardwareConcurrency()`):
```cpp
unsigned int NavigatorBase::hardwareConcurrency() const {
  // OLD: return std::max(1u, base::SysInfo::NumberOfProcessors());
  // NEW: return from profile
  return DeviceProfileStore::Get()->GetProfile().cpu_cores;
}
```

---

## PART 3: SCREEN PROPERTIES

### Files to modify in `third_party/blink/renderer/core/frame/`

#### `screen.cc`

```cpp
int Screen::width() const {
  // Return profile screen width instead of real display
  return DeviceProfileStore::Get()->GetProfile().screen_width;
}

int Screen::height() const {
  return DeviceProfileStore::Get()->GetProfile().screen_height;
}

int Screen::availWidth() const {
  return DeviceProfileStore::Get()->GetProfile().screen_width;
}

int Screen::availHeight() const {
  // Subtract realistic status bar + nav bar height
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  int statusBarPx = static_cast<int>(24 * profile.density);  // 24dp
  int navBarPx = static_cast<int>(48 * profile.density);     // 48dp
  return profile.screen_height - statusBarPx - navBarPx;
}

int Screen::colorDepth() const {
  return 24;  // Always 24-bit on Android
}

float Screen::devicePixelRatio() const {
  return DeviceProfileStore::Get()->GetProfile().density;
}
```

---

## PART 4: CANVAS FINGERPRINTING

### Files to modify in `third_party/blink/renderer/modules/canvas/`

Canvas fingerprinting works by rendering specific text/shapes and reading back pixel data. To produce unique but **consistent-per-session** output:

#### Strategy: Per-session rendering perturbation at the Skia level

Modify `third_party/skia/src/core/SkCanvas.cpp` or the Blink canvas rendering pipeline:

```
1. On session start, generate a 256-byte noise seed from the device profile
2. When CanvasRenderingContext2D renders text:
   - Apply sub-pixel offset (±0.001 to ±0.01 pixels) to glyph positions
   - This must happen in the text shaping/rasterization layer (HarfBuzz or Skia)
   - The offset is deterministic per session (same seed = same offset)
3. When toDataURL() or toBlob() is called:
   - The output is naturally different because the rendering was different
   - No post-processing hooks needed — this is genuine rendering variation
4. Different GPU profiles produce subtly different anti-aliasing patterns naturally
```

#### Specific files:
- `third_party/skia/src/gpu/GrSurfaceDrawContext.cpp` — GPU-accelerated canvas
- `third_party/skia/src/core/SkScalerContext.cpp` — Font glyph rendering
- `third_party/blink/renderer/platform/fonts/shaping/` — Text shaping

**Key principle**: Don't modify the pixel data after rendering. Modify the rendering parameters (sub-pixel positioning, gamma, anti-aliasing threshold) so the output is genuinely different at the GPU level.

---

## PART 5: WEBGL FINGERPRINTING

### Files to modify:

#### `gpu/command_buffer/service/gles2_cmd_decoder.cc`
#### `gpu/config/gpu_info.cc`

```cpp
// Override GPU identification strings
std::string GpuInfo::GetGLRenderer() const {
  return DeviceProfileStore::Get()->GetProfile().gl_renderer;
  // e.g., "Adreno (TM) 750" instead of real GPU
}

std::string GpuInfo::GetGLVendor() const {
  return DeviceProfileStore::Get()->GetProfile().gl_vendor;
  // e.g., "Qualcomm" instead of real vendor
}

std::string GpuInfo::GetGLVersion() const {
  return DeviceProfileStore::Get()->GetProfile().gl_version;
}
```

#### WebGL Extensions
Return only the extensions that the spoofed GPU **actually supports**:
```
- Adreno 750: Full ES 3.2 + all extensions
- Mali-G78: Full ES 3.2 + ARM-specific extensions
- Mali-G68: ES 3.2 with slightly fewer extensions
- Adreno 619: ES 3.2 with fewer extensions
```

#### WebGL Rendering
Similar to canvas — modify shader compilation or precision mediump float precision to produce per-session unique WebGL renders without JS-level hooks.

Files:
- `gpu/command_buffer/service/shader_translator.cc` — Modify precision hints
- `third_party/angle/src/` — ANGLE OpenGL ES layer

---

## PART 6: AUDIO FINGERPRINTING

### Files to modify in `third_party/blink/renderer/modules/webaudio/`

#### `audio_context.cc` / `offline_audio_context.cc`

Audio fingerprinting creates an OscillatorNode + DynamicsCompressorNode and reads the output. To produce unique-per-session output:

```
1. Modify DynamicsCompressorNode processing with per-session bias:
   - Add ±0.00001 to ±0.0001 to the compressor's knee/threshold calculation
   - This is done in the C++ audio processing, not via JS override
   
2. Modify OscillatorNode with per-session phase offset:
   - Apply a sub-sample phase offset (< 1 sample) unique to the session
   
3. The AudioBuffer.getChannelData() returns genuinely different float arrays
   because the underlying processing was different — no hooks needed
```

#### Specific file:
- `third_party/blink/renderer/modules/webaudio/dynamics_compressor_kernel.cc`
  ```cpp
  // In processing loop, add session-specific micro-offset
  float sessionBias = DeviceProfileStore::Get()->GetProfile().audio_bias;
  output[i] += sessionBias * 0.00001f;
  ```

---

## PART 7: HTTP HEADERS

### Files to modify in `net/http/`

#### `http_util.cc` / User-Agent generation

```
Chromium builds the User-Agent string in multiple places:
1. content/common/user_agent.cc — BuildUserAgentFromProduct()
2. components/embedder_support/user_agent_utils.cc — GetUserAgent()
3. content/browser/renderer_host/navigator.cc — GetUserAgent()
```

Modify ALL of these to use ProfileStore:

```cpp
std::string GetUserAgent() {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  // Build: Mozilla/5.0 (Linux; Android {ver}; {model}) AppleWebKit/537.36 
  //        (KHTML, like Gecko) Chrome/{version} Mobile Safari/537.36
  return base::StringPrintf(
    "Mozilla/5.0 (Linux; Android %d; %s) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/%s Mobile Safari/537.36",
    profile.android_version,
    profile.model.c_str(),
    CHROME_VERSION_STRING
  );
}
```

### Client Hints (Sec-CH-UA-*) headers

#### `components/embedder_support/user_agent_utils.cc`

```cpp
// Sec-CH-UA-Platform: "Android"
// Sec-CH-UA-Mobile: ?1
// Sec-CH-UA-Model: "{model}" — from profile
// Sec-CH-UA-Platform-Version: "{android_version}.0.0" — from profile
// Sec-CH-UA-Full-Version-List: accurate Chrome version
```

These must be modified at the network stack level so they're sent with EVERY HTTP request, not just the ones JavaScript can see.

---

## PART 8: TIMEZONE

### Files to modify:

#### `base/i18n/icu_util.cc` or timezone resolution

```
Always report timezone as "Asia/Kolkata" (UTC+5:30).

Modify:
- ICU timezone default
- Intl.DateTimeFormat resolved timezone
- Date.prototype.getTimezoneOffset() → return -330
- Performance.timeOrigin (should reflect IST)
```

Files:
- `third_party/icu/source/common/putil.cpp` — `uprv_tzname()`
- `v8/src/date/dateparser.cc` — timezone offset resolution

---

## PART 9: FONT FINGERPRINTING

### Modify font enumeration

#### `third_party/blink/renderer/platform/fonts/font_cache_android.cc`

```
Android devices have different fonts depending on manufacturer:
- Samsung: SamsungOne, SECRobotoLight
- Xiaomi: MiLanPro, MiSans  
- OnePlus: OnePlusSans, SlateForOnePlus
- Stock Android: Roboto, NotoSans

Per-profile, modify:
1. Available font list (return only fonts the claimed device would have)
2. Font metrics (slightly different glyph widths per device/manufacturer)
3. System emoji font (Samsung vs Google emoji)
```

---

## PART 10: SENSOR FINGERPRINTING

### Files to modify in `services/device/generic_sensor/`

#### `platform_sensor_android.cc`

```
When JavaScript calls Accelerometer, Gyroscope, Magnetometer:
1. Report sensor hardware name from profile
2. Add per-session noise pattern (unique noise floor per "device")
3. Match sensor data rate to real device capabilities
```

---

## PART 11: MEDIA DEVICE ENUMERATION

### Files to modify:

#### `third_party/blink/renderer/modules/mediastream/media_devices.cc`

```
navigator.mediaDevices.enumerateDevices() should return:
- Number of cameras matching profile (front_cameras, back_cameras)
- Microphone with device-appropriate label
- Device IDs that are unique per session (random UUIDs)
```

---

## PART 12: CLIENT RECT / ELEMENT SIZING

### Files to modify in `third_party/blink/renderer/core/dom/`

```
Element.getBoundingClientRect() and getClientRects() are used for fingerprinting.
Add sub-pixel noise (±0.001px) at the layout level:

Modify: third_party/blink/renderer/core/layout/layout_box.cc
The noise must be consistent within a session but different across sessions.
```

---

## PART 13: WEBRTC LOCAL IP LEAK

### Files to modify:

```
WebRTC can leak the local IP address. Modify:
- content/browser/renderer_host/media/p2p/
- Generate a fake local IP per session: 192.168.{random}.{random}
- Or disable local candidate generation entirely
```

---

## PART 13.5: PROFILE STORE SINGLETON (Thread-Safe Global State)

### This is the CENTRAL component — every other Part reads from it.

#### File: `device_profile_generator/profile_store.h`

```cpp
#ifndef DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_
#define DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_

#include "base/no_destructor.h"
#include "base/synchronization/lock.h"
#include "device_profile_generator/profile_generator.h"

class DeviceProfileStore {
 public:
  static DeviceProfileStore* Get() {
    static base::NoDestructor<DeviceProfileStore> instance;
    return instance.get();
  }
  
  // Get current profile (thread-safe, read-only after generation)
  const DeviceProfile& GetProfile() const {
    return profile_;
  }
  
  // Generate new identity (called on startup or "New Identity" press)
  void GenerateNewProfile() {
    base::AutoLock lock(lock_);
    profile_ = ProfileGenerator::Generate();
    // Also regenerate TLS profile
    profile_.tls_profile = TLSProfileGenerator::Generate();
    // Notify observers (HTTP header updater, TLS stack, etc.)
    NotifyProfileChanged();
  }
  
  // Load persisted profile from SharedPreferences (Android)
  void LoadFromStorage();
  
  // Save current profile to SharedPreferences
  void SaveToStorage();
  
  // Check if a profile exists (first launch detection)
  bool HasStoredProfile() const;
  
 private:
  friend class base::NoDestructor<DeviceProfileStore>;
  DeviceProfileStore();
  ~DeviceProfileStore() = delete;
  
  void NotifyProfileChanged();
  
  DeviceProfile profile_;
  mutable base::Lock lock_;
};

#endif  // DEVICE_PROFILE_GENERATOR_PROFILE_STORE_H_
```

#### File: `device_profile_generator/profile_store.cc`

```cpp
#include "device_profile_generator/profile_store.h"
#include "base/android/jni_android.h"

DeviceProfileStore::DeviceProfileStore() {
  // On construction (app start), either load stored profile or generate new
  if (HasStoredProfile()) {
    LoadFromStorage();
  } else {
    GenerateNewProfile();
    SaveToStorage();
  }
}

void DeviceProfileStore::LoadFromStorage() {
  // Read from Android SharedPreferences via JNI
  // Key: "ghost_device_profile"
  // Value: JSON serialized DeviceProfile
  // This ensures the same identity persists across app restarts
  // until the user presses "New Identity"
}

void DeviceProfileStore::SaveToStorage() {
  // Write to Android SharedPreferences via JNI
  // Serialize DeviceProfile to JSON and store
}

bool DeviceProfileStore::HasStoredProfile() const {
  // Check if SharedPreferences contains "ghost_device_profile" key
}

void DeviceProfileStore::NotifyProfileChanged() {
  // Update HTTP header generator
  // Update TLS stack configuration
  // Invalidate any cached values in renderer processes
  // This uses Chromium's IPC to notify renderer processes
}
```

#### BUILD.gn for device_profile_generator/:

```gn
# device_profile_generator/BUILD.gn
source_set("device_profile_generator") {
  sources = [
    "profile_generator.h",
    "profile_generator.cc",
    "profile_store.h",
    "profile_store.cc",
    "manufacturer_traits.h",
    "manufacturer_traits.cc",
    "gpu_database.h",
    "gpu_database.cc",
    "model_name_generator.h",
    "model_name_generator.cc",
    "build_fingerprint_generator.h",
    "build_fingerprint_generator.cc",
    "profile_validator.h",
    "profile_validator.cc",
    "software_math.h",
    "software_math.cc",
  ]
  deps = [
    "//base",
    "//base:base_static",
  ]
}
```

#### How to wire ProfileStore into Chromium's startup:

```
1. In chrome/browser/chrome_browser_main.cc:
   - In PreMainMessageLoopRun():
     DeviceProfileStore::Get();  // Forces singleton creation + profile load

2. In content/browser/browser_main_loop.cc:
   - Pass profile to renderer processes via IPC on renderer creation

3. In third_party/blink/renderer/:
   - Each renderer reads profile from IPC shared memory
   - NOTE: Renderers run in sandboxed processes, they can't call
     SharedPreferences directly. The browser process must pass the
     profile to renderers via Mojo IPC.

4. Mojo interface for profile passing:
   // content/common/ghost_profile.mojom
   module content.mojom;
   struct GhostDeviceProfile {
     string user_agent;
     string model;
     int32 screen_width;
     int32 screen_height;
     float density;
     int32 cpu_cores;
     float device_memory_gb;
     string gl_renderer;
     string gl_vendor;
     string gl_version;
     // ... all fields
   };
   interface GhostProfileProvider {
     GetProfile() => (GhostDeviceProfile profile);
   };
```

---

## PART 13.6: ANTI-BOT / ANTI-AUTOMATION MARKERS

### CRITICAL — These are the FIRST things FPJS checks

FPJS Pro's bot detection checks for automation markers BEFORE doing any fingerprinting. If these are wrong, it immediately flags `bot: true`.

#### What FPJS checks and what we MUST return:

```cpp
// === FILE: third_party/blink/renderer/core/frame/navigator.cc ===

// 1. navigator.webdriver — MUST be false (not undefined)
//    Puppeteer/Selenium set this to true. Real Chrome = false.
//    FPJS checks: navigator.webdriver === false
bool Navigator::webdriver() const {
  return false;  // Always false — we are NOT automated
}

// 2. navigator.plugins — MUST be empty PluginArray on mobile
//    Desktop Chrome has PDF viewer plugin. Mobile Chrome = empty.
//    FPJS checks: navigator.plugins.length === 0 (mobile)
DOMPluginArray* Navigator::plugins() const {
  return MakeGarbageCollected<DOMPluginArray>();  // Empty, length 0
}

// 3. navigator.mimeTypes — MUST be empty on mobile
DOMMimeTypeArray* Navigator::mimeTypes() const {
  return MakeGarbageCollected<DOMMimeTypeArray>();  // Empty, length 0
}

// 4. navigator.pdfViewerEnabled — false on mobile Chrome
bool Navigator::pdfViewerEnabled() const {
  return false;
}

// 5. navigator.doNotTrack — null (default, user hasn't set it)
String Navigator::doNotTrack() const {
  return String();  // null/undefined — default state
}

// === FILE: third_party/blink/renderer/core/frame/local_dom_window.cc ===

// 6. window.chrome — MUST exist (it's Chrome-specific)
//    FPJS checks: typeof window.chrome !== 'undefined'
//    AND checks: window.chrome.app exists
//    AND checks: window.chrome.csi exists  
//    AND checks: window.chrome.loadTimes exists
//    Stock Chromium already has this, but verify it's not stripped.

// 7. 'ontouchstart' in window — MUST be true (mobile)
//    FPJS uses this to verify mobile claim
//    Stock Chromium on Android already has this, DO NOT remove it.

// 8. window.TouchEvent — MUST be defined (mobile)
//    FPJS checks: typeof TouchEvent !== 'undefined'

// === FILE: content/browser/permissions/ ===

// 9. Permissions defaults for fresh identity:
//    navigator.permissions.query({name:'notifications'}) → 'prompt'
//    navigator.permissions.query({name:'geolocation'}) → 'prompt'
//    navigator.permissions.query({name:'camera'}) → 'prompt'
//    All permissions must be in 'prompt' state (not 'granted' or 'denied')
//    This is natural for a fresh browser — just ensure data clearing
//    resets all permission decisions.

// === ADDITIONAL MARKERS FPJS CHECKS ===

// 10. Error.prototype.stack format — must match V8 (Chrome) format
//     Chrome: "Error\n    at Object.<anonymous> (file:1:1)"
//     Firefox: "Object@file:1:1"
//     If stack format wrong → browser mismatch detected
//     Stock Chromium already produces correct format. Don't modify V8 error handling.

// 11. eval.toString() — Chrome returns "function eval() { [native code] }"
//     Don't monkey-patch eval or Function constructor.

// 12. navigator.vendor — must be "Google Inc." (Chrome-specific)
//     Firefox = "", Safari = "Apple Computer, Inc."
String Navigator::vendor() const {
  return "Google Inc.";
}

// 13. navigator.productSub — must be "20030107" (Chrome/Safari)
String Navigator::productSub() const {
  return "20030107";
}

// 14. window.outerWidth / outerHeight — must match screen dimensions
//     On mobile fullscreen, outerWidth ≈ screen.width, outerHeight ≈ screen.height
//     These come from the display metrics, override them to use profile values.

// 15. screen.orientation — must be { type: "portrait-primary", angle: 0 }
//     Mobile phones default to portrait
```

#### Automation markers that MUST NOT exist:

```
// These global variables indicate automation/bot tools.
// Our browser must NOT define ANY of these:

window.__selenium_unwrapped           // Selenium
window.__webdriver_evaluate            // Old Selenium
window.__driver_evaluate               // Old Selenium
window.__webdriver_script_fn           // Selenium
window.__fxdriver_evaluate             // Firefox Selenium
window.__nightmare                     // Nightmare.js
window._phantom                        // PhantomJS
window.callPhantom                     // PhantomJS
window.phantom                         // PhantomJS
window.domAutomation                   // Chrome DevTools Protocol
window.domAutomationController         // Chrome DevTools Protocol
window.cdc_adoQpoasnfa76pfcZLmcfl_Array  // chromedriver marker
window.cdc_adoQpoasnfa76pfcZLmcfl_Promise // chromedriver marker
navigator.__proto__.webdriver           // Webdriver override

// Stock Chromium doesn't have any of these naturally.
// Just DON'T add any automation frameworks to the build.
// If using any testing libraries during development, ensure they're
// stripped from the release build.
```

---

## PART 13.7: COMPLETE GPU EXTENSION LISTS (Per SoC)

### A builder NEEDS exact extension strings. FPJS validates GL extensions match the claimed GPU.

#### Adreno 750 (Snapdragon 8 Gen 3) — 73 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth_texture_cube_map, GL_OES_depth24,
GL_OES_element_index_uint, GL_OES_fbo_render_mipmap,
GL_OES_fragment_precision_high, GL_OES_get_program_binary,
GL_OES_packed_depth_stencil, GL_OES_rgb8_rgba8, GL_OES_sample_shading,
GL_OES_sample_variables, GL_OES_shader_image_atomic,
GL_OES_shader_multisample_interpolation, GL_OES_standard_derivatives,
GL_OES_surfaceless_context, GL_OES_texture_3D, GL_OES_texture_border_clamp,
GL_OES_texture_buffer, GL_OES_texture_compression_astc,
GL_OES_texture_cube_map_array, GL_OES_texture_float,
GL_OES_texture_float_linear, GL_OES_texture_half_float,
GL_OES_texture_half_float_linear, GL_OES_texture_npot,
GL_OES_texture_stencil8, GL_OES_texture_storage_multisample_2d_array,
GL_OES_vertex_array_object, GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_blend_equation_advanced_coherent,
GL_KHR_debug, GL_KHR_no_error, GL_KHR_robust_buffer_access_behavior,
GL_KHR_texture_compression_astc_hdr, GL_KHR_texture_compression_astc_ldr,
GL_EXT_EGL_image_storage, GL_EXT_blend_func_extended,
GL_EXT_blit_framebuffer_params, GL_EXT_buffer_storage,
GL_EXT_clip_control, GL_EXT_clip_cull_distance, GL_EXT_color_buffer_float,
GL_EXT_color_buffer_half_float, GL_EXT_copy_image, GL_EXT_debug_label,
GL_EXT_debug_marker, GL_EXT_discard_framebuffer,
GL_EXT_disjoint_timer_query, GL_EXT_draw_buffers_indexed,
GL_EXT_external_buffer, GL_EXT_fragment_invocation_density,
GL_EXT_geometry_shader, GL_EXT_gpu_shader5, GL_EXT_memory_object,
GL_EXT_memory_object_fd, GL_EXT_multisampled_render_to_texture,
GL_EXT_multisampled_render_to_texture2, GL_EXT_primitive_bounding_box,
GL_EXT_protected_textures, GL_EXT_read_format_bgra,
GL_EXT_robustness, GL_EXT_sRGB, GL_EXT_sRGB_write_control,
GL_EXT_shader_framebuffer_fetch, GL_EXT_shader_io_blocks,
GL_EXT_shader_non_constant_global_initializers,
GL_EXT_tessellation_shader, GL_EXT_texture_border_clamp,
GL_EXT_texture_buffer, GL_EXT_texture_cube_map_array,
GL_EXT_texture_filter_anisotropic, GL_EXT_texture_format_BGRA8888,
GL_EXT_texture_norm16, GL_EXT_texture_sRGB_decode,
GL_EXT_texture_type_2_10_10_10_REV, GL_QCOM_shader_framebuffer_fetch_noncoherent,
GL_QCOM_texture_foveated, GL_QCOM_texture_foveated_subsampled_layout,
GL_QCOM_motion_estimation, GL_QCOM_validate_shader_binary,
GL_OVR_multiview, GL_OVR_multiview2, GL_OVR_multiview_multisampled_render_to_texture
```

#### Adreno 730 (Snapdragon 8 Gen 1) — 68 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth_texture_cube_map, GL_OES_depth24,
GL_OES_element_index_uint, GL_OES_fbo_render_mipmap,
GL_OES_fragment_precision_high, GL_OES_get_program_binary,
GL_OES_packed_depth_stencil, GL_OES_rgb8_rgba8, GL_OES_sample_shading,
GL_OES_sample_variables, GL_OES_shader_image_atomic,
GL_OES_shader_multisample_interpolation, GL_OES_standard_derivatives,
GL_OES_surfaceless_context, GL_OES_texture_3D, GL_OES_texture_border_clamp,
GL_OES_texture_buffer, GL_OES_texture_compression_astc,
GL_OES_texture_cube_map_array, GL_OES_texture_float,
GL_OES_texture_float_linear, GL_OES_texture_half_float,
GL_OES_texture_half_float_linear, GL_OES_texture_npot,
GL_OES_texture_stencil8, GL_OES_texture_storage_multisample_2d_array,
GL_OES_vertex_array_object, GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_blend_equation_advanced_coherent,
GL_KHR_debug, GL_KHR_no_error, GL_KHR_robust_buffer_access_behavior,
GL_KHR_texture_compression_astc_hdr, GL_KHR_texture_compression_astc_ldr,
GL_EXT_EGL_image_storage, GL_EXT_blend_func_extended,
GL_EXT_buffer_storage, GL_EXT_clip_cull_distance,
GL_EXT_color_buffer_float, GL_EXT_color_buffer_half_float,
GL_EXT_copy_image, GL_EXT_debug_label, GL_EXT_debug_marker,
GL_EXT_discard_framebuffer, GL_EXT_disjoint_timer_query,
GL_EXT_draw_buffers_indexed, GL_EXT_external_buffer,
GL_EXT_geometry_shader, GL_EXT_gpu_shader5, GL_EXT_memory_object,
GL_EXT_memory_object_fd, GL_EXT_multisampled_render_to_texture,
GL_EXT_multisampled_render_to_texture2, GL_EXT_primitive_bounding_box,
GL_EXT_read_format_bgra, GL_EXT_robustness, GL_EXT_sRGB,
GL_EXT_sRGB_write_control, GL_EXT_shader_framebuffer_fetch,
GL_EXT_shader_io_blocks, GL_EXT_tessellation_shader,
GL_EXT_texture_border_clamp, GL_EXT_texture_buffer,
GL_EXT_texture_cube_map_array, GL_EXT_texture_filter_anisotropic,
GL_EXT_texture_format_BGRA8888, GL_EXT_texture_norm16,
GL_EXT_texture_sRGB_decode, GL_EXT_texture_type_2_10_10_10_REV,
GL_QCOM_shader_framebuffer_fetch_noncoherent, GL_QCOM_texture_foveated,
GL_OVR_multiview, GL_OVR_multiview2
```

#### Adreno 619 (Snapdragon 695/4 Gen 1) — 55 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth24, GL_OES_element_index_uint,
GL_OES_fbo_render_mipmap, GL_OES_fragment_precision_high,
GL_OES_get_program_binary, GL_OES_packed_depth_stencil,
GL_OES_rgb8_rgba8, GL_OES_sample_shading, GL_OES_sample_variables,
GL_OES_standard_derivatives, GL_OES_surfaceless_context,
GL_OES_texture_3D, GL_OES_texture_compression_astc,
GL_OES_texture_float, GL_OES_texture_float_linear,
GL_OES_texture_half_float, GL_OES_texture_half_float_linear,
GL_OES_texture_npot, GL_OES_texture_stencil8,
GL_OES_vertex_array_object, GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_debug, GL_KHR_no_error,
GL_KHR_texture_compression_astc_ldr,
GL_EXT_blend_func_extended, GL_EXT_color_buffer_float,
GL_EXT_color_buffer_half_float, GL_EXT_copy_image,
GL_EXT_debug_label, GL_EXT_debug_marker,
GL_EXT_discard_framebuffer, GL_EXT_disjoint_timer_query,
GL_EXT_draw_buffers_indexed, GL_EXT_geometry_shader,
GL_EXT_gpu_shader5, GL_EXT_multisampled_render_to_texture,
GL_EXT_read_format_bgra, GL_EXT_robustness, GL_EXT_sRGB,
GL_EXT_shader_framebuffer_fetch, GL_EXT_shader_io_blocks,
GL_EXT_texture_border_clamp, GL_EXT_texture_buffer,
GL_EXT_texture_filter_anisotropic, GL_EXT_texture_format_BGRA8888,
GL_EXT_texture_norm16, GL_EXT_texture_sRGB_decode,
GL_EXT_texture_type_2_10_10_10_REV,
GL_QCOM_shader_framebuffer_fetch_noncoherent,
GL_OVR_multiview, GL_OVR_multiview2
```

#### Mali-G720 (Dimensity 9300) — 62 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth_texture_cube_map, GL_OES_depth24,
GL_OES_element_index_uint, GL_OES_fbo_render_mipmap,
GL_OES_fragment_precision_high, GL_OES_get_program_binary,
GL_OES_packed_depth_stencil, GL_OES_rgb8_rgba8,
GL_OES_sample_shading, GL_OES_sample_variables,
GL_OES_shader_image_atomic, GL_OES_standard_derivatives,
GL_OES_surfaceless_context, GL_OES_texture_3D,
GL_OES_texture_border_clamp, GL_OES_texture_buffer,
GL_OES_texture_compression_astc, GL_OES_texture_cube_map_array,
GL_OES_texture_float, GL_OES_texture_float_linear,
GL_OES_texture_half_float, GL_OES_texture_half_float_linear,
GL_OES_texture_npot, GL_OES_texture_stencil8,
GL_OES_texture_storage_multisample_2d_array,
GL_OES_vertex_array_object, GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_blend_equation_advanced_coherent,
GL_KHR_debug, GL_KHR_no_error, GL_KHR_robust_buffer_access_behavior,
GL_KHR_texture_compression_astc_hdr, GL_KHR_texture_compression_astc_ldr,
GL_EXT_blend_func_extended, GL_EXT_buffer_storage,
GL_EXT_clip_cull_distance, GL_EXT_color_buffer_float,
GL_EXT_color_buffer_half_float, GL_EXT_copy_image,
GL_EXT_debug_marker, GL_EXT_discard_framebuffer,
GL_EXT_disjoint_timer_query, GL_EXT_draw_buffers_indexed,
GL_EXT_geometry_shader, GL_EXT_gpu_shader5,
GL_EXT_multisampled_render_to_texture,
GL_EXT_multisampled_render_to_texture2,
GL_EXT_primitive_bounding_box, GL_EXT_read_format_bgra,
GL_EXT_robustness, GL_EXT_sRGB, GL_EXT_sRGB_write_control,
GL_EXT_shader_io_blocks, GL_EXT_tessellation_shader,
GL_EXT_texture_border_clamp, GL_EXT_texture_buffer,
GL_EXT_texture_cube_map_array, GL_EXT_texture_filter_anisotropic,
GL_EXT_texture_format_BGRA8888, GL_EXT_texture_norm16,
GL_EXT_texture_sRGB_decode, GL_EXT_texture_type_2_10_10_10_REV,
GL_ARM_shader_framebuffer_fetch, GL_ARM_shader_framebuffer_fetch_depth_stencil,
GL_OVR_multiview, GL_OVR_multiview2
```

#### Mali-G610 (Dimensity 7200/8100) — 52 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth24, GL_OES_element_index_uint,
GL_OES_fbo_render_mipmap, GL_OES_fragment_precision_high,
GL_OES_get_program_binary, GL_OES_packed_depth_stencil,
GL_OES_rgb8_rgba8, GL_OES_sample_shading, GL_OES_sample_variables,
GL_OES_standard_derivatives, GL_OES_surfaceless_context,
GL_OES_texture_3D, GL_OES_texture_border_clamp,
GL_OES_texture_compression_astc, GL_OES_texture_float,
GL_OES_texture_float_linear, GL_OES_texture_half_float,
GL_OES_texture_half_float_linear, GL_OES_texture_npot,
GL_OES_texture_stencil8, GL_OES_vertex_array_object,
GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_debug, GL_KHR_no_error,
GL_KHR_texture_compression_astc_ldr,
GL_EXT_blend_func_extended, GL_EXT_color_buffer_float,
GL_EXT_color_buffer_half_float, GL_EXT_copy_image,
GL_EXT_debug_marker, GL_EXT_discard_framebuffer,
GL_EXT_draw_buffers_indexed, GL_EXT_geometry_shader,
GL_EXT_gpu_shader5, GL_EXT_multisampled_render_to_texture,
GL_EXT_read_format_bgra, GL_EXT_robustness, GL_EXT_sRGB,
GL_EXT_shader_io_blocks, GL_EXT_tessellation_shader,
GL_EXT_texture_border_clamp, GL_EXT_texture_buffer,
GL_EXT_texture_filter_anisotropic, GL_EXT_texture_format_BGRA8888,
GL_EXT_texture_norm16, GL_EXT_texture_sRGB_decode,
GL_ARM_shader_framebuffer_fetch,
GL_OVR_multiview, GL_OVR_multiview2
```

#### PowerVR BXM-8-256 (Helio G99) — 42 extensions:

```
GL_OES_EGL_image, GL_OES_EGL_image_external, GL_OES_EGL_sync,
GL_OES_depth_texture, GL_OES_depth24, GL_OES_element_index_uint,
GL_OES_fbo_render_mipmap, GL_OES_fragment_precision_high,
GL_OES_get_program_binary, GL_OES_packed_depth_stencil,
GL_OES_rgb8_rgba8, GL_OES_standard_derivatives,
GL_OES_surfaceless_context, GL_OES_texture_3D,
GL_OES_texture_compression_astc, GL_OES_texture_float,
GL_OES_texture_float_linear, GL_OES_texture_half_float,
GL_OES_texture_half_float_linear, GL_OES_texture_npot,
GL_OES_vertex_array_object, GL_OES_vertex_half_float,
GL_KHR_blend_equation_advanced, GL_KHR_debug,
GL_KHR_texture_compression_astc_ldr,
GL_EXT_color_buffer_float, GL_EXT_color_buffer_half_float,
GL_EXT_copy_image, GL_EXT_debug_marker, GL_EXT_discard_framebuffer,
GL_EXT_draw_buffers_indexed, GL_EXT_multisampled_render_to_texture,
GL_EXT_read_format_bgra, GL_EXT_robustness, GL_EXT_sRGB,
GL_EXT_shader_io_blocks, GL_EXT_texture_border_clamp,
GL_EXT_texture_filter_anisotropic, GL_EXT_texture_format_BGRA8888,
GL_EXT_texture_sRGB_decode, GL_EXT_texture_type_2_10_10_10_REV,
GL_IMG_texture_compression_pvrtc
```

> **NOTE for builder**: These extension lists are from real devices. For the remaining SoCs (Adreno 740, 725, 720, 710, 618, 616, Mali-G78, G710, G615, G57, Immortalis-G715, Xclipse-940, Tensor GPU), extract real extension lists by running `gl.getParameter(gl.EXTENSIONS)` and `gl.getSupportedExtensions()` on actual devices or from GPU specification databases. The pattern is: flagship GPUs have 60-75 extensions, mid-range has 45-60, budget has 35-50.

---

## PART 13.8: COMPLETE FONT LISTS PER MANUFACTURER

### FPJS checks which system fonts are installed. The list MUST match the claimed OEM.

#### Samsung (Galaxy S/A/M series):

```
Required fonts (always present on Samsung):
- Roboto (system default fallback)
- SamsungOne (Samsung's system UI font)  
- SECRobotoLight (Samsung's modified Roboto)
- NotoSansCJK (CJK support)
- NotoColorEmoji (if not using Samsung emoji)
- SamsungColorEmoji (Samsung's emoji font)
- NotoSansDevanagari (Hindi support, Indian market)
- NotoSansBengali (Bengali support)
- NotoSansTamil, NotoSansTelugu, NotoSansKannada, NotoSansMalayalam
- DroidSansFallback
- Courier New (web fallback)
- Georgia (web fallback)
- Verdana (web fallback)
- Times New Roman (web fallback)
- Comic Sans MS (available on Samsung)

Custom Samsung fonts (not on other OEMs):
+ SamsungSans
+ SECRoboto
+ SamsungOneUI
+ GothicA1 (Samsung's Korean font)
```

#### Xiaomi / Redmi / POCO:

```
Required fonts:
- Roboto
- MiSans (Xiaomi system font since MIUI 13)
- MiLanPro (older MIUI font)
- NotoSansCJK
- NotoColorEmoji
- NotoSansDevanagari, NotoSansBengali, NotoSansTamil
- DroidSansFallback
- Courier New, Georgia, Verdana, Times New Roman

Custom Xiaomi fonts:
+ MiSans VF (variable font)
+ MiSansLatin
+ Clock (MIUI clock widget font)
```

#### OnePlus:

```
Required fonts:
- Roboto
- OnePlusSans (OnePlus system font)
- SlateForOnePlus (older OxygenOS font)
- NotoSansCJK
- NotoColorEmoji
- NotoSansDevanagari, NotoSansBengali
- DroidSansFallback
- Courier New, Georgia, Verdana, Times New Roman

Custom OnePlus fonts:
+ OnePlusSansMono
+ OnePlusSansDisplay
```

#### Stock Android / Google Pixel:

```
Required fonts:
- Roboto (primary system font)
- Google Sans (Pixel exclusive)
- Product Sans (Google branding font)
- NotoSansCJK
- NotoColorEmoji
- NotoSansDevanagari, NotoSansBengali, NotoSansTamil, NotoSansTelugu
- DroidSansFallback
- NotoSerif
- CutiveMono
- Courier New, Georgia, Verdana, Times New Roman
- DancingScript, CarroisGothicSC, ComingSoon
```

#### Realme / OPPO / Vivo (ColorOS / Funtouch):

```
Required fonts:
- Roboto
- NotoSansCJK
- NotoColorEmoji
- NotoSansDevanagari, NotoSansBengali
- DroidSansFallback
- Courier New, Georgia, Verdana, Times New Roman

NO custom OEM fonts (these use stock Roboto as system font)
```

#### Motorola:

```
Required fonts:
- Roboto (stock Android, near-stock experience)
- NotoSansCJK
- NotoColorEmoji
- NotoSansDevanagari, NotoSansBengali
- DroidSansFallback
- Courier New, Georgia, Verdana, Times New Roman

NO custom OEM fonts (Motorola uses near-stock Android)
```

---

## PART 13.9: SENSOR HARDWARE NAMES PER MANUFACTURER

### FPJS checks accelerometer/gyro sensor names. They MUST match the OEM.

```
Sensor API returns: { name: "LSM6DSO Accelerometer", vendor: "STMicroelectronics" }
If we claim Samsung but sensor says "BMI260" (Bosch, used by Xiaomi) → mismatch.
```

#### Samsung sensors:

```
Accelerometer: "LSM6DSO Accelerometer" | "LSM6DSL Accelerometer" | "ICM-42607 Accelerometer"
Gyroscope:     "LSM6DSO Gyroscope" | "LSM6DSL Gyroscope" | "ICM-42607 Gyroscope"
Magnetometer:  "AK09918 Magnetometer" | "MMC5603 Magnetometer"
Barometer:     "BMP390 Barometer" | "LPS22HH Barometer"
Proximity:     "TMD4907 Proximity" | "STK33562 Proximity"
Light:         "TMD4907 ALS" | "STK33562 Light"
Vendor:        "STMicroelectronics" | "InvenSense" | "AKM" | "Bosch"
```

#### Xiaomi / Redmi / POCO:

```
Accelerometer: "BMI260 Accelerometer" | "ICM-42607 Accelerometer" | "LSM6DSO Accelerometer"
Gyroscope:     "BMI260 Gyroscope" | "ICM-42607 Gyroscope" | "LSM6DSO Gyroscope"
Magnetometer:  "AK09918 Magnetometer" | "MMC5603 Magnetometer"
Barometer:     "BMP380 Barometer" (flagship only)
Proximity:     "STK33562 Proximity" | "SX9324 SAR"
Light:         "STK33562 Light" | "TSL2591 ALS"
Vendor:        "Bosch" | "InvenSense" | "STMicroelectronics"
```

#### OnePlus:

```
Accelerometer: "BMI260 Accelerometer" | "LSM6DSO Accelerometer"
Gyroscope:     "BMI260 Gyroscope" | "LSM6DSO Gyroscope"
Magnetometer:  "AK09918 Magnetometer"
Proximity:     "STK33562 Proximity"
Light:         "TMD2725 Light"
Vendor:        "Bosch" | "STMicroelectronics"
```

#### Google Pixel:

```
Accelerometer: "BMI260 Accelerometer" | "LSM6DSR Accelerometer"
Gyroscope:     "BMI260 Gyroscope" | "LSM6DSR Gyroscope"
Magnetometer:  "MMC5633 Magnetometer"
Barometer:     "BMP580 Barometer"
Proximity:     "TMD3719 Proximity"
Light:         "TMD3719 ALS"
Vendor:        "Bosch" | "STMicroelectronics" | "MEMSIC"
```

#### Budget phones (Realme, Motorola, Tecno):

```
Accelerometer: "MC3419 Accelerometer" | "BMA253 Accelerometer" | "SC7A20 Accelerometer"
Gyroscope:     (OFTEN MISSING on budget phones — return empty/error)
Magnetometer:  "MMC5603 Magnetometer" | "AK09918 Magnetometer"
Barometer:     (NOT present on budget phones)
Proximity:     "STK3x1x Proximity"
Light:         "STK3x1x Light"
Vendor:        "mCube" | "Bosch" | "Sensortek"

NOTE: Budget phones often have NO gyroscope. The profile generator 
should set has_gyroscope=false for budget tier, causing 
new Gyroscope() to throw NotAllowedError.
```

---

## PART 13.95: CHROME VERSION MANAGEMENT

### The Chrome version in User-Agent MUST match real Chrome stable releases.

```
DON'T hardcode a Chrome version. It goes stale and gets flagged.

Strategy:
1. At build time, fetch the current Chrome stable version from:
   https://chromiumdash.appspot.com/fetch/milestones

2. Store it as a build-time constant:
   // In content/common/chrome_version.h
   #define GHOST_CHROME_VERSION "122.0.6261.64"
   
3. The build CI/CD pipeline should:
   - Check Chrome stable version weekly
   - If version changed → rebuild + push update
   - Users auto-update via Play Store / APK download

4. Version format must match real UA:
   Chrome/{major}.0.{build}.{patch}
   e.g., Chrome/122.0.6261.64
   
5. The version must appear in ALL of:
   - User-Agent header
   - navigator.userAgent
   - Sec-CH-UA header ("Chromium";v="{major}", "Google Chrome";v="{major}")
   - Sec-CH-UA-Full-Version-List
   
6. Must also match the actual Chromium source version you built from.
   If you build from Chromium 122 source, the version must be 122.x.x.x.
   Don't claim Chrome 128 if built from 122 source — FPJS can detect
   API mismatches (new APIs added in 128 won't exist in 122 binary).
```

---

## PART 14: MATH FINGERPRINTING (CPU FLOATING-POINT)

### The Problem

`Math.tan()`, `Math.atan2()`, `Math.exp()`, `Math.log()`, `Math.sin()`, `Math.cos()` produce **slightly different results on different CPU architectures** due to floating-point unit (FPU) implementation differences. FingerprintJS can call:

```javascript
// These produce different last-digits on different ARM CPU cores
Math.tan(-1e300)          // Different on Cortex-X4 vs Cortex-A78 vs Cortex-A53
Math.atan2(1, 2)          // FPU implementation-dependent
Math.expm1(1)             // Different precision per architecture
Math.log1p(0.5)           // Same
Math.cbrt(2)              // Same
```

The result is a **hardware fingerprint** — if we claim "Snapdragon 8 Gen 3" (Cortex-X4) but the math results match a "Dimensity 7050" (Cortex-A78), that's a detectable inconsistency.

### Solution: Override V8 Math Builtins

#### Files to modify in `v8/src/builtins/`

```cpp
// v8/src/builtins/builtins-math.cc

// Override transcendental math functions with standardized implementations
// that produce IDENTICAL results regardless of CPU architecture.

// Strategy: Use software double-precision math (libm/crlibm) instead of
// hardware FPU instructions. This ensures bit-exact results on every device.

// BEFORE: V8 calls hardware FPU via __builtin_sin(), __builtin_cos(), etc.
// These produce CPU-dependent results.

// AFTER: V8 calls our software implementation that produces 
// architecture-independent results, plus per-session micro-perturbation.

TF_BUILTIN(MathTan, MathGenericBuiltinReducer) {
  double x = GetDoubleArg(args, 0);
  // Use software implementation (bit-exact across all CPUs)
  double result = SoftwareMathTan(x);
  // Add per-session perturbation in the last 2-3 ULP (unit of least precision)
  // This makes each session unique without being detectable as fake
  uint64_t seed = DeviceProfileStore::Get()->GetProfile().math_seed;
  result = PerturbLastBits(result, seed, 2);  // Perturb last 2 bits
  return result;
}
```

#### File: `device_profile_generator/software_math.h`

```cpp
// Provide bit-exact software implementations of all Math.* functions
// Based on FDLIBM (Freely Distributable LIBM) or crlibm (Correctly Rounded LIBM)
// These produce identical results on ARM, x86, RISC-V, ANY architecture.

double SoftwareMathSin(double x);
double SoftwareMathCos(double x);
double SoftwareMathTan(double x);
double SoftwareMathAtan2(double y, double x);
double SoftwareMathExp(double x);
double SoftwareMathExpm1(double x);
double SoftwareMathLog(double x);
double SoftwareMathLog1p(double x);
double SoftwareMathCbrt(double x);
double SoftwareMathSinh(double x);
double SoftwareMathCosh(double x);
double SoftwareMathTanh(double x);

// PerturbLastBits: flip 1-3 ULP bits based on session seed
// This creates unique-per-session math signatures without
// producing values that are "wrong" (difference is < 10^-15)
double PerturbLastBits(double value, uint64_t seed, int bits);
```

**Result**: Every session produces slightly different `Math.*` results, but they all look like legitimate floating-point output. No CPU architecture leak.

---

## PART 15: EMOJI & UNICODE RENDERING

### The Problem

Samsung devices use Samsung emoji font. Google/Xiaomi/OnePlus use Google Noto Color Emoji. When FPJS draws emoji on canvas and reads pixels, the emoji style reveals the real manufacturer.

```javascript
// FPJS draws these on a canvas:
ctx.fillText("😀🎉🏳️‍🌈🚀💯", 0, 50);
// Then reads pixels back — Samsung emoji look COMPLETELY different from Google emoji
```

If we claim "Samsung Galaxy S24" but the canvas shows Google-style emoji, we're caught.

### Solution: Bundle Manufacturer Emoji Fonts

#### Files to modify:

```
chrome/android/java/res/font/     ← Bundle emoji fonts
third_party/blink/renderer/platform/fonts/font_cache_android.cc  ← Font selection

Strategy:
1. Bundle 3 emoji fonts in the APK:
   - SamsungColorEmoji.ttf (extracted from Samsung firmware)
   - NotoColorEmoji.ttf (Google's open source emoji)
   - NotoColorEmojiCompat.ttf (for stock Android claim)
   
2. When profile says "Samsung" → load SamsungColorEmoji
   When profile says anything else → load NotoColorEmoji

3. The emoji font MUST be loaded at the Skia/FreeType level,
   not as a web font. It must be the system emoji font.

4. Also swap system UI font:
   - Samsung profile → SamsungOne as system font  
   - Xiaomi profile → MiSans
   - OnePlus profile → OnePlusSans
   - Others → Roboto (stock Android)
```

#### APK Size Impact:
```
SamsungColorEmoji.ttf  ≈ 12MB
NotoColorEmoji.ttf     ≈ 10MB  
System fonts (3 sets)  ≈ 5MB
Total APK increase     ≈ 27MB (acceptable for a 200MB APK)
```

---

## PART 16: PERFORMANCE TIMING NORMALIZATION

### The Problem

FingerprintJS measures how long operations take. A budget phone (Snapdragon 4 Gen 1) takes ~50ms for canvas.toDataURL(). A flagship (8 Gen 3) takes ~8ms. If we claim budget phone but operations are fast, the timing reveals the real hardware.

```javascript
// FPJS timing test:
const t0 = performance.now();
canvas.toDataURL();
const elapsed = performance.now() - t0;
// If elapsed=8ms but claimed device is budget → SUSPICIOUS
```

### Solution: Add Artificial Delays

#### Files to modify in `third_party/blink/renderer/`

```cpp
// File: modules/canvas/canvas2d/canvas_rendering_context_2d.cc

String CanvasRenderingContext2D::toDataURL(const String& type, ...) {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  
  // Start timer
  auto start = base::TimeTicks::Now();
  
  // Do actual rendering
  String result = ActualToDataURL(type, quality);
  
  // Calculate how long a real device would take
  auto elapsed = base::TimeTicks::Now() - start;
  auto expected = profile.GetExpectedCanvasTime();  // From SoC tier
  
  // If we finished too fast, sleep the difference
  if (elapsed < expected) {
    base::PlatformThread::Sleep(expected - elapsed);
  }
  
  return result;
}
```

#### SoC Performance Tiers:
```
Flagship (8 Gen 3, Dimensity 9300):     canvas: 5-12ms, webgl: 3-8ms
Upper-Mid (7+ Gen 2, Dimensity 8300):   canvas: 12-25ms, webgl: 8-15ms
Mid-Range (Snapdragon 695, Helio G99):  canvas: 25-50ms, webgl: 15-30ms
Budget (Snapdragon 4 Gen 1, Helio G36): canvas: 50-100ms, webgl: 30-60ms

Each operation has a base delay + random jitter (±10-20%)
Jitter prevents exact timing correlation across sessions
```

#### Also normalize:
```
- crypto.subtle.digest() timing
- WebAssembly compilation timing
- JSON.parse() for large payloads
- Regular expression execution time
- Array.prototype.sort() on large arrays
```

---

## PART 17: STORAGE & MEMORY FINGERPRINTING

### The Problem

```javascript
// Navigator APIs that reveal real device specs:
navigator.storage.estimate()      // → {quota: 274877906944} = 256GB real storage
performance.memory                // → {jsHeapSizeLimit: 4294967296} = 4GB real heap
navigator.connection.downlink     // → reveals real network speed
navigator.connection.rtt          // → reveals real latency
navigator.connection.saveData     // → reveals data saver setting
```

If we claim "Samsung Galaxy A15" (64GB storage, 4GB RAM) but `storage.estimate()` shows 256GB, that's a leak.

### Solution:

#### Files to modify:

```cpp
// third_party/blink/renderer/modules/quota/storage_manager.cc

ScriptPromise StorageManager::estimate(ScriptState* script_state) {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  
  // Return spoofed storage that matches the claimed device tier
  uint64_t total = profile.GetStorageBytes();   // 64GB, 128GB, 256GB based on tier
  uint64_t used = total * (0.3 + RandFloat() * 0.4);  // 30-70% used (realistic)
  
  // Don't return exact round numbers — add noise
  total += RandInt(-500000000, 500000000);  // ±500MB noise
  used += RandInt(-100000000, 100000000);   // ±100MB noise
  
  return CreateEstimateResult(total, used);
}

// performance.memory (non-standard Chrome API)
// v8/src/api/api.cc or blink performance interface
HeapStatistics GetHeapStatistics() {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  // Return memory stats consistent with device RAM
  // 4GB device → ~1.5GB heap limit
  // 8GB device → ~2.5GB heap limit
  return FakeHeapStats(profile.ram_mb);
}

// Navigator.connection 
// third_party/blink/renderer/modules/netinfo/network_information.cc
double NetworkInformation::downlink() const {
  // Return realistic mobile network speed, not real
  // 4G: 5-20 Mbps, WiFi: 20-80 Mbps
  return DeviceProfileStore::Get()->GetProfile().GetNetworkDownlink();
}

unsigned long NetworkInformation::rtt() const {
  // 4G: 30-80ms, WiFi: 5-20ms
  return DeviceProfileStore::Get()->GetProfile().GetNetworkRtt();
}
```

#### Storage tiers from device price tier:
```
Budget:    64GB total (32-50GB available)
Mid-Range: 128GB total (80-100GB available)
Flagship:  256GB total (180-220GB available)
Premium:   512GB total (350-450GB available)
```

---

## PART 18: SPEECH SYNTHESIS VOICES

### The Problem

```javascript
speechSynthesis.getVoices()
// Returns different voice lists on different devices/manufacturers
// Samsung: "Samsung Korean", "Samsung English (US)", etc.
// Stock Android: "Google हिन्दी", "Google English (India)", etc.
```

### Solution:

```cpp
// third_party/blink/renderer/modules/speech/speech_synthesis.cc

void SpeechSynthesis::OnVoicesChanged() {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  
  // Clear real voice list and return manufacturer-appropriate voices
  voices_.clear();
  
  if (profile.manufacturer == "Samsung") {
    // Samsung has Samsung TTS + Google TTS voices
    AddVoice("Samsung English (United States)", "en-US", false);
    AddVoice("Samsung English (United Kingdom)", "en-GB", false);
    AddVoice("Samsung हिन्दी (भारत)", "hi-IN", false);
    AddVoice("Google English (India)", "en-IN", true);
    AddVoice("Google हिन्दी", "hi-IN", true);
    // ... (Samsung devices have 10-15 voices)
  } else {
    // Stock Android / Xiaomi / OnePlus use Google TTS only
    AddVoice("Google English (India)", "en-IN", true);
    AddVoice("Google English (United States)", "en-US", false);
    AddVoice("Google हिन्दी", "hi-IN", false);
    AddVoice("Google বাংলা", "bn-IN", false);
    // ... (Google TTS has 8-12 voices)
  }
}
```

---

## PART 19: WEBGPU ADAPTER INFO

### The Problem

```javascript
const adapter = await navigator.gpu.requestAdapter();
const info = await adapter.requestAdapterInfo();
// info.vendor: "qualcomm"
// info.architecture: "adreno-700"
// info.device: "adreno-750"  ← reveals real GPU at lower level than WebGL
```

### Solution:

```cpp
// third_party/blink/renderer/modules/webgpu/gpu_adapter.cc

void GPUAdapter::requestAdapterInfo(ScriptState* state, ...) {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  
  // Override adapter info with profile GPU
  GPUAdapterInfo* info = MakeGarbageCollected<GPUAdapterInfo>();
  info->setVendor(profile.webgpu_vendor);       // "qualcomm" / "arm" / "google"
  info->setArchitecture(profile.webgpu_arch);    // "adreno-700" / "valhall" / "mali-g7xx"
  info->setDevice(profile.webgpu_device);        // From SoC→GPU mapping
  info->setDescription(profile.webgpu_description);
  
  // Also limit visible WebGPU features to what the claimed GPU supports
  FilterFeaturesForGPU(profile.soc_name);
}

// If the claimed device's SoC doesn't support WebGPU at all
// (budget chips like Helio G36), disable WebGPU entirely:
bool GPUAdapter::IsAvailable() {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  return profile.supports_webgpu;  // false for budget SoCs
}
```

#### SoC → WebGPU Info mapping:
```
Snapdragon 8 Gen 3: vendor="qualcomm", arch="adreno-700", device="adreno-750"
Snapdragon 7+ Gen 2: vendor="qualcomm", arch="adreno-700", device="adreno-725"
Dimensity 9300: vendor="arm", arch="valhall", device="immortalis-g720"
Exynos 2400: vendor="arm", arch="valhall", device="xclipse-940"
Helio G99: WebGPU NOT supported (disable entirely)
Snapdragon 4 Gen 1: WebGPU NOT supported (disable entirely)
```

---

## PART 20: CSS MEDIA QUERY FINGERPRINTING

### The Problem

```javascript
// FPJS checks CSS feature queries:
matchMedia('(hover: hover)').matches         // desktop vs mobile
matchMedia('(pointer: fine)').matches        // mouse vs touch
matchMedia('(prefers-color-scheme: dark)')   // reveals system theme
matchMedia('(max-resolution: 2dppx)')        // reveals real DPI
matchMedia('(display-mode: standalone)')     // reveals if installed as PWA
matchMedia('(prefers-reduced-motion)')       // reveals accessibility settings
window.matchMedia('(color-gamut: p3)')       // reveals display capability
```

### Solution:

```cpp
// third_party/blink/renderer/core/css/media_query_evaluator.cc

bool MediaQueryEvaluator::EvalHover() const {
  // Mobile Android → hover: none
  return false;
}

bool MediaQueryEvaluator::EvalPointer() const {
  // Mobile Android → pointer: coarse (finger, not mouse)
  return true;  // coarse
}

bool MediaQueryEvaluator::EvalPrefersColorScheme() const {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  // Randomize: 60% dark, 40% light (matches real Indian user distribution)
  return profile.prefers_dark_mode;
}

float MediaQueryEvaluator::GetDevicePixelRatio() const {
  // Return profile's density, not real display density
  return DeviceProfileStore::Get()->GetProfile().density;
}

bool MediaQueryEvaluator::EvalColorGamut() const {
  auto& profile = DeviceProfileStore::Get()->GetProfile();
  // Flagship phones: P3 gamut. Budget: sRGB only.
  return profile.supports_p3_gamut;
}
```

---

## PART 21: ROOT/FRIDA/MAGISK HIDING (BONUS — Native SDK Only)

### ⚠️ IMPORTANT: NOT NEEDED FOR WEB BYPASS

The `jailbroken`, `frida`, `clonedApp`, and `factoryReset` signals are **ONLY available through the FingerprintJS Pro Native Android/iOS SDK** — they CANNOT be detected by the web JavaScript agent.

Polymarket (and most websites) use the **web JS agent** loaded via `<script>` tag. The web agent runs in the browser sandbox and has **NO access** to:
- File system paths (`/system/bin/su`, `/proc/self/maps`)
- Installed packages (`com.topjohnwu.magisk`)
- App installation metadata (factory reset timing)
- Native process inspection (Frida hooks)

**When is this part needed?**
- Only if you're building an Android APP (not browser) that embeds FPJS native SDK
- Or if a target website somehow has a companion app that reports these signals
- For pure web browsing (Polymarket, etc.) → **SKIP THIS PART entirely**

### The Problem (Native SDK only)

FPJS **Native Android SDK** (not the web JS agent) can detect:
- **Rooted device** (`jailbroken` signal) — checks `/system/bin/su`, Magisk
- **Frida instrumentation** (`frida` signal) — checks `/proc/self/maps`
- **Cloned apps** (`clonedApp` signal) — checks package installer
- **Recent factory reset** (`factoryReset` signal) — checks device setup time

### Solution (only if needed for native app use case):

```
Since this is a CUSTOM Chromium build, not a modified system app:
1. The browser itself won't trigger clonedApp (it's not a clone)
2. The browser won't trigger factoryReset (it checks app install time)
3. BUT root/Frida detection checks /proc, /system, and other system paths

Files to modify:
- content/browser/sandbox_linux.cc — Block access to /proc entries that reveal root
- Intercept file system calls that check for:
  • /system/bin/su
  • /system/xbin/su  
  • /data/local/tmp/frida-server
  • /proc/self/maps (Frida detection)
  • /proc/self/status (TracerPid for debugger detection)
  • com.topjohnwu.magisk package presence

Strategy: When the browser's internal file access checks these paths,
return "file not found" regardless of reality.
This is done at the Chromium content sandbox level, NOT at the OS level.
```

---

## PART 22: CLOUDFLARE TLS FINGERPRINT BYPASS (BoringSSL)

### Why This Is Critical

Cloudflare Bot Management uses **TLS fingerprinting** (JA3/JA4 hashes) to identify browsers BEFORE any JavaScript runs. Every browser has a unique TLS ClientHello signature based on:
- Cipher suite list and order
- TLS extensions list and order
- Supported groups (elliptic curves)
- Signature algorithms
- ALPN protocols
- GREASE values (random IETF-reserved values Chrome injects)

A stock Chromium build always produces the **same JA3 hash** for a given version. Cloudflare sees:
```
JA3: 773906b0efdefa24a7f2b8eb6985bf37  ← "This is Chromium 122 on Android"
```
If the same JA3 hash appears from 100 different "devices", Cloudflare knows it's the same browser binary = **same person**.

### Solution: Randomize TLS ClientHello Per Session

#### Files to modify in `third_party/boringssl/`

BoringSSL is Chromium's TLS library. Modify the ClientHello construction:

#### `ssl/handshake_client.cc` — Main ClientHello builder

```cpp
// In ssl_write_client_hello():
// BEFORE: cipher suites added in fixed order
// AFTER:  cipher suites shuffled per session

static bool ssl_add_client_hello_ciphers(SSL_HANDSHAKE *hs, CBB *cbb) {
  auto& tls = DeviceProfileStore::Get()->GetProfile().tls_profile;
  
  // Get the list of enabled ciphers
  std::vector<const SSL_CIPHER*> ciphers;
  for (const SSL_CIPHER* c : SSL_get_ciphers(hs->ssl)) {
    ciphers.push_back(c);
  }
  
  // Shuffle cipher order using session seed
  std::mt19937 rng(tls.cipher_shuffle_seed);
  std::shuffle(ciphers.begin(), ciphers.end(), rng);
  
  // Optionally DROP 1-3 non-essential ciphers to change list length
  int drop_count = tls.cipher_drop_count;  // 0-3
  if (drop_count > 0 && ciphers.size() > 8) {
    // Only drop non-essential ciphers (keep AES-GCM, ChaCha20)
    ciphers.resize(ciphers.size() - drop_count);
  }
  
  // Insert GREASE values at random positions
  InsertGreaseAt(ciphers, tls.grease_positions);
  
  // Write to ClientHello
  for (const auto* cipher : ciphers) {
    if (!CBB_add_u16(cbb, SSL_CIPHER_get_protocol_id(cipher))) return false;
  }
  return true;
}
```

#### `ssl/extensions.cc` — TLS Extension ordering

```cpp
// Extensions are normally added in a fixed order.
// Randomize the order per session:

static bool ssl_add_clienthello_tlsext(SSL_HANDSHAKE *hs, CBB *out) {
  auto& tls = DeviceProfileStore::Get()->GetProfile().tls_profile;
  
  // Build list of extensions to add
  struct ExtEntry {
    uint16_t type;
    std::function<bool(CBB*)> writer;
  };
  std::vector<ExtEntry> extensions;
  
  // Add all standard extensions...
  extensions.push_back({TLSEXT_TYPE_server_name, write_sni});
  extensions.push_back({TLSEXT_TYPE_ec_point_formats, write_ec_point_formats});
  extensions.push_back({TLSEXT_TYPE_supported_groups, write_supported_groups});
  extensions.push_back({TLSEXT_TYPE_session_ticket, write_session_ticket});
  extensions.push_back({TLSEXT_TYPE_signature_algorithms, write_sig_algs});
  extensions.push_back({TLSEXT_TYPE_application_layer_protocol_negotiation, write_alpn});
  extensions.push_back({TLSEXT_TYPE_supported_versions, write_supported_versions});
  extensions.push_back({TLSEXT_TYPE_key_share, write_key_share});
  extensions.push_back({TLSEXT_TYPE_psk_key_exchange_modes, write_psk_modes});
  // ... etc
  
  // Shuffle extension order (except SNI must stay first, PSK must stay last)
  auto first = extensions.begin() + 1;  // Skip SNI
  auto last = extensions.end() - 1;     // Skip PSK
  std::mt19937 rng(tls.extension_shuffle_seed);
  std::shuffle(first, last, rng);
  
  // Insert GREASE extensions at 2-4 random positions
  for (int pos : tls.grease_extension_positions) {
    if (pos < extensions.size()) {
      extensions.insert(extensions.begin() + pos, 
        {GetGreaseValue(rng), write_grease_extension});
    }
  }
  
  // Write extensions in shuffled order
  for (const auto& ext : extensions) {
    ext.writer(&ext_cbb);
  }
  return true;
}
```

#### `ssl/supported_groups.cc` — Elliptic curve order

```cpp
// Randomize supported groups (elliptic curves) order:
// Real Chrome uses: X25519, P-256, P-384
// We can vary: X25519+P-256, P-256+X25519, X25519+P-384+P-256, etc.

static const uint16_t kSupportedGroups[] = {
  // Base set — always include these
  SSL_GROUP_X25519,       // Most common first
  SSL_GROUP_SECP256R1,    // P-256
  SSL_GROUP_SECP384R1,    // P-384
};

// Per session, we can:
// 1. Reorder X25519 and P-256 (both are common first-choice)
// 2. Optionally include P-521
// 3. Optionally include X25519Kyber768 (post-quantum, newer Chrome)
```

#### `ssl/signature_algorithms.cc` — Signature algorithm order

```cpp
// Also shuffle signature algorithm preferences:
// ECDSA+SHA256, RSA-PSS+SHA256, RSA+SHA256, ECDSA+SHA384, etc.
// The order changes JA3 hash without breaking compatibility
```

### TLS Profile Structure:

```cpp
struct TLSProfile {
  // Cipher suite configuration
  uint32_t cipher_shuffle_seed;              // Seed for cipher reordering
  int cipher_drop_count;                     // 0-3 non-essential ciphers to omit
  std::vector<int> grease_positions;         // Where to insert GREASE in cipher list
  
  // Extension configuration  
  uint32_t extension_shuffle_seed;           // Seed for extension reordering
  std::vector<int> grease_extension_positions; // Where to insert GREASE extensions
  int num_grease_extensions;                 // 2-4 GREASE extensions to add
  
  // Supported groups (elliptic curves)
  bool swap_x25519_p256;                     // Swap first two curves?
  bool include_p521;                         // Include P-521?
  bool include_kyber;                        // Include X25519Kyber768 (post-quantum)?
  
  // Signature algorithms
  uint32_t sig_alg_shuffle_seed;             // Seed for signature algorithm reordering
  
  // ALPN
  bool include_http2;                        // true (always for real Chrome)
  bool include_http3;                        // 50% chance (depends on server support)
  
  // Session ticket
  bool send_session_ticket;                  // Varies (new vs returning)
  
  // Padding
  bool use_padding_extension;                // Chrome uses this to pad to 512 bytes
  int padding_target;                        // 512 or 1024

  // Computed
  std::string ja3_hash;                      // Computed JA3 for debugging
  std::string ja4_hash;                      // Computed JA4 for debugging
};
```

### What This Achieves:

| Aspect | Before (stock Chromium) | After (randomized) |
|---|---|---|
| JA3 hash | Same for all instances | **Unique per session** |
| JA4 hash | Same for all instances | **Unique per session** |
| Cipher suite count | Fixed (15-17) | **Varies (12-17)** |
| Cipher order | Fixed | **Randomized** |
| Extension order | Fixed | **Randomized** |
| GREASE positions | Fixed 2 positions | **Random 2-4 positions** |
| Supported curves | Fixed order | **Variable order + optional curves** |
| Cloudflare detection | "Same binary" | **"Different browsers"** |

### Important Constraints:

1. **Never remove required extensions** — SNI, supported_versions, key_share must always be present
2. **Keep cipher suites functional** — Don't remove AES-128-GCM or ChaCha20 (servers need these)
3. **GREASE values must follow RFC 8701** — Use only official GREASE codepoints (0x0A0A, 0x1A1A, etc.)
4. **The resulting TLS handshake must still work** — Test against Cloudflare, AWS, Google, etc.
5. **Mimic real browser diversity** — The shuffled profiles should look like they could come from different Chrome builds, Firefox, Safari, or Edge — not random garbage

### Reference: Real Browser JA3 Fingerprints to Mimic

```
Chrome 120 Android:  773906b0efdefa24a7f2b8eb6985bf37
Chrome 122 Android:  cd08e31494f9531f560d64c695473da9  
Chrome 120 Windows:  a]b4b8a0f0e28939e4c381b8b7b3bc83
Firefox 121:         b32309a26951912be7dba376398abc3b
Safari 17 iOS:       773906b0efdefa24a7f2b8eb6985bf37
Edge 120:            cd08e31494f9531f560d64c695473da9
Samsung Browser 24:  2ad2b501d4d91dfc76db7878f33e3dc8
```

Our randomizer should produce JA3 hashes that fall within the **statistical distribution** of real browsers — not identical to any specific known hash, but structurally similar.

---

## PART 23: BUILD & DEPLOYMENT

### Build instructions:

```bash
# 1. Set up Chromium for Android build environment
# Follow: https://chromium.googlesource.com/chromium/src/+/main/docs/android_build_instructions.md

# 2. Clone Chromium source
fetch --nohooks android
cd src
gclient sync

# 3. Apply all modifications from this spec

# 4. Configure build
gn gen out/Android --args='
  target_os = "android"
  target_cpu = "arm64"
  is_debug = false
  is_component_build = false
  is_official_build = true
  chrome_pgo_phase = 0
  enable_nacl = false
  symbol_level = 0
  android_channel = "stable"
'

# 5. Build
autoninja -C out/Android chrome_public_apk

# 6. Install
adb install out/Android/apks/ChromePublic.apk
```

### Build time: ~4-8 hours on 16-core machine with 32GB RAM
### Source size: ~30GB
### Build output: ~200MB APK

---

## PART 24: PROFILE PERSISTENCE & ROTATION

### Storage strategy:

```
Profile lifecycle:
1. On FIRST LAUNCH after install → Generate random profile, save to SharedPreferences
2. On SUBSEQUENT LAUNCHES → Load same profile (consistent identity)
3. On "NEW IDENTITY" button press → Delete profile, generate new one, restart browser
4. On "CLEAR ALL DATA" → Clear cookies, cache, localStorage, IDB, 
   service workers, WebSQL, AND generate new profile
5. Profile is NEVER written to disk in a way that fingerprinting JS can access
```

### UI Addition:
```
Add to Chrome's 3-dot menu:
├── "New Identity" → Clears everything + new device profile + restart
├── "Current Device" → Shows what device/GPU/screen is being spoofed
└── "Identity Log" → History of past identities (for testing)
```

---

## PART 25: WHAT THIS BROWSER DEFEATS

When built correctly, this browser will defeat:

| Detection Method | Status | How |
|---|---|---|
| FingerprintJS Pro Bot Detection | ✅ Defeated | No JS hooks = no prototype tampering artifacts |
| FingerprintJS Pro Tampering | ✅ Defeated | Native C++ changes, normal property descriptors |
| FingerprintJS Pro VM Detection | ✅ Defeated | Reports real mobile device, not VM |
| **Cloudflare Bot Management (TLS)** | **✅ Defeated** | **BoringSSL JA3/JA4 randomization — unique hash per session** |
| **Cloudflare Bot Management (JS)** | **✅ Defeated** | **Turnstile/managed challenge sees natural browser behavior** |
| Canvas Fingerprinting | ✅ Defeated | Genuine rendering differences at GPU/Skia level |
| WebGL Fingerprinting | ✅ Defeated | Different GPU strings + rendering |
| Audio Fingerprinting | ✅ Defeated | Different processing at DSP level |
| Font Fingerprinting | ✅ Defeated | Manufacturer-specific font lists |
| Screen Fingerprinting | ✅ Defeated | Native screen API returns profile values |
| UA/Client Hints Matching | ✅ Defeated | Same values in HTTP headers and JS |
| Sensor Fingerprinting | ✅ Defeated | Profile-matched sensor names + noise |
| **Device Fingerprint Uniqueness** | **✅ Defeated** | **Procedural generation = infinite devices, no repeats** |
| **usbrowserspeed.com** | **✅ Defeated** | **All JS APIs return native C++ values** |
| WebRTC IP Leak | ✅ Defeated | Fake local IP per session or disabled |
| ClientRect Fingerprinting | ✅ Defeated | Sub-pixel noise at layout level |
| **Math Fingerprinting (FPU)** | **✅ Defeated** | **Software math (FDLIBM) + per-session ULP perturbation** |
| **Emoji Rendering** | **✅ Defeated** | **Bundled OEM emoji fonts (Samsung/Google) loaded at Skia level** |
| **Performance Timing** | **✅ Defeated** | **Artificial delays match claimed SoC tier** |
| **Storage/Memory Fingerprinting** | **✅ Defeated** | **Spoofed storage.estimate(), performance.memory, connection API** |
| **Speech Synthesis Voices** | **✅ Defeated** | **Manufacturer-appropriate voice list returned** |
| **WebGPU Adapter Info** | **✅ Defeated** | **Spoofed vendor/arch/device or disabled for budget SoCs** |
| **CSS Media Queries** | **✅ Defeated** | **matchMedia() returns profile-consistent values** |
| **Root/Frida/Magisk Detection** | **🟡 Bonus** | **SDK-only signal — web JS agent CANNOT detect these** |

### Signals that DON'T APPLY to web browsing (Native SDK only):

| Signal | Why Not Applicable |
|---|---|
| `jailbroken` (root) | Web JS agent cannot read `/system/bin/su` — browser sandbox blocks it |
| `frida` | Web JS agent cannot read `/proc/self/maps` — browser sandbox blocks it |
| `clonedApp` | Web JS agent cannot check package installer metadata |
| `factoryReset` | Web JS agent cannot check device setup timestamp |

> **These 4 signals require the FingerprintJS Pro Native Android SDK** (`com.fingerprintjs.android`), which must be integrated into an APK. Websites using the web `<script>` agent (like Polymarket) **never receive these signals**.

### What this browser CANNOT defeat (need separate tools):

| Detection Method | Status | Solution |
|---|---|---|
| IP Fingerprinting | ❌ Need proxy | Residential rotating proxy (Indian IPs) |
| IP Reputation/Blocklist | ❌ Need proxy | Same — residential, NOT datacenter |
| Behavioral Biometrics | ❌ Need variation | Vary browsing patterns, timing, scrolling |
| Server-side Rate Limiting | ❌ Need proxy | Rotate IPs between sessions |
| Wallet Graph Analysis (fun.xyz) | ❌ Need wallet hygiene | Fund each wallet from different source |
| Email Correlation | ❌ Need fresh emails | New email per account |

---

## PART 26: TESTING CHECKLIST

After building, verify against these fingerprinting services:

1. **Your own site**: https://fingerprintcheck-endw.onrender.com/
   - [ ] Different visitor ID on each "New Identity"
   - [ ] Tampering: false
   - [ ] Bot: "notDetected"  
   - [ ] VM: false
   - [ ] Risk score < 30

2. **FingerprintJS Demo**: https://fingerprint.com/demo/
   - [ ] Different visitor ID each time
   - [ ] All signals appear natural

3. **BrowserLeaks**: https://browserleaks.com/
   - [ ] Canvas: unique hash per identity
   - [ ] WebGL: shows spoofed GPU
   - [ ] Audio: unique hash per identity
   - [ ] Fonts: manufacturer-appropriate list

4. **CreepJS**: https://abrahamjuliot.github.io/creepjs/
   - [ ] Lies/Trust score: HIGH trust
   - [ ] No "tampering detected" warnings

5. **AmIUnique**: https://amiunique.org/
   - [ ] All values match the claimed device

6. **Math.* Fingerprint Consistency**:
   - [ ] `Math.tan(-1e300)` returns per-session unique value
   - [ ] `Math.atan2(1,2)` returns per-session unique value
   - [ ] Results are within valid double-precision range (not obviously fake)

7. **Cloudflare TLS Test**: https://tls.browserleaks.com/ and https://ja3.zone/
   - [ ] Different JA3 hash on each "New Identity"
   - [ ] Different JA4 hash on each "New Identity"
   - [ ] TLS handshake completes successfully every time
   - [ ] Cloudflare-protected sites don't block or challenge

8. **Emoji & Storage Tests**:
   - [ ] Canvas emoji rendering matches claimed manufacturer (Samsung emoji for Samsung profile)
   - [ ] `navigator.storage.estimate()` returns device-tier-appropriate values
   - [ ] `speechSynthesis.getVoices()` returns manufacturer-appropriate voices
   - [ ] `matchMedia('(hover: hover)')` returns false (mobile)
   - [ ] `matchMedia('(pointer: fine)')` returns false (touch, not mouse)
   - [ ] WebGPU disabled for budget SoC profiles, enabled with correct info for flagships

9. **Polymarket** (the actual target):
   - [ ] FPJS produces different visitorId each session
   - [ ] Cloudflare doesn't issue challenge page
   - [ ] No Turnstile CAPTCHA triggered
   - [ ] Geoblock passes (with Indian residential IP)
   - [ ] Can create multiple accounts without correlation

10. **TLS Consistency Across 100 Sessions**:
    - [ ] No two sessions share the same JA3 hash
    - [ ] All sessions pass Cloudflare without challenge
    - [ ] No session produces an "impossible" TLS fingerprint

11. **Performance Timing Correlation**:
    - [ ] When claiming budget SoC → canvas.toDataURL() takes 50-100ms
    - [ ] When claiming flagship SoC → canvas.toDataURL() takes 5-12ms
    - [ ] Timing matches expected range for claimed chipset

---

## PART 27: ESTIMATED EFFORT

| Component | Difficulty | Time Estimate |
|---|---|---|
| Build environment setup | Medium | 1-2 days |
| **Procedural device generator** | **Medium** | **3-4 days** |
| **SoC/GPU database (25+ SoCs)** | **Medium** | **2 days** |
| **Model name + build fingerprint generators** | **Easy** | **1-2 days** |
| Profile store singleton | Easy | 1 day |
| Navigator/Screen properties | Easy | 1 day |
| User-Agent & Client Hints | Medium | 1-2 days |
| Canvas fingerprint (Skia) | Hard | 3-5 days |
| WebGL fingerprint | Hard | 2-3 days |
| Audio fingerprint | Medium | 1-2 days |
| Font enumeration | Medium | 1-2 days |
| Sensor fingerprinting | Easy | 1 day |
| Timezone/Locale | Easy | 0.5 days |
| Media devices | Easy | 0.5 days |
| ClientRect noise | Easy | 0.5 days |
| WebRTC IP protection | Easy | 0.5 days |
| **Math fingerprint (V8 software math)** | **Hard** | **3-4 days** |
| **Emoji/Unicode font bundling** | **Medium** | **2-3 days** |
| **Performance timing normalization** | **Medium** | **2 days** |
| **Storage/Memory/Network spoofing** | **Easy** | **1 day** |
| **Speech synthesis voices** | **Easy** | **0.5 days** |
| **WebGPU adapter spoofing** | **Easy** | **0.5 days** |
| **CSS media query overrides** | **Easy** | **0.5 days** |
| **Root/Frida/Magisk hiding (BONUS)** | **Medium** | **1-2 days (optional — not needed for web)** |
| **BoringSSL TLS randomization (JA3/JA4)** | **Very Hard** | **5-8 days** |
| **TLS profile generation & validation** | **Hard** | **2-3 days** |
| UI (New Identity / one-click menu) | Easy | 1-2 days |
| Testing & debugging | Hard | 5-7 days |
| **TOTAL (web bypass)** | | **~38-52 days (skip Part 21 for web-only)** |

---

## FINAL NOTES

1. **Keep Chromium version updated** — Fingerprinting services flag outdated browser versions. Set up a CI/CD pipeline to rebase your changes onto new Chromium releases monthly.

2. **Chrome version in UA must match real Chrome releases** — Don't use arbitrary version numbers. Use the latest stable Chrome version number.

3. **Test on REAL Android device** — Emulators have their own detection vectors (different /proc/cpuinfo, missing sensors). Deploy to a real phone.

4. **Indian timezone only** — Since all device profiles are Indian market phones and you're using Indian IP, always set timezone to Asia/Kolkata.

5. **Pair with residential proxy rotation** — For complete undetectability, rotate Indian residential IPs. The browser handles device identity; the proxy handles network identity.

6. **TLS randomization must be tested against real Cloudflare** — Set up a Cloudflare-protected test page and verify that different sessions produce different JA3/JA4 hashes and all pass bot detection.

7. **Device profiles are UNLIMITED** — The procedural generator creates infinite unique devices. There is no database to exhaust. Even if a fingerprinting service catalogues every profile you've ever used, the next one will be completely new.

8. **One-click workflow** — The user experience must be: press "New Identity" → everything changes (device, fingerprint, TLS profile, cookies, cache, storage). Zero configuration required.

9. **Never generate impossible devices** — Procedural generation must follow real constraints (e.g., Samsung can't have a Dimensity SoC, budget phones can't have 16GB RAM, 720p screens can't have 3.5x density). The validator catches these.
