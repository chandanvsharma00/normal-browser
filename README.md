# Normal Browser

**Anti-fingerprint Chromium-based Android browser** that defeats all 32 FingerprintJS Pro detection vectors at the C++ level.

One press of "New Identity" → completely new device fingerprint, new TLS signature, all data cleared. Zero configuration.

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                    BROWSER PROCESS                   │
│  DeviceProfile ──► ProfileStore ──► GhostProfileHost │
│       ▲                                    │         │
│       │                              Mojo IPC        │
│  ProfileGenerator                          │         │
│  (procedural device)                       ▼         │
│                              ┌─────────────────────┐ │
│                              │  RENDERER PROCESS   │ │
│                              │  GhostProfileClient │ │
│                              │       │             │ │
│                              │  ┌────┴────┐        │ │
│                              │  │ Blink   │        │ │
│                              │  │ Patches │        │ │
│                              │  └─────────┘        │ │
│                              └─────────────────────┘ │
└─────────────────────────────────────────────────────┘
```

## What's Spoofed (44+ API surfaces)

| Category | APIs |
|----------|------|
| **Canvas** | toDataURL, toBlob, getImageData with Skia pixel noise |
| **WebGL** | Renderer, vendor, extensions, shader precision, unmasked strings |
| **Audio** | AudioContext, OfflineAudioContext, oscillator + compressor noise |
| **Navigator** | userAgent, platform, hardwareConcurrency, deviceMemory, languages, plugins, mimeTypes |
| **Screen** | width, height, colorDepth, pixelRatio, availWidth/Height |
| **Math** | sin, cos, tan, atan2, log, exp — ULP-level noise via V8 patch |
| **Fonts** | Manufacturer-matched font lists with sub-pixel width noise |
| **WebRTC** | Local IP masking (mDNS), STUN candidate rewriting |
| **TLS** | Cipher suite order, extension order, GREASE values, ALPS/PSK |
| **Client Hints** | Sec-CH-UA, platform, model, mobile, full-version-list, bitness |
| **Timing** | performance.now() with monotonic jitter, navigation/resource timing |
| **CSS** | prefers-color-scheme, prefers-reduced-motion, forced-colors |
| **Sensors** | Accelerometer, gyroscope, magnetometer noise floor |
| **Storage** | navigator.storage.estimate(), connection.downlink/rtt/effectiveType |
| **Media** | enumerateDevices() with spoofed camera/mic labels and IDs |
| **Speech** | speechSynthesis.getVoices() with OEM-matched voice lists |
| **WebGPU** | Adapter info, limits, features matched to GPU profile |
| **DOM Rect** | getBoundingClientRect() sub-pixel noise |
| **Emoji** | Manufacturer-specific emoji rendering (custom NotoColorEmoji) |
| **Battery** | level, charging, chargingTime, dischargingTime |
| **Permissions** | All permission.query() returns "prompt" |
| **Anti-Bot** | Blocks /proc access, WebDriver detection, toString() native masking |
| **HTTP Headers** | Accept-Language, Sec-CH-UA-* headers matched to profile |
| **Timezone** | ICU timezone + locale matched to profile |

## Directory Structure

```
src/
├── device_profile_generator/    # Procedural device identity generation
│   ├── device_profile.h/cc      # DeviceProfile struct + JSON serialization
│   ├── profile_generator.h/cc   # Main generator (Samsung, Xiaomi, OnePlus, etc.)
│   ├── profile_store.h/cc       # Browser-process singleton store
│   ├── profile_validator.h/cc   # Validates profile consistency
│   ├── gpu_database.h/cc        # Real SoC → GPU mappings
│   ├── manufacturer_traits.h/cc # OEM-specific hardware traits
│   ├── model_name_generator.h/cc # Procedural model names
│   └── build_fingerprint_generator.h/cc # Android build.prop generation
│
├── chromium_patches/
│   ├── blink/                   # Blink renderer patches
│   │   ├── ghost_profile_client.h/cc  # Renderer-side profile cache (Mojo)
│   │   ├── navigator/navigator_spoofing.cc
│   │   ├── screen/screen_spoofing.cc
│   │   ├── canvas/canvas_spoofing.cc
│   │   ├── webgl/webgl_spoofing.cc
│   │   ├── audio/audio_spoofing.cc
│   │   ├── fonts/font_spoofing.cc
│   │   ├── timing/performance_timing_spoofing.cc
│   │   ├── sensors/sensor_spoofing.cc
│   │   ├── storage/storage_memory_spoofing.cc
│   │   ├── media/media_devices_spoofing.cc
│   │   ├── webrtc/webrtc_ip_spoofing.cc
│   │   ├── css_webgpu_speech/css_webgpu_speech_spoofing.cc
│   │   └── anti_bot/anti_bot_markers.h/cc
│   │
│   ├── v8/v8_math_patch.h/cc    # V8 Math.* ULP noise
│   ├── content/                 # Mojo IPC (ghost_profile.mojom, host)
│   ├── browser/                 # Browser startup wiring
│   ├── network/                 # HTTP header patching
│   ├── boringssl/               # TLS fingerprint randomization
│   ├── sandbox/                 # /proc access blocking
│   ├── timezone/                # Timezone/locale spoofing
│   └── android/                 # JNI bridge + Java UI
│
├── software_math/               # Deterministic noise utilities
├── tls_randomizer/              # TLS cipher/extension shuffling
└── emoji_fonts/                 # Custom emoji font (NotoColorEmoji)
```

## Build Instructions

See `BUILD_INSTRUCTIONS.md` for complete step-by-step guide to compile this into a working Chromium APK.

## License

Proprietary — All rights reserved.
