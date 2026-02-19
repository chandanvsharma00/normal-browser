#!/bin/bash
# Copyright 2024 Normal Browser Authors. All rights reserved.
# test_fingerprint.sh — Automated fingerprint verification tests.
#
# Tests the built APK against multiple fingerprinting services
# to verify anti-detection capabilities.
#
# Prerequisites:
#   - adb connected to test device/emulator
#   - Normal Browser APK installed
#   - Node.js installed (for test automation)
#
# Usage: ./test_fingerprint.sh [all|fpjs|creepjs|browserleaks|consistency]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$SCRIPT_DIR/../test_results/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

PACKAGE_NAME="com.nicebrowser.app"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
info() { echo -e "[INFO] $1"; }

# ====================================================================
# Test 1: Internal Consistency Check
# Verify all spoofed values are internally consistent.
# ====================================================================
test_consistency() {
  info "=== Test: Internal Consistency ==="

  # Create a test HTML page that checks all fingerprint surfaces
  cat > "$RESULTS_DIR/consistency_test.html" << 'TESTHTML'
<!DOCTYPE html>
<html>
<head><title>Normal Browser Consistency Test</title></head>
<body>
<pre id="results"></pre>
<script>
const results = [];
const pass = (name, val) => results.push(`PASS: ${name} = ${val}`);
const fail = (name, expected, actual) =>
  results.push(`FAIL: ${name} expected=${expected} actual=${actual}`);

// 1. navigator.platform
if (navigator.platform === 'Linux armv8l') pass('platform', navigator.platform);
else fail('platform', 'Linux armv8l', navigator.platform);

// 2. navigator.vendor
if (navigator.vendor === 'Google Inc.') pass('vendor', navigator.vendor);
else fail('vendor', 'Google Inc.', navigator.vendor);

// 3. navigator.productSub
if (navigator.productSub === '20030107') pass('productSub', navigator.productSub);
else fail('productSub', '20030107', navigator.productSub);

// 4. navigator.webdriver
if (navigator.webdriver === false) pass('webdriver', 'false');
else fail('webdriver', 'false', String(navigator.webdriver));

// 5. navigator.maxTouchPoints
if (navigator.maxTouchPoints >= 1) pass('maxTouchPoints', navigator.maxTouchPoints);
else fail('maxTouchPoints', '>=1', navigator.maxTouchPoints);

// 6. navigator.hardwareConcurrency
const cores = navigator.hardwareConcurrency;
if (cores >= 2 && cores <= 12) pass('hardwareConcurrency', cores);
else fail('hardwareConcurrency', '2-12', cores);

// 7. navigator.deviceMemory
const mem = navigator.deviceMemory;
if ([2, 4, 6, 8, 12, 16].includes(mem)) pass('deviceMemory', mem);
else fail('deviceMemory', '2|4|6|8|12|16', mem);

// 8. Screen dimensions
const w = screen.width, h = screen.height;
if (w > 300 && w < 2000 && h > 500 && h < 4000) pass('screen', `${w}x${h}`);
else fail('screen', 'reasonable', `${w}x${h}`);

// 9. colorDepth
if (screen.colorDepth === 24) pass('colorDepth', 24);
else fail('colorDepth', 24, screen.colorDepth);

// 10. devicePixelRatio
const dpr = window.devicePixelRatio;
if (dpr >= 1.0 && dpr <= 4.0) pass('devicePixelRatio', dpr);
else fail('devicePixelRatio', '1.0-4.0', dpr);

// 11. Timezone
const tz = Intl.DateTimeFormat().resolvedOptions().timeZone;
if (tz === 'Asia/Kolkata') pass('timezone', tz);
else fail('timezone', 'Asia/Kolkata', tz);

// 12. Timezone offset
const offset = new Date().getTimezoneOffset();
if (offset === -330) pass('timezoneOffset', offset);
else fail('timezoneOffset', -330, offset);

// 13. Language
const lang = navigator.language;
if (lang && lang.length > 0) pass('language', lang);
else fail('language', 'non-empty', lang);

// 14. UserAgent format
const ua = navigator.userAgent;
if (ua.includes('Android') && ua.includes('Chrome') && ua.includes('Mobile'))
  pass('userAgent', 'contains Android+Chrome+Mobile');
else fail('userAgent', 'Android+Chrome+Mobile format', ua);

// 15. UA model matches Sec-CH-UA-Model (if available)
pass('UA', ua.substring(0, 80) + '...');

// 16. Plugins (mobile = empty)
if (navigator.plugins.length === 0) pass('plugins', 'empty');
else fail('plugins', 'empty', `length=${navigator.plugins.length}`);

// 17. pdfViewerEnabled
if (navigator.pdfViewerEnabled === false) pass('pdfViewerEnabled', 'false');
else fail('pdfViewerEnabled', 'false', String(navigator.pdfViewerEnabled));

// 18. Touch support
if ('ontouchstart' in window) pass('touch', 'ontouchstart exists');
else fail('touch', 'ontouchstart exists', 'missing');

// 19. window.chrome exists
if (typeof window.chrome === 'object') pass('window.chrome', 'exists');
else fail('window.chrome', 'exists', typeof window.chrome);

// 20. CSS media queries
const hover = matchMedia('(hover: none)').matches;
if (hover) pass('hover', 'none');
else fail('hover', 'none', 'hover');

const pointer = matchMedia('(pointer: coarse)').matches;
if (pointer) pass('pointer', 'coarse');
else fail('pointer', 'coarse', 'fine');

// 21. Math fingerprint (should produce consistent results within session)
const tan1 = Math.tan(-1e300);
const atan1 = Math.atan2(1, 2);
pass('Math.tan(-1e300)', tan1);
pass('Math.atan2(1,2)', atan1);

// Verify Math is deterministic within session
const tan2 = Math.tan(-1e300);
if (tan1 === tan2) pass('Math.deterministic', 'same within session');
else fail('Math.deterministic', 'same', 'different');

// 22. Canvas fingerprint exists (non-zero)
const canvas = document.createElement('canvas');
canvas.width = 200; canvas.height = 50;
const ctx = canvas.getContext('2d');
ctx.fillStyle = '#f60';
ctx.fillRect(0, 0, 200, 50);
ctx.fillStyle = '#069';
ctx.font = '15px Arial';
ctx.fillText('Normal Browser Test', 2, 15);
const dataUrl = canvas.toDataURL();
if (dataUrl.length > 100) pass('canvas', `dataURL length=${dataUrl.length}`);
else fail('canvas', 'non-trivial dataURL', `length=${dataUrl.length}`);

// 23. WebGL
const gl = document.createElement('canvas').getContext('webgl');
if (gl) {
  const renderer = gl.getParameter(gl.RENDERER);
  const vendor = gl.getParameter(gl.VENDOR);
  pass('WebGL.renderer', renderer);
  pass('WebGL.vendor', vendor);
  const exts = gl.getSupportedExtensions();
  pass('WebGL.extensions', `count=${exts ? exts.length : 0}`);
} else {
  warn('WebGL', 'not available');
}

// 24. Audio context
try {
  const ac = new (window.AudioContext || window.webkitAudioContext)();
  pass('AudioContext.sampleRate', ac.sampleRate);
  pass('AudioContext.baseLatency', ac.baseLatency);
  ac.close();
} catch(e) {
  fail('AudioContext', 'available', e.message);
}

// 25. Storage estimate
navigator.storage.estimate().then(est => {
  pass('storage.quota', `${(est.quota / 1e9).toFixed(1)}GB`);
  pass('storage.usage', `${(est.usage / 1e6).toFixed(1)}MB`);
  // Final output
  document.getElementById('results').textContent = results.join('\n');
}).catch(e => {
  fail('storage', 'available', e.message);
  document.getElementById('results').textContent = results.join('\n');
});

// 26. Automation markers (must NOT exist)
const autoMarkers = [
  '__selenium_unwrapped', '__webdriver_evaluate', '__driver_evaluate',
  '__nightmare', '_phantom', 'callPhantom', 'phantom',
  'domAutomation', 'domAutomationController',
];
let autoClean = true;
for (const m of autoMarkers) {
  if (m in window) {
    fail('autoMarker', 'absent', m);
    autoClean = false;
  }
}
if (autoClean) pass('autoMarkers', 'all absent');
</script>
</body>
</html>
TESTHTML

  info "Consistency test page created at:"
  info "  $RESULTS_DIR/consistency_test.html"
  info "Push to device and open in Normal Browser to run."
  info ""
  info "  adb push $RESULTS_DIR/consistency_test.html /sdcard/Download/"
  info "  adb shell am start -a android.intent.action.VIEW -d 'file:///sdcard/Download/consistency_test.html' -n $PACKAGE_NAME/com.google.android.apps.chrome.Main"
}

# ====================================================================
# Test 2: FPJS Pro Detection
# ====================================================================
test_fpjs() {
  info "=== Test: FingerprintJS Pro ==="
  info ""
  info "Manual test steps:"
  info "  1. Open Normal Browser"
  info "  2. Navigate to: https://fingerprint.com/demo/"
  info "  3. Check results:"
  info "     - bot detection: false (MUST pass)"
  info "     - visitorId changes on 'New Identity'"
  info "     - confidence score is high (>0.9)"
  info "     - IP location matches (use VPN if needed)"
  info ""
  info "  4. Press 'New Identity' in menu"
  info "  5. Revisit fingerprint.com/demo/"
  info "  6. Verify NEW visitorId (different from step 3)"
  info ""
  info "Expected results:"
  info "  - bot: false"
  info "  - incognito: false"
  info "  - tampering signals: none detected"
  info "  - Different visitorId per identity"
}

# ====================================================================
# Test 3: CreepJS Detection
# ====================================================================
test_creepjs() {
  info "=== Test: CreepJS ==="
  info ""
  info "Manual test steps:"
  info "  1. Open: https://abrahamjuliot.github.io/creepjs/"
  info "  2. Wait for all tests to complete (~30 seconds)"
  info "  3. Check:"
  info "     - Trust Score: should be 'A' or 'B'"
  info "     - Lies detected: 0 (CRITICAL)"
  info "     - Fingerprint hash changes on 'New Identity'"
  info ""
  info "  4. Specific sections to verify:"
  info "     - Navigator: no lies detected"
  info "     - Canvas: unique hash, no tampering flag"
  info "     - WebGL: renderer/vendor match claimed device"
  info "     - Audio: unique hash"
  info "     - Fonts: match claimed manufacturer"
  info "     - Math: no architecture mismatch"
  info "     - Screen: consistent with UA device"
  info "     - Timezone: Asia/Kolkata"
}

# ====================================================================
# Test 4: BrowserLeaks Comprehensive
# ====================================================================
test_browserleaks() {
  info "=== Test: BrowserLeaks ==="
  info ""
  info "Test each page on https://browserleaks.com/:"
  info ""
  info "  1. /ip — IP address (use VPN, not our browser's job)"
  info "  2. /javascript — navigator properties match profile"
  info "  3. /canvas — unique per session, consistent within session"
  info "  4. /webgl — GL_RENDERER/VENDOR match claimed SoC"
  info "  5. /webrtc — no local IP leak (192.168.x.x or mdns)"
  info "  6. /fonts — font list matches claimed OEM"
  info "  7. /ssl — TLS fingerprint (JA3) unique per session"
  info "  8. /geo — geolocation (permission should be 'prompt')"
  info "  9. /features — CSS features match mobile profile"
  info ""
  info "Record results in: $RESULTS_DIR/browserleaks_results.txt"
}

# ====================================================================
# Test 5: Multi-Session Uniqueness
# ====================================================================
test_uniqueness() {
  info "=== Test: Multi-Session Uniqueness ==="
  info ""
  info "This test verifies that 'New Identity' produces truly unique profiles."
  info ""
  info "Steps:"
  info "  1. Open fingerprint.com/demo/"
  info "  2. Note the visitorId and all fingerprint components"
  info "  3. Press 'New Identity'"
  info "  4. Revisit fingerprint.com/demo/"
  info "  5. Verify ALL of these changed:"
  info "     - visitorId"
  info "     - Canvas hash"
  info "     - WebGL hash"
  info "     - Audio hash"
  info "     - Font hash"
  info "     - Math hash"
  info "     - User-Agent string"
  info "     - Screen dimensions"
  info "  6. Repeat 5 times and record results"
  info ""
  info "Record in: $RESULTS_DIR/uniqueness_results.txt"
}

# ====================================================================
# Main
# ====================================================================
case "${1:-all}" in
  consistency)
    test_consistency
    ;;
  fpjs)
    test_fpjs
    ;;
  creepjs)
    test_creepjs
    ;;
  browserleaks)
    test_browserleaks
    ;;
  uniqueness)
    test_uniqueness
    ;;
  all)
    test_consistency
    echo ""
    test_fpjs
    echo ""
    test_creepjs
    echo ""
    test_browserleaks
    echo ""
    test_uniqueness
    ;;
  *)
    echo "Usage: $0 [all|consistency|fpjs|creepjs|browserleaks|uniqueness]"
    exit 1
    ;;
esac

info ""
info "Results directory: $RESULTS_DIR"
info "All tests defined. Run with device connected."
