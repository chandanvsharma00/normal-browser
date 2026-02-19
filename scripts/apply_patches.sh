#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# apply_patches.sh — Apply all Normal Browser patches to a Chromium source tree.
#
# Usage: ./apply_patches.sh /path/to/chromium/src
#
# This script copies all patch files into the correct locations in the
# Chromium source tree. Run this AFTER fetching Chromium source and
# BEFORE running `gn gen` and `autoninja`.

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

# ====================================================================
# Step 1: Copy our custom modules into the Chromium tree
# ====================================================================
echo "[1/8] Copying device_profile_generator module..."
mkdir -p "$CHROMIUM_SRC/device_profile_generator"
cp -v "$PATCH_SRC/device_profile_generator/"*.h \
      "$PATCH_SRC/device_profile_generator/"*.cc \
      "$PATCH_SRC/device_profile_generator/BUILD.gn" \
      "$CHROMIUM_SRC/device_profile_generator/"

echo "[2/8] Copying software_math module..."
mkdir -p "$CHROMIUM_SRC/software_math"
cp -v "$PATCH_SRC/software_math/"*.h \
      "$PATCH_SRC/software_math/"*.cc \
      "$PATCH_SRC/software_math/BUILD.gn" \
      "$CHROMIUM_SRC/software_math/"

echo "[3/8] Copying tls_randomizer module..."
mkdir -p "$CHROMIUM_SRC/tls_randomizer"
cp -v "$PATCH_SRC/tls_randomizer/"*.h \
      "$PATCH_SRC/tls_randomizer/"*.cc \
      "$PATCH_SRC/tls_randomizer/BUILD.gn" \
      "$CHROMIUM_SRC/tls_randomizer/"

# ====================================================================
# Step 2: Copy Mojo IPC interface definition
# ====================================================================
echo "[4/8] Copying Mojo IPC interface..."
mkdir -p "$CHROMIUM_SRC/content/common"
cp -v "$PATCH_SRC/chromium_patches/content/ghost_profile.mojom" \
      "$CHROMIUM_SRC/content/common/"

# Copy Mojo host (browser side)
cp -v "$PATCH_SRC/chromium_patches/content/ghost_profile_host.h" \
      "$PATCH_SRC/chromium_patches/content/ghost_profile_host.cc" \
      "$CHROMIUM_SRC/content/browser/"

# Copy Mojo client (renderer side)
cp -v "$PATCH_SRC/chromium_patches/blink/ghost_profile_client.h" \
      "$PATCH_SRC/chromium_patches/blink/ghost_profile_client.cc" \
      "$CHROMIUM_SRC/third_party/blink/renderer/core/"

# ====================================================================
# Step 3: Copy Blink renderer patches
# ====================================================================
echo "[5/8] Copying Blink renderer patches..."

# These are REFERENCE files — the actual modifications need to be
# applied as source code edits to existing Chromium files.
# We copy them as reference and the developer applies the patches manually
# or use a diff-based approach.

BLINK_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/blink"
mkdir -p "$BLINK_PATCH_DIR"/{navigator,screen,canvas,webgl,audio,fonts,sensors,media,storage,css_webgpu_speech,anti_bot}

cp -v "$PATCH_SRC/chromium_patches/blink/navigator/navigator_spoofing.cc" \
      "$BLINK_PATCH_DIR/navigator/"
cp -v "$PATCH_SRC/chromium_patches/blink/screen/screen_spoofing.cc" \
      "$BLINK_PATCH_DIR/screen/"
cp -v "$PATCH_SRC/chromium_patches/blink/canvas/canvas_spoofing.cc" \
      "$BLINK_PATCH_DIR/canvas/"
cp -v "$PATCH_SRC/chromium_patches/blink/webgl/webgl_spoofing.cc" \
      "$BLINK_PATCH_DIR/webgl/"
cp -v "$PATCH_SRC/chromium_patches/blink/audio/audio_spoofing.cc" \
      "$BLINK_PATCH_DIR/audio/"
cp -v "$PATCH_SRC/chromium_patches/blink/fonts/font_spoofing.cc" \
      "$BLINK_PATCH_DIR/fonts/"
cp -v "$PATCH_SRC/chromium_patches/blink/sensors/sensor_spoofing.cc" \
      "$BLINK_PATCH_DIR/sensors/"
cp -v "$PATCH_SRC/chromium_patches/blink/media/media_devices_spoofing.cc" \
      "$BLINK_PATCH_DIR/media/"
cp -v "$PATCH_SRC/chromium_patches/blink/storage/storage_memory_spoofing.cc" \
      "$BLINK_PATCH_DIR/storage/"
cp -v "$PATCH_SRC/chromium_patches/blink/css_webgpu_speech/css_webgpu_speech_spoofing.cc" \
      "$BLINK_PATCH_DIR/css_webgpu_speech/"
cp -v "$PATCH_SRC/chromium_patches/blink/anti_bot/anti_bot_markers.cc" \
      "$BLINK_PATCH_DIR/anti_bot/"

# ====================================================================
# Step 4: Copy BoringSSL TLS patches
# ====================================================================
echo "[6/8] Copying BoringSSL TLS patches..."
BORINGSSL_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/boringssl"
mkdir -p "$BORINGSSL_PATCH_DIR"
cp -v "$PATCH_SRC/chromium_patches/boringssl/"*.h \
      "$PATCH_SRC/chromium_patches/boringssl/"*.cc \
      "$BORINGSSL_PATCH_DIR/"

# ====================================================================
# Step 5: Copy V8 Math patches
# ====================================================================
echo "[7/8] Copying V8 Math patches..."
V8_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/v8"
mkdir -p "$V8_PATCH_DIR"
cp -v "$PATCH_SRC/chromium_patches/v8/"*.h \
      "$PATCH_SRC/chromium_patches/v8/"*.cc \
      "$V8_PATCH_DIR/"

# ====================================================================
# Step 6: Copy network/timezone/Android patches
# ====================================================================
echo "[8/8] Copying network, timezone, and Android patches..."

NETWORK_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/network"
mkdir -p "$NETWORK_PATCH_DIR"
cp -v "$PATCH_SRC/chromium_patches/network/"*.h \
      "$PATCH_SRC/chromium_patches/network/"*.cc \
      "$NETWORK_PATCH_DIR/"

TZ_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/timezone"
mkdir -p "$TZ_PATCH_DIR"
cp -v "$PATCH_SRC/chromium_patches/timezone/"*.cc \
      "$TZ_PATCH_DIR/"

ANDROID_PATCH_DIR="$CHROMIUM_SRC/normal_browser_patches/android"
mkdir -p "$ANDROID_PATCH_DIR/java/org/nicebrowser/ghost"
mkdir -p "$ANDROID_PATCH_DIR/native"
cp -v "$PATCH_SRC/chromium_patches/android/java/org/nicebrowser/ghost/"*.java \
      "$ANDROID_PATCH_DIR/java/org/nicebrowser/ghost/"
cp -v "$PATCH_SRC/chromium_patches/android/native/"*.cc \
      "$ANDROID_PATCH_DIR/native/"

echo ""
echo "=== All patches copied successfully! ==="
echo ""
echo "NEXT STEPS:"
echo "  1. Apply source-level edits to existing Chromium files"
echo "     (see each patch file's header for exact files to modify)"
echo "  2. Update BUILD.gn files to include new source files"
echo "  3. Run: gn gen out/Android --args='...'"
echo "  4. Run: autoninja -C out/Android chrome_public_apk"
echo ""
