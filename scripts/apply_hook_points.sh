#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# apply_hook_points.sh — Apply inline edits to existing Chromium files.
#
# This script inserts #include directives and function call hooks into
# existing Chromium source files so our patch modules are actually invoked.
#
# Usage: ./apply_hook_points.sh /path/to/chromium/src
#
# Run AFTER apply_patches.sh and BEFORE gn gen.
# Each patch is idempotent — running twice won't duplicate edits.

set -euo pipefail

CHROMIUM_SRC="${1:?Usage: $0 /path/to/chromium/src}"

if [ ! -d "$CHROMIUM_SRC/chrome" ]; then
  echo "ERROR: $CHROMIUM_SRC does not look like a Chromium source tree."
  exit 1
fi

PASS=0
FAIL=0
SKIP=0

# Helper: insert a line after a pattern if not already present
# Usage: insert_after <file> <grep_pattern_for_anchor> <line_to_insert>
insert_after() {
  local file="$1"
  local anchor="$2"
  local newline="$3"

  if [ ! -f "$file" ]; then
    echo "  MISS: $file not found"
    ((FAIL++))
    return 1
  fi

  # Already applied?
  if grep -qF "$newline" "$file" 2>/dev/null; then
    echo "  SKIP: already present in $(basename "$file")"
    ((SKIP++))
    return 0
  fi

  # Find the anchor
  if ! grep -q "$anchor" "$file" 2>/dev/null; then
    echo "  MISS: anchor not found in $(basename "$file"): $anchor"
    ((FAIL++))
    return 1
  fi

  # Insert after the LAST match of anchor
  sed -i "0,/$anchor/{ /$anchor/a\\
$newline
}" "$file"
  echo "  OK:   inserted in $(basename "$file")"
  ((PASS++))
  return 0
}

# Helper: insert a line before a pattern if not already present
insert_before() {
  local file="$1"
  local anchor="$2"
  local newline="$3"

  if [ ! -f "$file" ]; then
    echo "  MISS: $file not found"
    ((FAIL++))
    return 1
  fi

  if grep -qF "$newline" "$file" 2>/dev/null; then
    echo "  SKIP: already present in $(basename "$file")"
    ((SKIP++))
    return 0
  fi

  if ! grep -q "$anchor" "$file" 2>/dev/null; then
    echo "  MISS: anchor not found in $(basename "$file"): $anchor"
    ((FAIL++))
    return 1
  fi

  sed -i "0,/$anchor/{
/$anchor/i\\
$newline
}" "$file"
  echo "  OK:   inserted in $(basename "$file")"
  ((PASS++))
  return 0
}

# Helper: prepend include at top of file (after last existing #include block)
add_include() {
  local file="$1"
  local include_line="$2"

  if [ ! -f "$file" ]; then
    echo "  MISS: $file not found"
    ((FAIL++))
    return 1
  fi

  if grep -qF "$include_line" "$file" 2>/dev/null; then
    echo "  SKIP: already present in $(basename "$file")"
    ((SKIP++))
    return 0
  fi

  # Find last #include line number and insert after it
  local last_include_line
  last_include_line=$(grep -n '^#include' "$file" | tail -1 | cut -d: -f1)
  if [ -z "$last_include_line" ]; then
    # No includes found, insert at line 1
    sed -i "1i\\$include_line" "$file"
  else
    sed -i "${last_include_line}a\\$include_line" "$file"
  fi
  echo "  OK:   added include in $(basename "$file")"
  ((PASS++))
  return 0
}

echo "=== Normal Browser Hook Point Patcher ==="
echo "Chromium src: $CHROMIUM_SRC"
echo ""

# ====================================================================
# HOOK 1: Chrome Browser Startup → InitializeGhostMode()
# ====================================================================
echo "[1/12] Chrome startup wiring..."
CHROME_MAIN="$CHROMIUM_SRC/chrome/browser/chrome_browser_main_android.cc"
add_include "$CHROME_MAIN" '#include "chrome/browser/ghost/browser_startup_wiring.h"'
insert_after "$CHROME_MAIN" \
  "PreMainMessageLoopRun" \
  "  normal_browser::InitializeGhostMode();  // Normal Browser: init ghost profiles"

# ====================================================================
# HOOK 2: Content Renderer → Initialize GhostProfileClient
# ====================================================================
echo "[2/12] Renderer GhostProfileClient initialization..."
RENDER_FRAME="$CHROMIUM_SRC/content/renderer/render_frame_impl.cc"
add_include "$RENDER_FRAME" '#include "third_party/blink/renderer/core/ghost_profile_client.h"'
insert_after "$RENDER_FRAME" \
  "RenderFrameImpl::Initialize(" \
  "  normal_browser::GhostProfileClient::Get()->Initialize();  // Normal Browser"

# ====================================================================
# HOOK 3: Mojo binding for GhostProfileProvider
# ====================================================================
echo "[3/12] Mojo GhostProfileProvider binding..."
RFH_IMPL="$CHROMIUM_SRC/content/browser/renderer_host/render_frame_host_impl.cc"
add_include "$RFH_IMPL" '#include "content/browser/ghost_profile/ghost_profile_host.h"'
# Insert Mojo binding in the BindReceiver/RegisterMojoInterfaces area
insert_after "$RFH_IMPL" \
  "RegisterMojoInterfaces" \
  "  // Normal Browser: bind GhostProfileProvider for renderer\\n  registry->AddInterface(base::BindRepeating(\\&normal_browser::GhostProfileHost::Create));"

# ====================================================================
# HOOK 4: Navigator UA override
# ====================================================================
echo "[4/12] Navigator userAgent() override..."
NAVIGATOR_CC="$CHROMIUM_SRC/third_party/blink/renderer/core/frame/navigator.cc"
add_include "$NAVIGATOR_CC" '#include "third_party/blink/renderer/core/frame/navigator_spoofing.cc"'

# ====================================================================
# HOOK 5: Screen property overrides
# ====================================================================
echo "[5/12] Screen property overrides..."
SCREEN_CC="$CHROMIUM_SRC/third_party/blink/renderer/core/frame/screen.cc"
add_include "$SCREEN_CC" '#include "third_party/blink/renderer/core/frame/screen_spoofing.cc"'

# ====================================================================
# HOOK 6: Canvas fingerprint noise
# ====================================================================
echo "[6/12] Canvas toDataURL/toBlob/getImageData hooks..."
CANVAS_CTX="$CHROMIUM_SRC/third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc"
add_include "$CANVAS_CTX" '#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_spoofing.cc"'

# ====================================================================
# HOOK 7: WebGL parameter + extension spoofing
# ====================================================================
echo "[7/14] WebGL getParameter/getExtensions/getSupportedExtensions hooks..."
WEBGL_CTX="$CHROMIUM_SRC/third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc"
add_include "$WEBGL_CTX" '#include "third_party/blink/renderer/modules/webgl/webgl_spoofing.cc"'

# Hook getSupportedExtensions() to return spoofed extension list
insert_before "$WEBGL_CTX" \
  "return result;" \
  "  // Normal Browser: override WebGL extension list\\n  auto spoofed_exts = blink::webgl_spoofing::GetSpoofedWebGLExtensions();\\n  if (spoofed_exts.has_value()) return spoofed_exts.value();"

# Hook getExtension() to filter by spoofed profile
insert_after "$WEBGL_CTX" \
  "getExtension.*ScriptState" \
  "  // Normal Browser: block extensions not in spoofed profile\\n  if (!blink::webgl_spoofing::ShouldEnableWebGLExtension(name)) return nullptr;"

# ====================================================================
# HOOK 8: Audio context spoofing
# ====================================================================
echo "[8/14] AudioContext/OfflineAudioContext hooks..."
AUDIO_CTX="$CHROMIUM_SRC/third_party/blink/renderer/modules/webaudio/audio_context.cc"
add_include "$AUDIO_CTX" '#include "third_party/blink/renderer/modules/webaudio/audio_spoofing.cc"'

OFFLINE_AUDIO="$CHROMIUM_SRC/third_party/blink/renderer/modules/webaudio/offline_audio_context.cc"
if [ -f "$OFFLINE_AUDIO" ]; then
  add_include "$OFFLINE_AUDIO" '#include "third_party/blink/renderer/modules/webaudio/audio_spoofing.cc"'
fi

# ====================================================================
# HOOK 9: V8 Math builtins → FDLIBM replacements
# ====================================================================
echo "[9/14] V8 Math builtins override..."
V8_MATH="$CHROMIUM_SRC/v8/src/builtins/builtins-math-gen.cc"
add_include "$V8_MATH" '#include "v8/src/builtins/v8_math_patch.h"'
# Also patch the external references for transcendental functions
V8_EXTREF="$CHROMIUM_SRC/v8/src/codegen/external-reference.cc"
if [ -f "$V8_EXTREF" ]; then
  add_include "$V8_EXTREF" '#include "v8/src/builtins/v8_math_patch.h"'
  echo "  NOTE: You must manually replace ieee754::sin/cos/tan/etc calls"
  echo "        with v8_math_patch::PatchedMathSin/Cos/Tan/etc in"
  echo "        external-reference.cc. See v8_math_patch.h for the full table."
fi

# ====================================================================
# HOOK 10: BoringSSL TLS randomization
# ====================================================================
echo "[10/14] BoringSSL ClientHello randomization..."
HANDSHAKE="$CHROMIUM_SRC/third_party/boringssl/src/ssl/handshake_client.cc"
add_include "$HANDSHAKE" '#include "third_party/boringssl/src/ssl/boringssl_tls_patch.h"'
echo "  NOTE: You must manually call normal_browser::tls::ApplyTLSRandomization(ssl)"
echo "        before ssl_write_client_hello() in handshake_client.cc"

# ====================================================================
# HOOK 11: HTTP Header / Client Hints spoofing
# ====================================================================
echo "[11/14] HTTP header and Client Hints overrides..."
HTTP_HEADERS="$CHROMIUM_SRC/net/http/http_request_headers.cc"
if [ -f "$HTTP_HEADERS" ]; then
  add_include "$HTTP_HEADERS" '#include "net/http/http_header_patch.h"'
fi

# Also hook the NavigatorUAData for Client Hints API
UA_DATA="$CHROMIUM_SRC/third_party/blink/renderer/core/frame/navigator_ua_data.cc"
if [ -f "$UA_DATA" ]; then
  add_include "$UA_DATA" '#include "net/http/http_header_patch.h"'
  echo "  NOTE: Override NavigatorUAData::brands() and NavigatorUAData::getHighEntropyValues()"
  echo "        to use normal_browser::http::GetSpoofedUserAgentMetadata()"
fi

# ====================================================================
# HOOK 12: Timezone/Locale after ICU init
# ====================================================================
echo "[12/14] Timezone and locale override..."
ICU_UTIL="$CHROMIUM_SRC/base/i18n/icu_util.cc"
if [ -f "$ICU_UTIL" ]; then
  add_include "$ICU_UTIL" '#include "base/i18n/timezone_locale_patch.cc"'
fi

# ====================================================================
# HOOK 13: DOMRect/ClientRect noise (emoji + layout fingerprinting)
# ====================================================================
echo "[13/14] DOMRect getBoundingClientRect noise..."
ELEMENT_CC="$CHROMIUM_SRC/third_party/blink/renderer/core/dom/element.cc"
add_include "$ELEMENT_CC" '#include "third_party/blink/renderer/core/dom/domrect_spoofing.cc"'
# Hook into GetBoundingClientRectNoLifecycleUpdate — insert noise after AdjustRect
insert_after "$ELEMENT_CC" \
  "AdjustRectForScrollAndAbsoluteZoom" \
  "  // Normal Browser: Apply per-profile DOMRect noise for fingerprint variation\n  {\n    double nx = result.x(), ny = result.y();\n    double nw = result.width(), nh = result.height();\n    blink::domrect_spoofing::ApplyDOMRectNoise(nx, ny, nw, nh);\n    result.SetRect(static_cast<float>(nx), static_cast<float>(ny),\n                   static_cast<float>(nw), static_cast<float>(nh));\n  }"

RANGE_CC="$CHROMIUM_SRC/third_party/blink/renderer/core/dom/range.cc"
if [ -f "$RANGE_CC" ]; then
  add_include "$RANGE_CC" '#include "third_party/blink/renderer/core/dom/domrect_spoofing.cc"'
fi

# ====================================================================
# HOOK 14: Font family blocking (prevents FPJS font detection)
# ====================================================================
echo "[14/14] Font family blocking for fingerprint prevention..."
# 14a: Add ShouldBlockFontFamily to font_spoofing.h (already done via apply_patches.sh)
# 14b: Hook into FontCache::GetFontPlatformData in font_cache.cc
FONT_CACHE_CC="$CHROMIUM_SRC/third_party/blink/renderer/platform/fonts/font_cache.cc"
if [ -f "$FONT_CACHE_CC" ]; then
  add_include "$FONT_CACHE_CC" '#include "third_party/blink/renderer/platform/fonts/font_spoofing.h"'
  # Insert ShouldBlockFontFamily check after TRACE_EVENT in GetFontPlatformData
  insert_after "$FONT_CACHE_CC" \
    'TRACE_EVENT0.*FontCache::GetFontPlatformData' \
    "  // Normal Browser: Block fingerprintable font families\n  if (creation_params.CreationType() == kCreateFontByFamily) {\n    String family_str = creation_params.Family().GetString();\n    if (!family_str.empty() \&\& blink::ShouldBlockFontFamily(family_str.Utf8())) {\n      return nullptr;\n    }\n  }"
fi

FONT_FALLBACK="$CHROMIUM_SRC/third_party/blink/renderer/platform/fonts/font_fallback_list.cc"
if [ -f "$FONT_FALLBACK" ]; then
  add_include "$FONT_FALLBACK" '#include "third_party/blink/renderer/platform/fonts/font_spoofing.h"'
fi

echo ""
echo "========================================"
echo "Hook point patching complete!"
echo "  Passed: $PASS"
echo "  Skipped (already applied): $SKIP"
echo "  Failed (needs manual edit): $FAIL"
echo "========================================"
echo ""

if [ "$FAIL" -gt 0 ]; then
  echo "WARNING: $FAIL hook(s) could not be auto-applied."
  echo "These require manual edits — see BUILD_INSTRUCTIONS.md Step 3.B"
  echo ""
fi

echo "Hooks that ALWAYS need manual adjustment:"
echo "  1. V8 external-reference.cc — replace ieee754 function pointers"
echo "     with v8_math_patch:: equivalents (see v8_math_patch.h)"
echo "  2. BoringSSL handshake_client.cc — call ApplyTLSRandomization()"
echo "     before ssl_write_client_hello()"
echo "  3. Navigator/Screen/Canvas — verify the #include-as-inline approach"
echo "     works with the specific Chrome 130 code layout, or switch to"
echo "     explicit function call hooks if needed."
echo ""
echo "Next: Run scripts/apply_build_gn_patches.sh $CHROMIUM_SRC"
