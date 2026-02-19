#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# chromium_hook_patches.sh — Apply hook-point edits to existing Chromium files.
#
# This script modifies existing Chromium source files to call Normal Browser
# spoofing code. It must be run AFTER apply_patches.sh has copied our new
# files into the Chromium tree.
#
# Usage:
#   cd ~/chromium/src
#   bash /path/to/normal-browser/patches/chromium_hook_patches.sh
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

HOOK_COUNT=0
SKIP_COUNT=0

# ============================================================
# Helper: Insert a line after a matching pattern (if not already present)
# ============================================================
insert_after() {
    local file="$CHROMIUM_ROOT/$1"
    local pattern="$2"
    local new_line="$3"

    if [[ ! -f "$file" ]]; then
        echo "  WARNING: File not found: $file (skipping)"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    # Check if the new line is already present (idempotent)
    if grep -qF "$new_line" "$file" 2>/dev/null; then
        echo "  SKIP (already applied): $new_line"
        return 0
    fi

    # Check that the pattern exists in the file
    if ! grep -qF "$pattern" "$file" 2>/dev/null; then
        echo "  WARNING: Pattern not found in $file: $pattern"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    # Insert the new line after the first match of the pattern
    sed -i "0,/$(sed 's/[\/&]/\\&/g' <<< "$pattern")/s/$(sed 's/[\/&]/\\&/g' <<< "$pattern")/&\n$(sed 's/[\/&]/\\&/g' <<< "$new_line")/" "$file"
    HOOK_COUNT=$((HOOK_COUNT + 1))
    echo "  APPLIED: $new_line  →  $file"
    return 0
}

# ============================================================
# Helper: Insert a line before a matching pattern (if not already present)
# ============================================================
insert_before() {
    local file="$CHROMIUM_ROOT/$1"
    local pattern="$2"
    local new_line="$3"

    if [[ ! -f "$file" ]]; then
        echo "  WARNING: File not found: $file (skipping)"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    if grep -qF "$new_line" "$file" 2>/dev/null; then
        echo "  SKIP (already applied): $new_line"
        return 0
    fi

    if ! grep -qF "$pattern" "$file" 2>/dev/null; then
        echo "  WARNING: Pattern not found in $file: $pattern"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    sed -i "0,/$(sed 's/[\/&]/\\&/g' <<< "$pattern")/s/$(sed 's/[\/&]/\\&/g' <<< "$pattern")/$(sed 's/[\/&]/\\&/g' <<< "$new_line")\n&/" "$file"
    HOOK_COUNT=$((HOOK_COUNT + 1))
    echo "  APPLIED: $new_line  →  $file"
    return 0
}

# ============================================================
# Helper: Add an include at the top of a file (after the last existing #include)
# ============================================================
add_include() {
    local file="$CHROMIUM_ROOT/$1"
    local include_line="$2"

    if [[ ! -f "$file" ]]; then
        echo "  WARNING: File not found: $file (skipping)"
        SKIP_COUNT=$((SKIP_COUNT + 1))
        return 1
    fi

    if grep -qF "$include_line" "$file" 2>/dev/null; then
        echo "  SKIP (already applied): $include_line in $(basename "$file")"
        return 0
    fi

    # Find the last #include line number and insert after it
    local last_include_line
    last_include_line=$(grep -n '^#include' "$file" | tail -1 | cut -d: -f1)

    if [[ -z "$last_include_line" ]]; then
        # No includes found, insert at line 1
        sed -i "1i\\$include_line" "$file"
    else
        sed -i "${last_include_line}a\\$include_line" "$file"
    fi

    HOOK_COUNT=$((HOOK_COUNT + 1))
    echo "  APPLIED: $include_line  →  $(basename "$file")"
    return 0
}

echo "============================================="
echo " Normal Browser — Hook-Point Patch Script"
echo "============================================="
echo ""
echo "Target: $CHROMIUM_ROOT"
echo ""

# ============================================================
# HOOK 1: Navigator — third_party/blink/renderer/core/frame/navigator.cc
# Add include for ghost_profile_client.h
# ============================================================
echo ">>> Hook 1/10: Navigator (navigator.cc)"
add_include \
    "third_party/blink/renderer/core/frame/navigator.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# ============================================================
# HOOK 2: Screen — third_party/blink/renderer/core/frame/screen.cc
# Add include for ghost_profile_client.h
# ============================================================
echo ""
echo ">>> Hook 2/10: Screen (screen.cc)"
add_include \
    "third_party/blink/renderer/core/frame/screen.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# ============================================================
# HOOK 3: Canvas — canvas_rendering_context_2d.cc
# Add include for canvas_spoofing.cc functions
# ============================================================
echo ""
echo ">>> Hook 3/10: Canvas (canvas_rendering_context_2d.cc)"
add_include \
    "third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# ============================================================
# HOOK 4: WebGL — webgl_rendering_context_base.cc
# Add include for WebGL spoofing hooks
# ============================================================
echo ""
echo ">>> Hook 4/10: WebGL (webgl_rendering_context_base.cc)"
add_include \
    "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# ============================================================
# HOOK 5: Audio — audio_context.cc
# Add include for audio spoofing
# ============================================================
echo ""
echo ">>> Hook 5/10: Audio (audio_context.cc, offline_audio_context.cc)"
add_include \
    "third_party/blink/renderer/modules/webaudio/audio_context.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'
add_include \
    "third_party/blink/renderer/modules/webaudio/offline_audio_context.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# ============================================================
# HOOK 6: V8 Math — builtins-math-gen.cc
# Add include for V8 math override function pointers
# ============================================================
echo ""
echo ">>> Hook 6/10: V8 Math (builtins-math-gen.cc)"
add_include \
    "v8/src/builtins/builtins-math-gen.cc" \
    '#include "v8/src/builtins/v8_math_patch.h"'

# ============================================================
# HOOK 7: TLS — handshake_client.cc
# Add include for BoringSSL TLS randomization
# ============================================================
echo ""
echo ">>> Hook 7/10: TLS/BoringSSL (handshake_client.cc)"
add_include \
    "third_party/boringssl/src/ssl/handshake_client.cc" \
    '#include "third_party/boringssl/src/ssl/boringssl_tls_patch.h"'

# ============================================================
# HOOK 8: HTTP Headers — http_request_headers.cc
# Add include for header spoofing
# ============================================================
echo ""
echo ">>> Hook 8/10: HTTP Headers (http_request_headers.cc)"
add_include \
    "net/http/http_request_headers.cc" \
    '#include "net/http/http_header_patch.h"'

# ============================================================
# HOOK 9: Chrome Startup — chrome_browser_main.cc
# Add include and call InitializeGhostMode() in PreMainMessageLoopRun()
# ============================================================
echo ""
echo ">>> Hook 9/10: Chrome Startup (chrome_browser_main.cc)"
add_include \
    "chrome/browser/chrome_browser_main.cc" \
    '#include "chrome/browser/ghost/browser_startup_wiring.h"'

# Insert InitializeGhostMode() call in PreMainMessageLoopRun()
insert_after \
    "chrome/browser/chrome_browser_main.cc" \
    "ChromeBrowserMainParts::PreMainMessageLoopRunImpl" \
    "  normal_browser::InitializeGhostMode();  // Normal Browser: init ghost mode"

# Insert ShutdownGhostMode() call in PostMainMessageLoopRun()
insert_after \
    "chrome/browser/chrome_browser_main.cc" \
    "ChromeBrowserMainParts::PostMainMessageLoopRun" \
    "  normal_browser::ShutdownGhostMode();  // Normal Browser: shutdown ghost mode"

# ============================================================
# HOOK 10: Content RenderFrame — render_frame_impl.cc
# Initialize GhostProfileClient in renderer
# ============================================================
echo ""
echo ">>> Hook 10/10: RenderFrame (render_frame_impl.cc)"
add_include \
    "content/renderer/render_frame_impl.cc" \
    '#include "third_party/blink/renderer/core/ghost_profile_client.h"'

# Insert GhostProfileClient initialization in RenderFrameImpl::Initialize()
insert_after \
    "content/renderer/render_frame_impl.cc" \
    "RenderFrameImpl::Initialize" \
    "  normal_browser::GhostProfileClient::Get()->Initialize();  // Normal Browser"

echo ""
echo "============================================="
echo " Hook-point patches complete!"
echo ""
echo " Applied: $HOOK_COUNT hooks"
echo " Skipped: $SKIP_COUNT (not found or already applied)"
echo ""
echo " IMPORTANT: These hooks add #include directives and function calls"
echo " to existing Chromium source files. The actual spoofing logic is"
echo " in the files copied by apply_patches.sh."
echo "============================================="
