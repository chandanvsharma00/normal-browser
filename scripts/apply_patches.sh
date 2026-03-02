#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# apply_patches.sh — Apply all Normal Browser patches to a Chromium source tree.
#
# Usage: ./apply_patches.sh /path/to/chromium/src
#
# This script copies all patch files into the correct locations in the
# Chromium source tree, then calls apply_hook_points.sh to wire them in.
# Run this AFTER fetching Chromium source and BEFORE running `gn gen`.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_SRC="$SCRIPT_DIR/../src"
CHROMIUM_SRC="${1:?Usage: $0 /path/to/chromium/src}"

if [ ! -d "$CHROMIUM_SRC/chrome" ]; then
  echo "ERROR: $CHROMIUM_SRC does not look like a Chromium source tree."
  echo "Expected to find chrome/ directory."
  exit 1
fi

echo "=== Normal Browser Patch Applier ==="
echo "Patch source: $PATCH_SRC"
echo "Chromium src: $CHROMIUM_SRC"
echo ""

copy_file() {
  local src="$1"
  local dst="$2"
  mkdir -p "$(dirname "$dst")"
  cp -v "$src" "$dst"
}

FAIL=0

# ====================================================================
# Step 1: Copy standalone modules (with their own BUILD.gn)
# ====================================================================
echo "[1/7] Copying device_profile_generator module..."
mkdir -p "$CHROMIUM_SRC/device_profile_generator"
cp -v "$PATCH_SRC/device_profile_generator/"*.h \
      "$PATCH_SRC/device_profile_generator/"*.cc \
      "$PATCH_SRC/device_profile_generator/BUILD.gn" \
      "$CHROMIUM_SRC/device_profile_generator/"

echo "[2/7] Copying software_math module..."
mkdir -p "$CHROMIUM_SRC/software_math"
cp -v "$PATCH_SRC/software_math/"*.h \
      "$PATCH_SRC/software_math/"*.cc \
      "$PATCH_SRC/software_math/BUILD.gn" \
      "$CHROMIUM_SRC/software_math/"

echo "[3/7] Copying tls_randomizer module..."
mkdir -p "$CHROMIUM_SRC/tls_randomizer"
cp -v "$PATCH_SRC/tls_randomizer/"*.h \
      "$PATCH_SRC/tls_randomizer/"*.cc \
      "$PATCH_SRC/tls_randomizer/BUILD.gn" \
      "$CHROMIUM_SRC/tls_randomizer/"

# ====================================================================
# Step 2: Copy Mojo IPC interface + host/client
# ====================================================================
echo "[4/7] Copying Mojo IPC interface and host/client..."

# Mojom definition → content/common/
copy_file "$PATCH_SRC/chromium_patches/content/ghost_profile.mojom" \
          "$CHROMIUM_SRC/content/common/ghost_profile.mojom"

# Mojo host (browser process) → content/browser/ghost_profile/
copy_file "$PATCH_SRC/chromium_patches/content/ghost_profile_host.h" \
          "$CHROMIUM_SRC/content/browser/ghost_profile/ghost_profile_host.h"
copy_file "$PATCH_SRC/chromium_patches/content/ghost_profile_host.cc" \
          "$CHROMIUM_SRC/content/browser/ghost_profile/ghost_profile_host.cc"

# Mojo client (renderer process) → third_party/blink/renderer/core/
copy_file "$PATCH_SRC/chromium_patches/blink/ghost_profile_client.h" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/ghost_profile_client.h"
copy_file "$PATCH_SRC/chromium_patches/blink/ghost_profile_client.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/ghost_profile_client.cc"

# ====================================================================
# Step 3: Copy Blink renderer patches into actual Chromium paths
# ====================================================================
echo "[5/7] Copying Blink renderer patches..."

copy_file "$PATCH_SRC/chromium_patches/blink/navigator/navigator_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/frame/navigator_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/screen/screen_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/frame/screen_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/canvas/canvas_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/canvas/canvas2d/canvas_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/webgl/webgl_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/webgl/webgl_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/audio/audio_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/webaudio/audio_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/fonts/font_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/platform/fonts/font_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/timing/performance_timing_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/timing/performance_timing_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/sensors/sensor_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/sensor/sensor_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/storage/storage_memory_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/storage/storage_memory_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/media/media_devices_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/mediastream/media_devices_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/webrtc/webrtc_ip_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/modules/peerconnection/webrtc_ip_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/css_webgpu_speech/css_webgpu_speech_spoofing.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/css/css_webgpu_speech_spoofing.cc"

copy_file "$PATCH_SRC/chromium_patches/blink/anti_bot/anti_bot_markers.h" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/anti_bot/anti_bot_markers.h"
copy_file "$PATCH_SRC/chromium_patches/blink/anti_bot/anti_bot_markers.cc" \
          "$CHROMIUM_SRC/third_party/blink/renderer/core/anti_bot/anti_bot_markers.cc"

# ====================================================================
# Step 4: Copy V8 / BoringSSL / Network / Timezone patches
# ====================================================================
echo "[6/7] Copying V8, BoringSSL, Network, Timezone, Sandbox patches..."

# V8 Math → v8/src/builtins/
copy_file "$PATCH_SRC/chromium_patches/v8/v8_math_patch.h" \
          "$CHROMIUM_SRC/v8/src/builtins/v8_math_patch.h"
copy_file "$PATCH_SRC/chromium_patches/v8/v8_math_patch.cc" \
          "$CHROMIUM_SRC/v8/src/builtins/v8_math_patch.cc"

# BoringSSL TLS → third_party/boringssl/src/ssl/
copy_file "$PATCH_SRC/chromium_patches/boringssl/boringssl_tls_patch.h" \
          "$CHROMIUM_SRC/third_party/boringssl/src/ssl/boringssl_tls_patch.h"
copy_file "$PATCH_SRC/chromium_patches/boringssl/boringssl_tls_patch.cc" \
          "$CHROMIUM_SRC/third_party/boringssl/src/ssl/boringssl_tls_patch.cc"

# Network HTTP headers → net/http/
copy_file "$PATCH_SRC/chromium_patches/network/http_header_patch.h" \
          "$CHROMIUM_SRC/net/http/http_header_patch.h"
copy_file "$PATCH_SRC/chromium_patches/network/http_header_patch.cc" \
          "$CHROMIUM_SRC/net/http/http_header_patch.cc"

# Timezone → base/i18n/
copy_file "$PATCH_SRC/chromium_patches/timezone/timezone_locale_patch.cc" \
          "$CHROMIUM_SRC/base/i18n/timezone_locale_patch.cc"

# Sandbox → sandbox/linux/
copy_file "$PATCH_SRC/chromium_patches/sandbox/proc_access_sandbox.cc" \
          "$CHROMIUM_SRC/sandbox/linux/proc_access_sandbox.cc"

# Browser startup → chrome/browser/ghost/
copy_file "$PATCH_SRC/chromium_patches/browser/browser_startup_wiring.h" \
          "$CHROMIUM_SRC/chrome/browser/ghost/browser_startup_wiring.h"
copy_file "$PATCH_SRC/chromium_patches/browser/browser_startup_wiring.cc" \
          "$CHROMIUM_SRC/chrome/browser/ghost/browser_startup_wiring.cc"

# ====================================================================
# Step 5: Copy Android Java + JNI files
# ====================================================================
echo "[7/7] Copying Android Java and JNI files..."

JAVA_DST="$CHROMIUM_SRC/chrome/android/java/src/org/nicebrowser/ghost"
mkdir -p "$JAVA_DST"
cp -v "$PATCH_SRC/chromium_patches/android/java/org/nicebrowser/ghost/"*.java \
      "$JAVA_DST/"

copy_file "$PATCH_SRC/chromium_patches/android/native/ghost_mode_jni_bridge.cc" \
          "$CHROMIUM_SRC/chrome/browser/ghost/android/ghost_mode_jni_bridge.cc"

echo ""
echo "=== All patch files copied to Chromium source tree! ==="
echo ""
echo "NEXT STEPS:"
echo "  1. Run: bash scripts/apply_hook_points.sh $CHROMIUM_SRC"
echo "     (Applies inline edits to existing Chromium files)"
echo "  2. Run: bash scripts/apply_build_gn_patches.sh $CHROMIUM_SRC"
echo "     (Updates BUILD.gn files to include our sources)"
echo "  3. Run: gn gen out/NormalBrowser --args='...'"
echo "  4. Run: autoninja -C out/NormalBrowser chrome_public_apk"
echo ""
