#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# build_gn_patches.sh — Add Normal Browser source files to Chromium BUILD.gn files.
#
# This script modifies existing BUILD.gn files in the Chromium tree to include
# our new source files. It must be run AFTER apply_patches.sh has copied files.
#
# Usage:
#   cd ~/chromium/src
#   bash /path/to/normal-browser/patches/build_gn_patches.sh
#
# Targets: Chrome 130 (tags/130.0.6723.58)

set -euo pipefail

CHROMIUM_ROOT="$(pwd)"

# ============================================================
# Verify we're in a Chromium tree
# ============================================================
if [[ ! -f "$CHROMIUM_ROOT/BUILD.gn" ]] || [[ ! -d "$CHROMIUM_ROOT/third_party/blink" ]]; then
    echo "ERROR: This doesn't look like a Chromium source tree."
    echo "       Run this script from ~/chromium/src/"
    exit 1
fi

PATCH_COUNT=0
SKIP_COUNT=0

# ============================================================
# Helper: Add source entries to a BUILD.gn file's sources list.
# Inserts entries after a known anchor pattern (e.g. existing source file).
# Idempotent — skips if entries already exist.
# ============================================================
add_sources_to_build_gn() {
    local gn_file="$CHROMIUM_ROOT/$1"
    local anchor_pattern="$2"
    shift 2
    local entries=("$@")

    if [[ ! -f "$gn_file" ]]; then
        echo "  WARNING: BUILD.gn not found: $gn_file (skipping)"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    local any_added=false
    for entry in "${entries[@]}"; do
        if grep -qF "$entry" "$gn_file" 2>/dev/null; then
            echo "  SKIP (already present): $entry"
            continue
        fi

        # Verify anchor exists
        if ! grep -qF "$anchor_pattern" "$gn_file" 2>/dev/null; then
            echo "  WARNING: Anchor pattern not found in $gn_file: $anchor_pattern"
            SKIP_COUNT=$((SKIP_COUNT + 1))
            return 1
        fi

        # Insert after the anchor pattern
        local escaped_anchor
        escaped_anchor=$(sed 's/[\/&.*[\^$]/\\&/g' <<< "$anchor_pattern")
        local escaped_entry
        escaped_entry=$(sed 's/[\/&]/\\&/g' <<< "$entry")
        sed -i "0,/${escaped_anchor}/s/${escaped_anchor}/&\n    ${escaped_entry}/" "$gn_file"
        PATCH_COUNT=$((PATCH_COUNT + 1))
        any_added=true
        echo "  ADDED: $entry"
    done

    if $any_added; then
        echo "  → Updated: $gn_file"
    fi
    return 0
}

# ============================================================
# Helper: Add dependency entries to a BUILD.gn file.
# ============================================================
add_deps_to_build_gn() {
    local gn_file="$CHROMIUM_ROOT/$1"
    local deps_anchor="$2"
    shift 2
    local deps=("$@")

    if [[ ! -f "$gn_file" ]]; then
        echo "  WARNING: BUILD.gn not found: $gn_file (skipping)"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    for dep in "${deps[@]}"; do
        if grep -qF "$dep" "$gn_file" 2>/dev/null; then
            echo "  SKIP (already present): $dep"
            continue
        fi

        if ! grep -qF "$deps_anchor" "$gn_file" 2>/dev/null; then
            echo "  WARNING: Deps anchor not found in $gn_file: $deps_anchor"
            SKIP_COUNT=$((SKIP_COUNT + 1))
            return 1
        fi

        local escaped_anchor
        escaped_anchor=$(sed 's/[\/&.*[\^$]/\\&/g' <<< "$deps_anchor")
        local escaped_dep
        escaped_dep=$(sed 's/[\/&]/\\&/g' <<< "$dep")
        sed -i "0,/${escaped_anchor}/s/${escaped_anchor}/&\n    ${escaped_dep}/" "$gn_file"
        PATCH_COUNT=$((PATCH_COUNT + 1))
        echo "  ADDED dep: $dep"
    done
    return 0
}

echo "============================================="
echo " Normal Browser — BUILD.gn Patch Script"
echo "============================================="
echo ""
echo "Target: $CHROMIUM_ROOT"
echo ""

# ============================================================
# 1. Blink Core BUILD.gn
#    Add ghost_profile_client, navigator/screen spoofing, timing,
#    CSS/WebGPU/Speech, and anti-bot files.
# ============================================================
echo ">>> 1/6: Blink Core BUILD.gn"
echo "    (third_party/blink/renderer/core/BUILD.gn)"

# Look for an existing source file in the core sources list to use as anchor
add_sources_to_build_gn \
    "third_party/blink/renderer/core/BUILD.gn" \
    "sources = [" \
    '"frame/navigator_spoofing.cc",' \
    '"frame/screen_spoofing.cc",' \
    '"timing/performance_timing_spoofing.cc",' \
    '"css/css_webgpu_speech_spoofing.cc",' \
    '"ghost_profile_client.cc",' \
    '"ghost_profile_client.h",' \
    '"anti_bot/anti_bot_markers.cc",' \
    '"anti_bot/anti_bot_markers.h",'

add_deps_to_build_gn \
    "third_party/blink/renderer/core/BUILD.gn" \
    "deps = [" \
    '"//components/device_profile_generator",' \
    '"//components/software_math",'

echo ""

# ============================================================
# 2. Blink Modules BUILD.gn
#    Add canvas, WebGL, audio, sensor, storage, media, WebRTC spoofing.
# ============================================================
echo ">>> 2/6: Blink Modules BUILD.gn"
echo "    (third_party/blink/renderer/modules/BUILD.gn)"

add_sources_to_build_gn \
    "third_party/blink/renderer/modules/BUILD.gn" \
    "sources = [" \
    '"canvas/canvas_spoofing.cc",' \
    '"webgl/webgl_spoofing.cc",' \
    '"webaudio/audio_spoofing.cc",' \
    '"sensor/sensor_spoofing.cc",' \
    '"storage/storage_memory_spoofing.cc",' \
    '"mediastream/media_devices_spoofing.cc",' \
    '"peerconnection/webrtc_ip_spoofing.cc",'

add_deps_to_build_gn \
    "third_party/blink/renderer/modules/BUILD.gn" \
    "deps = [" \
    '"//components/device_profile_generator",' \
    '"//components/software_math",'

echo ""

# ============================================================
# 3. V8 BUILD.gn
#    Add math patch files.
# ============================================================
echo ">>> 3/6: V8 BUILD.gn"
echo "    (v8/BUILD.gn)"

add_sources_to_build_gn \
    "v8/BUILD.gn" \
    "sources = [" \
    '"src/builtins/v8_math_patch.cc",' \
    '"src/builtins/v8_math_patch.h",'

echo ""

# ============================================================
# 4. Chrome Browser BUILD.gn
#    Add browser startup wiring and Android JNI bridge.
# ============================================================
echo ">>> 4/6: Chrome Browser BUILD.gn"
echo "    (chrome/browser/BUILD.gn)"

add_sources_to_build_gn \
    "chrome/browser/BUILD.gn" \
    "sources = [" \
    '"ghost/browser_startup_wiring.cc",' \
    '"ghost/browser_startup_wiring.h",' \
    '"ghost/android/ghost_mode_jni_bridge.cc",'

add_deps_to_build_gn \
    "chrome/browser/BUILD.gn" \
    "deps = [" \
    '"//components/device_profile_generator",' \
    '"//components/software_math",' \
    '"//components/tls_randomizer",'

echo ""

# ============================================================
# 5. Content Browser BUILD.gn
#    Add Mojo host (ghost_profile_host).
# ============================================================
echo ">>> 5/6: Content Browser BUILD.gn"
echo "    (content/browser/BUILD.gn)"

add_sources_to_build_gn \
    "content/browser/BUILD.gn" \
    "sources = [" \
    '"ghost_profile/ghost_profile_host.cc",' \
    '"ghost_profile/ghost_profile_host.h",'

add_deps_to_build_gn \
    "content/browser/BUILD.gn" \
    "deps = [" \
    '"//components/device_profile_generator",'

echo ""

# ============================================================
# 6. Net HTTP BUILD.gn
#    Add HTTP header patch.
# ============================================================
echo ">>> 6/6: Net HTTP BUILD.gn"
echo "    (net/http/BUILD.gn)"

add_sources_to_build_gn \
    "net/http/BUILD.gn" \
    "sources = [" \
    '"http_header_patch.cc",' \
    '"http_header_patch.h",'

add_deps_to_build_gn \
    "net/http/BUILD.gn" \
    "deps = [" \
    '"//components/device_profile_generator",'

echo ""
echo "============================================="
echo " BUILD.gn patches complete!"
echo ""
echo " Patched:  $PATCH_COUNT entries"
echo " Skipped:  $SKIP_COUNT (not found or already applied)"
echo ""
echo " NEXT: Run 'gn gen out/Release --args=...' to regenerate build files."
echo "============================================="
