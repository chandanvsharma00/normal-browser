// Copyright 2024 Normal Browser Authors. All rights reserved.
// navigator_spoofing.cc — Patches for navigator.* properties.
//
// These are C++ modifications to apply inside:
//   third_party/blink/renderer/core/frame/navigator.cc
//   third_party/blink/renderer/core/frame/navigator_base.cc
//
// Each function shows the ORIGINAL Chromium code (commented) and
// the REPLACEMENT code that reads from GhostProfileClient.

// ====================================================================
// HOW TO APPLY:  Search for each function in the Chromium source and
// replace the implementation body with the code shown below.
// Add #include "third_party/blink/renderer/core/ghost_profile_client.h"
// at the top of each modified file.
// ====================================================================

#include "third_party/blink/renderer/core/ghost_profile_client.h"

namespace blink {

// ====================================================================
// navigator.userAgent
// File: third_party/blink/renderer/core/frame/navigator.cc
// ====================================================================
// ORIGINAL:
//   String Navigator::userAgent() const {
//     return GetFrame()->Loader().UserAgent();
//   }
// REPLACEMENT:
String Navigator::userAgent() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return String::FromUTF8(client->GetProfile().user_agent);
  }
  // Fallback to original if ghost mode not ready.
  return GetFrame()->Loader().UserAgent();
}

// ====================================================================
// navigator.platform
// File: third_party/blink/renderer/core/frame/navigator_base.cc
// ====================================================================
// ORIGINAL:
//   String NavigatorBase::platform() const {
//     return NavigatorID::platform();
//   }
// REPLACEMENT:
String NavigatorBase::platform() const {
  // Always "Linux armv8l" on Android — matches real Chrome on ARM64.
  return "Linux armv8l";
}

// ====================================================================
// navigator.hardwareConcurrency
// File: third_party/blink/renderer/core/frame/navigator_concurrent_hardware.cc
// ====================================================================
// ORIGINAL:
//   unsigned int NavigatorConcurrentHardware::hardwareConcurrency() const {
//     return std::max(1u, base::SysInfo::NumberOfProcessors());
//   }
// REPLACEMENT:
unsigned int NavigatorConcurrentHardware::hardwareConcurrency() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().cpu_cores;
  }
  return 8;  // Safe default (most Android phones)
}

// ====================================================================
// navigator.deviceMemory
// File: third_party/blink/renderer/core/frame/navigator_device_memory.cc
// ====================================================================
// ORIGINAL:
//   float NavigatorDeviceMemory::deviceMemory() const {
//     return ApproximateDeviceMemory::GetApproximatedDeviceMemory();
//   }
// REPLACEMENT:
float NavigatorDeviceMemory::deviceMemory() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    // Return as GB, rounded to standard values: 2, 4, 6, 8, 12, 16.
    float gb = static_cast<float>(client->GetProfile().ram_mb) / 1024.0f;
    // Clamp to standard reported values.
    if (gb >= 14.0f) return 16.0f;
    if (gb >= 10.0f) return 12.0f;
    if (gb >= 7.0f) return 8.0f;
    if (gb >= 5.0f) return 6.0f;
    if (gb >= 3.0f) return 4.0f;
    return 2.0f;
  }
  return 8.0f;
}

// ====================================================================
// navigator.language / navigator.languages
// File: third_party/blink/renderer/core/frame/navigator_language.cc
// ====================================================================
// ORIGINAL:
//   AtomicString NavigatorLanguage::language() {
//     return AtomicString(NavigatorLanguage::UserPreferredLanguages()[0]);
//   }
// REPLACEMENT:
AtomicString NavigatorLanguage::language() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return AtomicString(String::FromUTF8(client->GetProfile().primary_language));
  }
  return AtomicString("en-IN");
}

Vector<String> NavigatorLanguage::languages() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    Vector<String> result;
    for (const auto& lang : client->GetProfile().languages) {
      result.push_back(String::FromUTF8(lang));
    }
    return result;
  }
  return {"en-IN", "en", "hi"};
}

// ====================================================================
// navigator.maxTouchPoints
// File: third_party/blink/renderer/core/frame/navigator.cc
// ====================================================================
// Always 5 for Android mobile.
int Navigator::maxTouchPoints() const {
  return 5;
}

// ====================================================================
// navigator.vendor
// File: third_party/blink/renderer/core/frame/navigator_id.cc
// ====================================================================
String NavigatorID::vendor() const {
  return "Google Inc.";
}

// ====================================================================
// navigator.productSub
// ====================================================================
String NavigatorID::productSub() const {
  return "20030107";
}

// ====================================================================
// navigator.webdriver — CRITICAL: must be false, not undefined
// File: third_party/blink/renderer/core/frame/navigator.cc
// ====================================================================
bool Navigator::webdriver() const {
  return false;  // Never true — we are NOT automated.
}

// ====================================================================
// navigator.pdfViewerEnabled — false on mobile Chrome
// ====================================================================
bool Navigator::pdfViewerEnabled() const {
  return false;
}

// ====================================================================
// navigator.doNotTrack — null (default state)
// ====================================================================
String Navigator::doNotTrack() const {
  return String();  // null — user hasn't set it
}

// ====================================================================
// navigator.connection (NetworkInformation)
// File: third_party/blink/renderer/modules/netinfo/network_information.cc
// ====================================================================
// REPLACEMENT for effectiveType():
String NetworkInformation::effectiveType() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return String::FromUTF8(client->GetProfile().effective_type);
  }
  return "4g";
}

// REPLACEMENT for downlink():
double NetworkInformation::downlink() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().network_downlink_mbps;
  }
  return 10.0;
}

// REPLACEMENT for rtt():
unsigned long NetworkInformation::rtt() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().network_rtt_ms;
  }
  return 50;
}

// ====================================================================
// navigator.getBattery()
// File: third_party/blink/renderer/modules/battery/battery_manager.cc
// ====================================================================
// Modify BatteryManager to return profile values:
double BatteryManager::level() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return static_cast<double>(client->GetProfile().battery_level);
  }
  return 0.72;
}

bool BatteryManager::charging() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().battery_charging;
  }
  return false;
}

}  // namespace blink
