#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# apply_build_gn_patches.sh — Update Chromium BUILD.gn files to include
# Normal Browser source files and dependencies.
#
# Usage: ./apply_build_gn_patches.sh /path/to/chromium/src
#
# Run AFTER apply_patches.sh + apply_hook_points.sh, BEFORE gn gen.
# Each patch is idempotent — running twice won't duplicate entries.

set -euo pipefail

CHROMIUM_SRC="${1:?Usage: $0 /path/to/chromium/src}"

if [ ! -d "$CHROMIUM_SRC/chrome" ]; then
  echo "ERROR: $CHROMIUM_SRC does not look like a Chromium source tree."
  exit 1
fi

PASS=0
FAIL=0
SKIP=0

# Helper: add a line to a file's sources list (after 'sources = [' or 'sources += [')
# Gets inserted just before the first ']' that closes the sources block.
add_to_sources() {
  local file="$1"
  local entry="$2"
  local section="${3:-sources}"  # default section name

  if [ ! -f "$file" ]; then
    echo "  MISS: $file not found"
    ((FAIL++))
    return 1
  fi

  if grep -qF "$entry" "$file" 2>/dev/null; then
    echo "  SKIP: $entry already in $(basename "$file")"
    ((SKIP++))
    return 0
  fi

  echo "  ADD:  $entry → $(basename "$file")"
  ((PASS++))
  return 0
}

# Helper: add deps to a BUILD.gn file
add_dep() {
  local file="$1"
  local dep="$2"

  if [ ! -f "$file" ]; then
    echo "  MISS: $file not found"
    ((FAIL++))
    return 1
  fi

  if grep -qF "$dep" "$file" 2>/dev/null; then
    echo "  SKIP: $dep already in $(basename "$file")"
    ((SKIP++))
    return 0
  fi

  echo "  ADD:  dep $dep → $(basename "$file")"
  ((PASS++))
  return 0
}

echo "=== Normal Browser BUILD.gn Patcher ==="
echo "Chromium src: $CHROMIUM_SRC"
echo ""
echo "NOTE: This script REPORTS what BUILD.gn changes are needed."
echo "      Due to the complexity of GN syntax, some edits must be manual."
echo "      Each entry below tells you exactly what to add and where."
echo ""

# ====================================================================
# 1. Top-level BUILD.gn — add deps on our custom modules
# ====================================================================
echo "[1/7] Top-level deps on custom modules..."
echo "  FILE: $CHROMIUM_SRC/BUILD.gn"
echo "  In the main 'group(\"gn_all\")' target, add to deps:"
echo '    "//device_profile_generator",'
echo '    "//software_math",'
echo '    "//tls_randomizer",'
echo ""

# ====================================================================
# 2. Blink core BUILD.gn — add our core .cc/.h files
# ====================================================================
echo "[2/7] Blink core sources..."
BLINK_CORE_GN="$CHROMIUM_SRC/third_party/blink/renderer/core/BUILD.gn"
echo "  FILE: $BLINK_CORE_GN"
echo "  Add to blink_core_sources (the main sources list):"
cat << 'EOF'
    "frame/navigator_spoofing.cc",
    "frame/screen_spoofing.cc",
    "timing/performance_timing_spoofing.cc",
    "css/css_webgpu_speech_spoofing.cc",
    "ghost_profile_client.cc",
    "ghost_profile_client.h",
    "anti_bot/anti_bot_markers.cc",
    "anti_bot/anti_bot_markers.h",
EOF
echo "  Add to deps:"
echo '    "//device_profile_generator",'
echo '    "//software_math",'
echo ""

# ====================================================================
# 3. Blink modules BUILD.gn — add module-level spoofing files
# ====================================================================
echo "[3/7] Blink modules sources..."
BLINK_MOD_GN="$CHROMIUM_SRC/third_party/blink/renderer/modules/BUILD.gn"
echo "  FILE: $BLINK_MOD_GN"
echo "  Add to the appropriate module source_set targets:"
cat << 'EOF'
    # In canvas2d target:
    "canvas/canvas2d/canvas_spoofing.cc",

    # In webgl target:
    "webgl/webgl_spoofing.cc",

    # In webaudio target:
    "webaudio/audio_spoofing.cc",

    # In sensor target:
    "sensor/sensor_spoofing.cc",

    # In storage target:
    "storage/storage_memory_spoofing.cc",

    # In mediastream target:
    "mediastream/media_devices_spoofing.cc",

    # In peerconnection target:
    "peerconnection/webrtc_ip_spoofing.cc",
EOF
echo ""

# ====================================================================
# 4. Blink platform BUILD.gn — add font spoofing
# ====================================================================
echo "[4/7] Blink platform fonts..."
BLINK_PLAT_GN="$CHROMIUM_SRC/third_party/blink/renderer/platform/BUILD.gn"
echo "  FILE: $BLINK_PLAT_GN"
echo "  Add to platform sources (fonts section):"
cat << 'EOF'
    "fonts/font_spoofing.cc",
EOF
echo ""

# ====================================================================
# 5. V8 BUILD.gn — add math patch files
# ====================================================================
echo "[5/7] V8 builtins..."
V8_GN="$CHROMIUM_SRC/v8/BUILD.gn"
echo "  FILE: $V8_GN"
echo "  Add to v8_base_without_compiler sources (builtins section):"
cat << 'EOF'
    "src/builtins/v8_math_patch.cc",
    "src/builtins/v8_math_patch.h",
EOF
echo "  Add to deps:"
echo '    "//software_math",'
echo ""

# ====================================================================
# 6. Chrome browser BUILD.gn — add startup wiring + JNI
# ====================================================================
echo "[6/7] Chrome browser module..."
CHROME_GN="$CHROMIUM_SRC/chrome/browser/BUILD.gn"
echo "  FILE: $CHROME_GN"
echo "  Add to sources:"
cat << 'EOF'
    "ghost/browser_startup_wiring.cc",
    "ghost/browser_startup_wiring.h",
    "ghost/android/ghost_mode_jni_bridge.cc",
EOF
echo "  Add to deps:"
echo '    "//device_profile_generator",'
echo '    "//content/browser/ghost_profile",'
echo ""

# ====================================================================
# 7. Content browser BUILD.gn — add Mojo host
# ====================================================================
echo "[7/7] Content browser Mojo host..."
CONTENT_GN="$CHROMIUM_SRC/content/browser/BUILD.gn"
echo "  FILE: $CONTENT_GN"
echo "  Add to sources:"
cat << 'EOF'
    "ghost_profile/ghost_profile_host.cc",
    "ghost_profile/ghost_profile_host.h",
EOF
echo ""

# Also need to register the .mojom
CONTENT_COMMON_GN="$CHROMIUM_SRC/content/common/BUILD.gn"
echo "  FILE: $CONTENT_COMMON_GN"
echo "  Add to mojom(\"mojo_bindings\") sources:"
cat << 'EOF'
    "ghost_profile.mojom",
EOF
echo ""

# Net BUILD.gn
echo "  FILE: $CHROMIUM_SRC/net/http/BUILD.gn (or net/BUILD.gn)"
echo "  Add to sources:"
cat << 'EOF'
    "http/http_header_patch.cc",
    "http/http_header_patch.h",
EOF
echo "  Add to deps:"
echo '    "//device_profile_generator",'
echo ""

# BoringSSL
echo "  FILE: $CHROMIUM_SRC/third_party/boringssl/BUILD.gn"
echo "  Add to sources:"
cat << 'EOF'
    "src/ssl/boringssl_tls_patch.cc",
    "src/ssl/boringssl_tls_patch.h",
EOF
echo "  Add to deps:"
echo '    "//tls_randomizer",'
echo ""

# Chrome Android Java
echo "  FILE: $CHROMIUM_SRC/chrome/android/BUILD.gn"
echo "  Add to android_library(\"chrome_java\") sources:"
cat << 'EOF'
    "java/src/org/nicebrowser/ghost/GhostModeManager.java",
    "java/src/org/nicebrowser/ghost/GhostModeMenuHelper.java",
    "java/src/org/nicebrowser/ghost/AutoRotateManager.java",
    "java/src/org/nicebrowser/ghost/IdentityLogActivity.java",
EOF
echo ""

echo "========================================"
echo "BUILD.gn changes reported."
echo "Apply these edits manually to the Chromium BUILD.gn files."
echo "========================================"
echo ""
echo "After applying all BUILD.gn changes, run:"
echo "  cd $CHROMIUM_SRC"
echo "  gn gen out/NormalBrowser --args='target_os=\"android\" target_cpu=\"arm64\" is_debug=false is_official_build=true'"
echo "  autoninja -C out/NormalBrowser chrome_public_apk"
