#!/bin/bash
# apply_patches.sh — Copy Normal Browser source files into the Chromium tree
#
# Usage:
#   cd ~/chromium/src
#   bash /path/to/normal-browser/apply_patches.sh
#
# Assumes:
#   - CWD is ~/chromium/src (the Chromium source root)
#   - This repo is cloned somewhere accessible
#   - Chromium source is at Chrome 130 (tags/130.0.6723.58)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/src"
CHROMIUM_ROOT="$(pwd)"

echo "============================================="
echo " Normal Browser — Patch Application Script"
echo "============================================="
echo ""
echo "Source:   $SRC_DIR"
echo "Target:   $CHROMIUM_ROOT"
echo ""

# Verify we're in a Chromium tree
if [[ ! -f "$CHROMIUM_ROOT/BUILD.gn" ]] || [[ ! -d "$CHROMIUM_ROOT/third_party/blink" ]]; then
    echo "ERROR: This doesn't look like a Chromium source tree."
    echo "       Run this script from ~/chromium/src/"
    exit 1
fi

# ============================================================
# Helper: copy file with directory creation
# ============================================================
copy_file() {
    local src="$1"
    local dest="$CHROMIUM_ROOT/$2"
    mkdir -p "$(dirname "$dest")"
    cp -v "$src" "$dest"
}

echo ""
echo ">>> Step 1: Copying new component libraries..."
echo ""

# software_math
copy_file "$SRC_DIR/software_math/software_math.h"  "components/software_math/software_math.h"
copy_file "$SRC_DIR/software_math/software_math.cc" "components/software_math/software_math.cc"
copy_file "$SRC_DIR/software_math/BUILD.gn"         "components/software_math/BUILD.gn"

# tls_randomizer
copy_file "$SRC_DIR/tls_randomizer/tls_randomizer.h"  "components/tls_randomizer/tls_randomizer.h"
copy_file "$SRC_DIR/tls_randomizer/tls_randomizer.cc" "components/tls_randomizer/tls_randomizer.cc"
copy_file "$SRC_DIR/tls_randomizer/BUILD.gn"          "components/tls_randomizer/BUILD.gn"

# device_profile_generator
for f in "$SRC_DIR"/device_profile_generator/*.{h,cc}; do
    [[ -f "$f" ]] && copy_file "$f" "components/device_profile_generator/$(basename "$f")"
done
copy_file "$SRC_DIR/device_profile_generator/BUILD.gn" "components/device_profile_generator/BUILD.gn"

echo ""
echo ">>> Step 2: Copying Blink renderer patches..."
echo ""

# Ghost Profile Client (renderer-side Mojo cache)
copy_file "$SRC_DIR/chromium_patches/blink/ghost_profile_client.h"  "third_party/blink/renderer/core/ghost_profile_client.h"
copy_file "$SRC_DIR/chromium_patches/blink/ghost_profile_client.cc" "third_party/blink/renderer/core/ghost_profile_client.cc"

# Navigator
copy_file "$SRC_DIR/chromium_patches/blink/navigator/navigator_spoofing.cc" \
    "third_party/blink/renderer/core/frame/navigator_spoofing.cc"

# Screen
copy_file "$SRC_DIR/chromium_patches/blink/screen/screen_spoofing.cc" \
    "third_party/blink/renderer/core/frame/screen_spoofing.cc"

# Canvas
copy_file "$SRC_DIR/chromium_patches/blink/canvas/canvas_spoofing.cc" \
    "third_party/blink/renderer/modules/canvas/canvas_spoofing.cc"

# WebGL
copy_file "$SRC_DIR/chromium_patches/blink/webgl/webgl_spoofing.cc" \
    "third_party/blink/renderer/modules/webgl/webgl_spoofing.cc"

# Audio
copy_file "$SRC_DIR/chromium_patches/blink/audio/audio_spoofing.cc" \
    "third_party/blink/renderer/modules/webaudio/audio_spoofing.cc"

# Fonts
copy_file "$SRC_DIR/chromium_patches/blink/fonts/font_spoofing.cc" \
    "third_party/blink/renderer/platform/fonts/font_spoofing.cc"

# Timing
copy_file "$SRC_DIR/chromium_patches/blink/timing/performance_timing_spoofing.cc" \
    "third_party/blink/renderer/core/timing/performance_timing_spoofing.cc"

# Sensors
copy_file "$SRC_DIR/chromium_patches/blink/sensors/sensor_spoofing.cc" \
    "third_party/blink/renderer/modules/sensor/sensor_spoofing.cc"

# Storage / Network Info
copy_file "$SRC_DIR/chromium_patches/blink/storage/storage_memory_spoofing.cc" \
    "third_party/blink/renderer/modules/storage/storage_memory_spoofing.cc"

# Media Devices
copy_file "$SRC_DIR/chromium_patches/blink/media/media_devices_spoofing.cc" \
    "third_party/blink/renderer/modules/mediastream/media_devices_spoofing.cc"

# WebRTC
copy_file "$SRC_DIR/chromium_patches/blink/webrtc/webrtc_ip_spoofing.cc" \
    "third_party/blink/renderer/modules/peerconnection/webrtc_ip_spoofing.cc"

# CSS / WebGPU / Speech
copy_file "$SRC_DIR/chromium_patches/blink/css_webgpu_speech/css_webgpu_speech_spoofing.cc" \
    "third_party/blink/renderer/core/css/css_webgpu_speech_spoofing.cc"

# Anti-Bot Markers
copy_file "$SRC_DIR/chromium_patches/blink/anti_bot/anti_bot_markers.h" \
    "third_party/blink/renderer/core/anti_bot/anti_bot_markers.h"
copy_file "$SRC_DIR/chromium_patches/blink/anti_bot/anti_bot_markers.cc" \
    "third_party/blink/renderer/core/anti_bot/anti_bot_markers.cc"

echo ""
echo ">>> Step 3: Copying V8 Math patch..."
echo ""

copy_file "$SRC_DIR/chromium_patches/v8/v8_math_patch.h"  "v8/src/builtins/v8_math_patch.h"
copy_file "$SRC_DIR/chromium_patches/v8/v8_math_patch.cc" "v8/src/builtins/v8_math_patch.cc"

echo ""
echo ">>> Step 4: Copying Content/Mojo IPC..."
echo ""

copy_file "$SRC_DIR/chromium_patches/content/ghost_profile.mojom" \
    "content/browser/ghost_profile/ghost_profile.mojom"
copy_file "$SRC_DIR/chromium_patches/content/ghost_profile_host.h" \
    "content/browser/ghost_profile/ghost_profile_host.h"
copy_file "$SRC_DIR/chromium_patches/content/ghost_profile_host.cc" \
    "content/browser/ghost_profile/ghost_profile_host.cc"

echo ""
echo ">>> Step 5: Copying Browser-side wiring..."
echo ""

copy_file "$SRC_DIR/chromium_patches/browser/browser_startup_wiring.h" \
    "chrome/browser/ghost/browser_startup_wiring.h"
copy_file "$SRC_DIR/chromium_patches/browser/browser_startup_wiring.cc" \
    "chrome/browser/ghost/browser_startup_wiring.cc"

echo ""
echo ">>> Step 6: Copying Network/HTTP header patch..."
echo ""

copy_file "$SRC_DIR/chromium_patches/network/http_header_patch.h"  "net/http/http_header_patch.h"
copy_file "$SRC_DIR/chromium_patches/network/http_header_patch.cc" "net/http/http_header_patch.cc"

echo ""
echo ">>> Step 7: Copying BoringSSL/TLS patch..."
echo ""

copy_file "$SRC_DIR/chromium_patches/boringssl/boringssl_tls_patch.h" \
    "third_party/boringssl/src/ssl/boringssl_tls_patch.h"
copy_file "$SRC_DIR/chromium_patches/boringssl/boringssl_tls_patch.cc" \
    "third_party/boringssl/src/ssl/boringssl_tls_patch.cc"

echo ""
echo ">>> Step 8: Copying Sandbox & Timezone patches..."
echo ""

copy_file "$SRC_DIR/chromium_patches/sandbox/proc_access_sandbox.cc" \
    "sandbox/linux/proc_access_sandbox.cc"
copy_file "$SRC_DIR/chromium_patches/timezone/timezone_locale_patch.cc" \
    "base/i18n/timezone_locale_patch.cc"

echo ""
echo ">>> Step 9: Copying Android JNI bridge & Java files..."
echo ""

copy_file "$SRC_DIR/chromium_patches/android/native/ghost_mode_jni_bridge.cc" \
    "chrome/browser/ghost/android/ghost_mode_jni_bridge.cc"

for f in "$SRC_DIR"/chromium_patches/android/java/org/nicebrowser/ghost/*.java; do
    [[ -f "$f" ]] && copy_file "$f" "chrome/android/java/src/org/nicebrowser/ghost/$(basename "$f")"
done

echo ""
echo "============================================="
echo " Patch application complete!"
echo ""
echo " Files copied: $(find "$SRC_DIR" -type f \( -name '*.h' -o -name '*.cc' -o -name '*.java' -o -name '*.mojom' \) | wc -l)"
echo ""
echo " NEXT STEPS:"
echo "   1. Apply BUILD.gn modifications (see BUILD_INSTRUCTIONS.md Step 4)"
echo "   2. Apply hook-point patches to existing Chromium files (see BUILD_INSTRUCTIONS.md Step 3.B)"
echo "   3. Run: autoninja -C out/Release chrome_public_apk"
echo "============================================="
