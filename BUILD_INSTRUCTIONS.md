# Normal Browser — Build Instructions

Complete guide to compile this project into a working Chromium Android APK.

---

## Prerequisites

### Hardware Requirements
- **CPU**: 16+ cores recommended (8 minimum)
- **RAM**: 64 GB recommended (32 GB minimum, with swap)
- **Disk**: 250 GB free space (Chromium source + build output)
- **OS**: Ubuntu 22.04/24.04 LTS (x86_64)

### Software Requirements
```bash
# Install depot_tools (Chromium's build toolchain)
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$PATH:$HOME/depot_tools"  # Add to ~/.bashrc

# Install system dependencies
sudo apt-get update
sudo apt-get install -y build-essential git python3 python3-pip curl \
  lsb-release pkg-config libglib2.0-dev libdrm-dev libva-dev \
  lib32gcc-s1 libc6-i386 linux-libc-dev clang lld
```

---

## Step 1: Fetch Chromium Source

```bash
mkdir ~/chromium && cd ~/chromium
fetch --nohooks android

# Target Chrome 130 (our patches are tested against this version)
cd src
git checkout tags/130.0.6723.58 -b normal-browser-build

# Install build dependencies
./build/install-build-deps.sh --android
gclient runhooks
```

**Expected time**: 30-60 minutes (depending on network speed)
**Expected disk usage**: ~60 GB after fetch

---

## Step 2: Configure Build (GN args)

```bash
cd ~/chromium/src
gn gen out/Release --args='
  target_os = "android"
  target_cpu = "arm64"
  is_debug = false
  is_official_build = true
  is_component_build = false
  is_clang = true
  use_goma = false
  symbol_level = 0
  enable_nacl = false
  blink_symbol_level = 0
  v8_symbol_level = 0
  chrome_public_manifest_package = "org.nicebrowser.normal"
  android_channel = "stable"
  android_default_version_name = "130.0.6723.58"
  treat_warnings_as_errors = false
'
```

For **debug builds** (faster iteration, but larger APK):
```bash
gn gen out/Debug --args='
  target_os = "android"
  target_cpu = "arm64"
  is_debug = true
  is_component_build = true
  is_clang = true
  symbol_level = 1
  chrome_public_manifest_package = "org.nicebrowser.normal"
'
```

---

## Step 3: Apply Normal Browser Patches

Clone this repo and run the patch application script:

```bash
# Clone Normal Browser source
cd ~/chromium
git clone https://github.com/arjunkishannohal/normal-browser.git normal-browser-patches

# Run the patch script
cd ~/chromium/src
bash ../normal-browser-patches/apply_patches.sh
```

### What `apply_patches.sh` Does

The script copies our source files into the correct locations within the Chromium tree and patches existing Chromium files to hook our code in. Here's the mapping:

#### A. New Files (copied into Chromium tree)

| Our File | Chromium Destination |
|----------|---------------------|
| `src/device_profile_generator/*` | `components/device_profile_generator/` |
| `src/software_math/*` | `components/software_math/` |
| `src/tls_randomizer/*` | `components/tls_randomizer/` |
| `src/chromium_patches/blink/ghost_profile_client.h` | `third_party/blink/renderer/core/ghost_profile_client.h` |
| `src/chromium_patches/blink/ghost_profile_client.cc` | `third_party/blink/renderer/core/ghost_profile_client.cc` |
| `src/chromium_patches/blink/navigator/navigator_spoofing.cc` | `third_party/blink/renderer/core/frame/navigator_spoofing.cc` |
| `src/chromium_patches/blink/screen/screen_spoofing.cc` | `third_party/blink/renderer/core/frame/screen_spoofing.cc` |
| `src/chromium_patches/blink/canvas/canvas_spoofing.cc` | `third_party/blink/renderer/modules/canvas/canvas_spoofing.cc` |
| `src/chromium_patches/blink/webgl/webgl_spoofing.cc` | `third_party/blink/renderer/modules/webgl/webgl_spoofing.cc` |
| `src/chromium_patches/blink/audio/audio_spoofing.cc` | `third_party/blink/renderer/modules/webaudio/audio_spoofing.cc` |
| `src/chromium_patches/blink/fonts/font_spoofing.cc` | `third_party/blink/renderer/platform/fonts/font_spoofing.cc` |
| `src/chromium_patches/blink/timing/performance_timing_spoofing.cc` | `third_party/blink/renderer/core/timing/performance_timing_spoofing.cc` |
| `src/chromium_patches/blink/sensors/sensor_spoofing.cc` | `third_party/blink/renderer/modules/sensor/sensor_spoofing.cc` |
| `src/chromium_patches/blink/storage/storage_memory_spoofing.cc` | `third_party/blink/renderer/modules/storage/storage_memory_spoofing.cc` |
| `src/chromium_patches/blink/media/media_devices_spoofing.cc` | `third_party/blink/renderer/modules/mediastream/media_devices_spoofing.cc` |
| `src/chromium_patches/blink/webrtc/webrtc_ip_spoofing.cc` | `third_party/blink/renderer/modules/peerconnection/webrtc_ip_spoofing.cc` |
| `src/chromium_patches/blink/css_webgpu_speech/css_webgpu_speech_spoofing.cc` | `third_party/blink/renderer/core/css/css_webgpu_speech_spoofing.cc` |
| `src/chromium_patches/blink/anti_bot/anti_bot_markers.h` | `third_party/blink/renderer/core/anti_bot/anti_bot_markers.h` |
| `src/chromium_patches/blink/anti_bot/anti_bot_markers.cc` | `third_party/blink/renderer/core/anti_bot/anti_bot_markers.cc` |
| `src/chromium_patches/v8/v8_math_patch.h` | `v8/src/builtins/v8_math_patch.h` |
| `src/chromium_patches/v8/v8_math_patch.cc` | `v8/src/builtins/v8_math_patch.cc` |
| `src/chromium_patches/content/ghost_profile.mojom` | `content/browser/ghost_profile/ghost_profile.mojom` |
| `src/chromium_patches/content/ghost_profile_host.h` | `content/browser/ghost_profile/ghost_profile_host.h` |
| `src/chromium_patches/content/ghost_profile_host.cc` | `content/browser/ghost_profile/ghost_profile_host.cc` |
| `src/chromium_patches/browser/browser_startup_wiring.h` | `chrome/browser/ghost/browser_startup_wiring.h` |
| `src/chromium_patches/browser/browser_startup_wiring.cc` | `chrome/browser/ghost/browser_startup_wiring.cc` |
| `src/chromium_patches/network/http_header_patch.h` | `net/http/http_header_patch.h` |
| `src/chromium_patches/network/http_header_patch.cc` | `net/http/http_header_patch.cc` |
| `src/chromium_patches/boringssl/boringssl_tls_patch.h` | `third_party/boringssl/src/ssl/boringssl_tls_patch.h` |
| `src/chromium_patches/boringssl/boringssl_tls_patch.cc` | `third_party/boringssl/src/ssl/boringssl_tls_patch.cc` |
| `src/chromium_patches/sandbox/proc_access_sandbox.cc` | `sandbox/linux/proc_access_sandbox.cc` |
| `src/chromium_patches/timezone/timezone_locale_patch.cc` | `base/i18n/timezone_locale_patch.cc` |
| `src/chromium_patches/android/native/ghost_mode_jni_bridge.cc` | `chrome/browser/ghost/android/ghost_mode_jni_bridge.cc` |
| `src/chromium_patches/android/java/org/nicebrowser/ghost/*.java` | `chrome/android/java/src/org/nicebrowser/ghost/` |

#### B. Existing Chromium Files to Patch (hook points)

These are the minimal edits to existing Chromium source files to call our code:

**1. Navigator — `third_party/blink/renderer/core/frame/navigator.cc`**
```cpp
// At the top, add:
#include "third_party/blink/renderer/core/frame/navigator_spoofing.cc"

// In Navigator::userAgent(), before the return statement:
//   return NavigatorUserAgent::SpoofedUserAgent(this);
// (Replace the existing return with our spoofed version)
```

**2. Screen — `third_party/blink/renderer/core/frame/screen.cc`**
```cpp
// Hook Screen::width(), Screen::height(), Screen::colorDepth(), Screen::pixelDepth()
// to call our screen_spoofing.cc functions
```

**3. Canvas — `third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc`**
```cpp
// In toDataURL() / toBlob() / getImageData():
//   Call ApplyCanvasNoise() before encoding
```

**4. WebGL — `third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc`**
```cpp
// Hook getParameter() for RENDERER, VENDOR, UNMASKED_RENDERER, UNMASKED_VENDOR
// Hook getShaderPrecisionFormat()
// Hook getSupportedExtensions()
```

**5. Audio — `third_party/blink/renderer/modules/webaudio/audio_context.cc`**
```cpp
// Hook AudioContext and OfflineAudioContext to add noise via audio_spoofing.cc
```

**6. V8 Math — `v8/src/builtins/builtins-math-gen.cc`**
```cpp
// In MathSin, MathCos, MathTan, MathAtan2, MathLog, MathExp:
//   Apply ULP noise from v8_math_patch.cc
```

**7. TLS — `third_party/boringssl/src/ssl/handshake_client.cc`**
```cpp
// Before sending ClientHello:
//   Call boringssl_tls_patch to shuffle cipher/extension order
```

**8. HTTP Headers — `net/http/http_request_headers.cc`**
```cpp
// Hook SetHeader() to intercept Accept-Language, Sec-CH-UA-* headers
```

**9. Chrome Startup — `chrome/browser/chrome_browser_main.cc`**
```cpp
// In PreMainMessageLoopRun():
//   Call BrowserStartupWiring::Initialize()
```

**10. Content RenderFrame — `content/renderer/render_frame_impl.cc`**
```cpp
// In RenderFrameImpl::Initialize():
//   Initialize GhostProfileClient
```

---

## Step 4: Update BUILD.gn Files

### A. `components/device_profile_generator/BUILD.gn`
Already provided in `src/device_profile_generator/BUILD.gn`. Copy it.

### B. Add to existing BUILD.gn files

**`third_party/blink/renderer/core/BUILD.gn`** — add our .cc files to `blink_core_sources`:
```gn
# In the sources list, add:
"frame/navigator_spoofing.cc",
"frame/screen_spoofing.cc",
"timing/performance_timing_spoofing.cc",
"css/css_webgpu_speech_spoofing.cc",
"ghost_profile_client.cc",
"ghost_profile_client.h",
"anti_bot/anti_bot_markers.cc",
"anti_bot/anti_bot_markers.h",
```

**`third_party/blink/renderer/modules/BUILD.gn`** — add module-level spoofing files:
```gn
"canvas/canvas_spoofing.cc",
"webgl/webgl_spoofing.cc",
"webaudio/audio_spoofing.cc",
"sensor/sensor_spoofing.cc",
"storage/storage_memory_spoofing.cc",
"mediastream/media_devices_spoofing.cc",
"peerconnection/webrtc_ip_spoofing.cc",
```

**`v8/BUILD.gn`** — add math patch:
```gn
"src/builtins/v8_math_patch.cc",
"src/builtins/v8_math_patch.h",
```

**`chrome/browser/BUILD.gn`** — add browser-side files:
```gn
"ghost/browser_startup_wiring.cc",
"ghost/browser_startup_wiring.h",
"ghost/android/ghost_mode_jni_bridge.cc",
```

**`content/browser/BUILD.gn`** — add Mojo host:
```gn
"ghost_profile/ghost_profile_host.cc",
"ghost_profile/ghost_profile_host.h",
```

**`net/http/BUILD.gn`** — add header patch:
```gn
"http_header_patch.cc",
"http_header_patch.h",
```

### C. Dependencies

Each BUILD.gn that includes our files needs deps on:
```gn
deps += [
  "//components/device_profile_generator",
  "//components/software_math",
  "//components/tls_randomizer",
]
```

---

## Step 5: Compile

```bash
cd ~/chromium/src

# Full build (first time: 2-6 hours depending on hardware)
autoninja -C out/Release chrome_public_apk

# For faster iteration after first build:
autoninja -C out/Release chrome_public_apk -j$(nproc)
```

### Common Build Fixes

| Error | Fix |
|-------|-----|
| `undefined reference to GhostProfileClient` | Missing source in BUILD.gn |
| `no member named 'math_seed' in 'DeviceProfile'` | device_profile.h not copied correctly |
| `mojom not found` | Run `python3 mojo/public/tools/mojom/mojom_parser.py` on ghost_profile.mojom |
| `Java compilation error` | Check Java files copied to correct package path |

---

## Step 6: Install & Test

```bash
# Install on connected Android device (USB debugging enabled)
adb install -r out/Release/apks/ChromePublic.apk

# Or for signed release:
# First generate a keystore, then:
# jarsigner + zipalign the APK
```

### Testing Checklist

1. **Basic browsing** — Visit google.com, verify pages load
2. **Ghost Mode toggle** — 3-dot menu → Ghost Mode: ON
3. **New Identity** — 3-dot menu → New Identity → verify toast shows new device name
4. **FingerprintJS Pro** — Visit https://fingerprint.com/demo/
   - Generate identity A, note visitorId
   - Press "New Identity"
   - Refresh fingerprint.com/demo
   - Verify visitorId is DIFFERENT
5. **CreepJS** — Visit https://abrahamjuliot.github.io/creepjs/
   - Check all spoofed fields show realistic values
6. **BrowserLeaks** — Visit https://browserleaks.com/
   - Canvas, WebGL, Audio, Fonts tabs should all show unique hashes per identity

---

## Step 7: Signing for Release

```bash
# Generate keystore (one time)
keytool -genkey -v -keystore normal-browser.keystore \
  -alias normal -keyalg RSA -keysize 2048 -validity 10000

# Sign the APK
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
  -keystore normal-browser.keystore \
  out/Release/apks/ChromePublic.apk normal

# Align
zipalign -v 4 out/Release/apks/ChromePublic.apk NormalBrowser.apk
```

---

## Troubleshooting

### Memory Issues During Build
```bash
# If OOM during linking, add swap:
sudo fallocate -l 32G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# Limit parallel jobs:
autoninja -C out/Release chrome_public_apk -j8
```

### Incremental Builds
After first successful build, only changed files recompile (~2-5 min):
```bash
autoninja -C out/Release chrome_public_apk
```

---

## File Count Summary

| Component | Files | Lines (approx) |
|-----------|-------|-----------------|
| Device Profile Generator | 14 (.h + .cc) | ~3,500 |
| Blink Patches | 20 (.h + .cc) | ~4,000 |
| V8 Math Patch | 2 | ~200 |
| Content/Mojo | 3 | ~400 |
| Browser Wiring | 2 | ~300 |
| Network/HTTP | 2 | ~250 |
| BoringSSL/TLS | 2 | ~350 |
| Sandbox/Timezone | 2 | ~200 |
| Android Java | 4 | ~900 |
| **Total** | **51** | **~10,100** |
