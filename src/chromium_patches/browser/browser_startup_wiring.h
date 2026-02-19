// Copyright 2024 Normal Browser Authors. All rights reserved.
// browser_startup_wiring.h — Entry points for Ghost Mode initialization.
//
// Called from chrome/browser/chrome_browser_main_android.cc during
// browser startup and shutdown.

#ifndef NORMAL_BROWSER_BROWSER_STARTUP_WIRING_H_
#define NORMAL_BROWSER_BROWSER_STARTUP_WIRING_H_

namespace normal_browser {

// Initialize all Ghost Mode subsystems. Call from PreMainMessageLoopRun().
void InitializeGhostMode();

// Clean shutdown. Call from PostMainMessageLoopRun().
void ShutdownGhostMode();

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_BROWSER_STARTUP_WIRING_H_
