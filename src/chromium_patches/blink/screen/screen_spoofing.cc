// Copyright 2024 Normal Browser Authors. All rights reserved.
// screen_spoofing.cc — Patches for screen.* properties and window dimensions.
//
// Modifications for:
//   third_party/blink/renderer/core/frame/screen.cc
//   third_party/blink/renderer/core/frame/local_dom_window.cc
//
// Reports profile screen dimensions instead of real display.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

namespace blink {

// ====================================================================
// Screen properties
// File: third_party/blink/renderer/core/frame/screen.cc
// ====================================================================

// ORIGINAL: return GetFrame()->Host()->GetScreenInfo().rect.width();
int Screen::width() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().screen_width;
  }
  return GetFrame()->Host()->GetScreenInfo().rect.width();
}

// ORIGINAL: return GetFrame()->Host()->GetScreenInfo().rect.height();
int Screen::height() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().screen_height;
  }
  return GetFrame()->Host()->GetScreenInfo().rect.height();
}

int Screen::availWidth() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().screen_width;
  }
  return GetFrame()->Host()->GetScreenInfo().available_rect.width();
}

int Screen::availHeight() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    // Subtract realistic status bar (24dp) + navigation bar (48dp).
    int status_bar_px = static_cast<int>(24.0f * p.density);
    int nav_bar_px = static_cast<int>(48.0f * p.density);
    return p.screen_height - status_bar_px - nav_bar_px;
  }
  return GetFrame()->Host()->GetScreenInfo().available_rect.height();
}

int Screen::colorDepth() const {
  return 24;  // Always 24-bit on Android.
}

int Screen::pixelDepth() const {
  return 24;  // Same as colorDepth on Android.
}

// ====================================================================
// window.devicePixelRatio
// File: third_party/blink/renderer/core/frame/local_dom_window.cc
// ====================================================================
double LocalDOMWindow::devicePixelRatio() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return static_cast<double>(client->GetProfile().density);
  }
  return GetFrame()->DevicePixelRatio();
}

// ====================================================================
// window.outerWidth / window.outerHeight
// FPJS checks these match screen dimensions for mobile fullscreen.
// ====================================================================
int LocalDOMWindow::outerWidth() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().screen_width;
  }
  return GetFrame()->Host()->GetScreenInfo().rect.width();
}

int LocalDOMWindow::outerHeight() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().screen_height;
  }
  return GetFrame()->Host()->GetScreenInfo().rect.height();
}

// ====================================================================
// window.innerWidth / window.innerHeight
// Should be screen width × density adjusted (viewport size).
// ====================================================================
int LocalDOMWindow::innerWidth() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    // innerWidth in CSS pixels = screen_width / density
    return static_cast<int>(p.screen_width / p.density);
  }
  // Fallback to original
  return GetFrame()->View()->LayoutViewport()->VisibleContentRect().width();
}

int LocalDOMWindow::innerHeight() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    const auto& p = client->GetProfile();
    // innerHeight in CSS pixels, minus Chrome UI (URL bar ~56dp)
    int chrome_ui_dp = 56;
    int total_dp = static_cast<int>(p.screen_height / p.density);
    return total_dp - chrome_ui_dp;
  }
  return GetFrame()->View()->LayoutViewport()->VisibleContentRect().height();
}

// ====================================================================
// screen.orientation
// File: third_party/blink/renderer/modules/screen_orientation/
//       screen_orientation.cc
// Always portrait-primary for mobile.
// ====================================================================
String ScreenOrientation::type() const {
  return "portrait-primary";
}

unsigned short ScreenOrientation::angle() const {
  return 0;
}

}  // namespace blink
