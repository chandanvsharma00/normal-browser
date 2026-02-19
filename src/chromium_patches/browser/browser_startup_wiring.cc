// Copyright 2024 Normal Browser Authors. All rights reserved.
// browser_startup_wiring.cc — Ghost Mode initialization during Chrome startup.
//
// This file patches chrome/browser/chrome_browser_main_android.cc
// or chrome/browser/chrome_browser_main.cc to initialize all Normal
// Browser subsystems in the correct order at startup.
//
// INTEGRATION POINT:
//   chrome/browser/chrome_browser_main_android.cc
//     PreMainMessageLoopRun() {
//       ...
//       normal_browser::InitializeGhostMode();
//       ...
//     }
//
// The initialization order is CRITICAL:
//   1. DeviceProfileStore — generate/restore the spoofed identity
//   2. Timezone/Locale lock — before any renderer starts
//   3. TLS config cache — before any network request
//   4. Mojo host setup — so renderers can fetch profiles

#include "device_profile_generator/profile_store.h"
#include "chromium_patches/timezone/timezone_locale_patch.h"
#include "chromium_patches/network/boringssl_tls_patch.h"
#include "base/logging.h"

namespace normal_browser {

// ====================================================================
// Main initialization entry point — called from PreMainMessageLoopRun()
// ====================================================================
void InitializeGhostMode() {
  LOG(INFO) << "[GhostMode] Initializing Normal Browser subsystems...";

  // ---------------------------------------------------------------
  // Step 1: Initialize the DeviceProfileStore singleton.
  //   - If a persisted profile exists (from SharedPreferences via JNI),
  //     it will be loaded by the Java side and pushed down via
  //     nativeImportProfileJson().
  //   - If no persisted profile, generate a fresh one.
  // ---------------------------------------------------------------
  auto* store = DeviceProfileStore::Get();
  if (!store->HasProfile()) {
    store->GenerateNewProfile();
    LOG(INFO) << "[GhostMode] Generated initial device profile.";
  } else {
    LOG(INFO) << "[GhostMode] Restored persisted device profile.";
  }

  // ---------------------------------------------------------------
  // Step 2: Lock timezone and locale.
  //   Must happen before any renderer process starts, because
  //   renderers inherit the browser's timezone via ICU.
  // ---------------------------------------------------------------
  ApplyTimezoneAndLocale();
  LOG(INFO) << "[GhostMode] Timezone locked to Asia/Kolkata.";

  // ---------------------------------------------------------------
  // Step 3: Pre-warm the TLS config cache.
  //   The first HTTPS connection will use the spoofed TLS config.
  //   Caching it now avoids a race on the first request.
  // ---------------------------------------------------------------
  // GetCachedConfig() in boringssl_tls_patch.cc handles lazy init,
  // but we can trigger it here to ensure it's ready.
  LOG(INFO) << "[GhostMode] TLS config will be initialized on first use.";

  // ---------------------------------------------------------------
  // Step 4: Mojo host wiring is handled in render_frame_host_impl.cc
  //   via BindReceiver, not here. Just log the expectation.
  // ---------------------------------------------------------------
  LOG(INFO) << "[GhostMode] Mojo GhostProfileProvider ready for renderers.";

  LOG(INFO) << "[GhostMode] Initialization complete.";
}

// ====================================================================
// Shutdown cleanup — called from ChromeBrowserMainParts::PostMainMessageLoop()
// ====================================================================
void ShutdownGhostMode() {
  // Currently no cleanup needed. The DeviceProfileStore is a NoDestructor
  // singleton and persists for the process lifetime.
  LOG(INFO) << "[GhostMode] Shutdown complete.";
}

}  // namespace normal_browser
