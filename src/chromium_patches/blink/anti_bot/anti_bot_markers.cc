// Copyright 2024 Normal Browser Authors. All rights reserved.
// anti_bot_markers.cc — Ensure the browser passes ALL bot detection checks.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/core/frame/navigator.cc
//   third_party/blink/renderer/core/frame/local_dom_window.cc
//   third_party/blink/renderer/bindings/core/v8/
//   content/browser/permissions/permission_service_impl.cc
//
// FPJS Pro's bot detection checks these FIRST before fingerprinting.
// If ANY of these fail, the visitor is immediately flagged as bot:true.
//
// NOTE: Navigator property overrides (webdriver, vendor, productSub,
// pdfViewerEnabled, doNotTrack, plugins, mimeTypes) are implemented
// in navigator_spoofing.cc. This file handles non-navigator anti-bot
// checks (permissions reset, automation globals, process detection).

#include <cstring>
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace blink {

// ====================================================================
// 1. navigator.webdriver — MUST be false (not undefined)
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Puppeteer/Selenium set this to true. Real Chrome = false.
//    FPJS checks: navigator.webdriver === false
//    CreepJS checks: navigator.webdriver !== undefined && !navigator.webdriver
//
//    ORIGINAL Chromium: returns true when DevTools protocol is active,
//                       false otherwise.
//    PATCHED: Always return false.
// ====================================================================
// bool Navigator::webdriver() const {
//   return false;  // Always false — we are NOT automated
// }

// ====================================================================
// 2. navigator.plugins — MUST be empty PluginArray on mobile
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Desktop Chrome has 5 plugins (PDF viewer, Native Client, etc.)
//    Mobile Chrome = empty (length 0).
//    FPJS checks: navigator.plugins.length === 0 for mobile claim.
//
//    If plugins.length > 0 on mobile → contradiction detected.
//    Stock Chromium Android already returns empty. Ensure not modified.
// ====================================================================
// DOMPluginArray* Navigator::plugins() const {
//   return MakeGarbageCollected<DOMPluginArray>(); // Empty, length 0
// }

// ====================================================================
// 3. navigator.mimeTypes — MUST be empty on mobile
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Linked to plugins. Mobile = empty.
// ====================================================================
// DOMMimeTypeArray* Navigator::mimeTypes() const {
//   return MakeGarbageCollected<DOMMimeTypeArray>(); // Empty
// }

// ====================================================================
// 4. navigator.pdfViewerEnabled — false on mobile Chrome
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Desktop: true (Chrome has built-in PDF viewer)
//    Mobile:  false
// ====================================================================
// bool Navigator::pdfViewerEnabled() const {
//   return false;
// }

// ====================================================================
// 5. navigator.doNotTrack — null (default state)
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    "1" = user enabled DNT
//    null = user hasn't changed it (default, most common)
//    FPJS checks the default state to match fresh browser profile.
// ====================================================================
// String Navigator::doNotTrack() const {
//   return String();  // null — default state
// }

// ====================================================================
// 6. navigator.vendor — MUST be "Google Inc."
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Chrome/Chromium: "Google Inc."
//    Firefox: ""
//    Safari: "Apple Computer, Inc."
//    FPJS uses this to verify browser engine claim.
// ====================================================================
// String Navigator::vendor() const {
//   return "Google Inc.";
// }

// ====================================================================
// 7. navigator.productSub — MUST be "20030107"
//    File: third_party/blink/renderer/core/frame/navigator.cc
//
//    Chrome/Safari: "20030107"
//    Firefox: "20100101"
// ====================================================================
// String Navigator::productSub() const {
//   return "20030107";
// }

// ====================================================================
// 8. window.chrome object — MUST exist
//    File: third_party/blink/renderer/core/frame/local_dom_window.cc
//           OR bindings layer
//
//    FPJS checks: typeof window.chrome !== 'undefined'
//    Also checks: window.chrome.app exists
//                 window.chrome.csi() exists
//                 window.chrome.loadTimes() exists
//                 window.chrome.runtime exists
//
//    Stock Chromium already defines window.chrome.
//    DON'T strip Chrome-specific APIs from the build.
//
//    VERIFICATION: Ensure these all pass:
//      typeof window.chrome === 'object'           ✓
//      typeof window.chrome.app === 'object'       ✓
//      typeof window.chrome.csi === 'function'     ✓ (non-Android? check)
//      typeof window.chrome.loadTimes === 'function' ✓
// ====================================================================

// ====================================================================
// 9. Touch event support — MUST exist on mobile
//    File: already present in stock Chromium Android
//
//    FPJS checks:
//      'ontouchstart' in window     → true
//      typeof TouchEvent !== 'undefined'  → true
//      navigator.maxTouchPoints >= 1 → true (we return 5)
//
//    Stock Chromium Android already supports these.
//    DON'T remove touch event support.
// ====================================================================

// ====================================================================
// 10. Permission states — MUST be 'prompt' for fresh identity
//     File: content/browser/permissions/permission_service_impl.cc
//
//     FPJS checks permission states:
//       navigator.permissions.query({name:'notifications'}) → 'prompt'
//       navigator.permissions.query({name:'geolocation'})   → 'prompt'
//       navigator.permissions.query({name:'camera'})        → 'prompt'
//       navigator.permissions.query({name:'microphone'})    → 'prompt'
//       navigator.permissions.query({name:'push'})          → 'prompt'
//
//     A fresh browser should have ALL permissions in 'prompt' state.
//     'granted' or 'denied' suggests previous interaction → trackable.
//
//     SOLUTION: When Ghost Mode is active or identity is rotated,
//     clear all permission decisions:
// ====================================================================
void ResetAllPermissionsForGhostMode(
    HostContentSettingsMap* settings_map) {
  // Called when:
  //   1. "New Identity" button is pressed
  //   2. Data clearing completes
  //   3. First launch
  //
  // Clears all per-site permission decisions so that every permission
  // query returns 'prompt' — matching a fresh browser install.
  if (!settings_map) return;

  static constexpr ContentSettingsType kPermissionTypes[] = {
      ContentSettingsType::NOTIFICATIONS,
      ContentSettingsType::GEOLOCATION,
      ContentSettingsType::MEDIASTREAM_CAMERA,
      ContentSettingsType::MEDIASTREAM_MIC,
      ContentSettingsType::SENSORS,
      ContentSettingsType::MIDI_SYSEX,
      ContentSettingsType::CLIPBOARD_READ_WRITE,
      ContentSettingsType::PAYMENT_HANDLER,
      ContentSettingsType::IDLE_DETECTION,
      ContentSettingsType::BACKGROUND_SYNC,
      ContentSettingsType::USB_GUARD,
      ContentSettingsType::BLUETOOTH_GUARD,
      ContentSettingsType::NFC,
      ContentSettingsType::STORAGE_ACCESS,
      ContentSettingsType::WINDOW_MANAGEMENT,
  };

  for (ContentSettingsType type : kPermissionTypes) {
    settings_map->ClearSettingsForOneType(type);
  }
}

// ====================================================================
// 11. Error.prototype.stack format — Chrome V8 format
//     Stock Chromium uses V8, so Error stack trace format is already
//     correct: "Error\n    at Object.<anonymous> (file:1:1)"
//     DON'T modify V8 error handling.
// ====================================================================

// ====================================================================
// 12. eval.toString() — must return native code string
//     eval.toString() === 'function eval() { [native code] }'
//     DON'T monkey-patch eval or Function constructor.
//     Stock Chromium already returns this correctly.
// ====================================================================

// ====================================================================
// 13. Automation globals that MUST NOT exist
//     File: Verified by NOT including any automation frameworks.
//
//     These variables indicate bot tools. Our browser MUST NOT define:
//       window.__selenium_unwrapped
//       window.__webdriver_evaluate
//       window.__driver_evaluate
//       window.__webdriver_script_fn
//       window.__fxdriver_evaluate
//       window.__nightmare
//       window._phantom / window.callPhantom / window.phantom
//       window.domAutomation / window.domAutomationController
//       window.cdc_adoQpoasnfa76pfcZLmcfl_Array   (chromedriver)
//       window.cdc_adoQpoasnfa76pfcZLmcfl_Promise  (chromedriver)
//       navigator.__proto__.webdriver (prototype pollution)
//
//     SOLUTION: Don't bundle chromedriver, Selenium, or Puppeteer.
//     In release builds, strip ALL DevTools protocol automation hooks.
// ====================================================================

// ====================================================================
// 14. ChromeDriver detection marker removal
//     File: chrome/test/chromedriver/ — DO NOT include in build
//
//     ChromeDriver injects specific variables into the page.
//     Ensure the build does NOT include chromedriver components.
//
//     In BUILD.gn, ensure these are excluded:
//       chrome/test/chromedriver:*
//       components/autofill/content/renderer/test_*
// ====================================================================

// ====================================================================
// 15. DevTools Protocol marker — modify when in Ghost Mode
//     File: content/browser/devtools/devtools_agent_host_impl.cc
//
//     Some fingerprinters detect if DevTools is open by:
//       - Checking debug formatting speed
//       - Detecting console.log override
//       - Measuring function toString() deoptimization
//
//     In Ghost Mode, disable DevTools protocol entirely to prevent
//     any detectable side effects.
// ====================================================================
bool ShouldDisableDevToolsInGhostMode() {
  // When Ghost Mode is active, disable DevTools attachment to
  // prevent any detectable runtime artifacts.
  // This only affects Ghost Mode tabs — normal tabs keep DevTools.
  return true;
}

// ====================================================================
// 16. Screen orientation consistency
//     File: Already handled in screen_spoofing.cc
//
//     orientation.type → "portrait-primary"
//     orientation.angle → 0
//     These MUST match mobile claim.
// ====================================================================

// ====================================================================
// 17. Process detection prevention
//     File: content/browser/sandbox_linux.cc (Android)
//
//     Block read access to paths that reveal:
//       /proc/self/maps    → Frida detection
//       /proc/self/status  → TracerPid (debugger detection)
//       /system/bin/su     → Root detection
//       /system/xbin/su    → Root detection
//
//     These are only relevant if a website somehow gains native code
//     execution (e.g., via WebAssembly or native SDK). For pure web
//     JS, these paths are already inaccessible. But as defense in depth:
// ====================================================================
bool ShouldBlockProcAccess(const char* path) {
  // Returns true if the path should return ENOENT in Ghost Mode.
  if (!path) return false;

  // Block /proc/self/maps (Frida detection)
  if (strstr(path, "/proc/self/maps")) return true;

  // Block /proc/self/status (TracerPid → debugger detection)
  if (strstr(path, "/proc/self/status")) return true;

  // Block root-indicative paths
  if (strstr(path, "/system/bin/su")) return true;
  if (strstr(path, "/system/xbin/su")) return true;
  if (strstr(path, "/data/local/tmp/frida")) return true;
  if (strstr(path, "com.topjohnwu.magisk")) return true;

  return false;
}

}  // namespace blink
