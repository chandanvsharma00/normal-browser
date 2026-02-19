// Copyright 2024 Normal Browser Authors. All rights reserved.
// http_header_patch.cc — User-Agent, Client Hints, and other HTTP header
//                        modifications at the network stack level.
//
// FILES TO MODIFY:
//   content/common/user_agent.cc
//   components/embedder_support/user_agent_utils.cc
//   content/browser/renderer_host/navigator.cc
//   net/http/http_network_session.cc
//   services/network/public/cpp/header_util.cc
//   components/embedder_support/user_agent_utils.cc (Client Hints)
//
// All HTTP headers are set here at the network layer so they are
// consistent across ALL requests (XHR, fetch, navigation, subresource).

#include "device_profile_generator/profile_store.h"

#include <cstdio>
#include <string>

namespace normal_browser {
namespace http {

// ====================================================================
// Chrome version management
// This MUST match the actual Chromium source version we built from.
// Updated at build time by the CI/CD pipeline.
//
// CRITICAL FOR FPJS PRO:
// V8 error messages (e.g., "Cannot read properties of null") change
// between Chrome versions. If this constant disagrees with the actual
// V8 binary, FPJS can detect the mismatch by probing error.message.
// ENSURE this matches your checkout: chrome/VERSION → MAJOR.MINOR.BUILD.PATCH
// ====================================================================
const char kChromeVersionMajor[] = "130";
const char kChromeVersionFull[] = "130.0.6723.58";
const char kChromiumVersion[] = "130.0.6723.58";

// Build-time verification: If CHROME_VERSION_MAJOR is defined by the build
// system, verify it matches our constant. This catches drift between the
// actual Chromium source checkout and our hardcoded version.
#ifdef CHROME_VERSION_MAJOR
static_assert(CHROME_VERSION_MAJOR == 130,
    "CRITICAL: kChromeVersionMajor disagrees with built Chrome version! "
    "Update kChromeVersionMajor/kChromeVersionFull to match chrome/VERSION. "
    "Mismatched versions leak fingerprints via V8 error messages.");
#endif

// ====================================================================
// PATCH 1: User-Agent Header
// File: components/embedder_support/user_agent_utils.cc
//        → GetUserAgent()
// File: content/common/user_agent.cc
//        → BuildUserAgentFromProduct()
//
// Format: Mozilla/5.0 (Linux; Android {ver}; {model}) AppleWebKit/537.36
//         (KHTML, like Gecko) Chrome/{version} Mobile Safari/537.36
// ====================================================================

std::string BuildSpoofedUserAgent() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return "Mozilla/5.0 (Linux; Android 14; SM-A546B) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Chrome/" +
           std::string(kChromeVersionFull) + " Mobile Safari/537.36";
  }

  const auto& p = store->GetProfile();
  // Use the pre-built UA string from profile generation
  return p.user_agent;
}

// ====================================================================
// PATCH 2: Sec-CH-UA Header (User-Agent Client Hints)
// File: components/embedder_support/user_agent_utils.cc
//
// Low-entropy hints (sent with every request):
//   Sec-CH-UA: "Chromium";v="130", "Google Chrome";v="130", "Not?A_Brand";v="99"
//   Sec-CH-UA-Mobile: ?1
//   Sec-CH-UA-Platform: "Android"
//
// High-entropy hints (sent only when Accept-CH requests them):
//   Sec-CH-UA-Model: "SM-S928B"
//   Sec-CH-UA-Platform-Version: "14.0.0"
//   Sec-CH-UA-Full-Version-List: "Chromium";v="130.0.6723.58", ...
//   Sec-CH-UA-Arch: "arm"
//   Sec-CH-UA-Bitness: "64"
//   Sec-CH-UA-WoW64: ?0
// ====================================================================

// Returns the Sec-CH-UA header value
std::string GetSecChUa() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return "\"Chromium\";v=\"" + std::string(kChromeVersionMajor) + "\", "
           "\"Google Chrome\";v=\"" + std::string(kChromeVersionMajor) + "\", "
           "\"Not-A.Brand\";v=\"99\"";
  }
  return store->GetProfile().ch_ua;
}

// Returns Sec-CH-UA-Mobile
std::string GetSecChUaMobile() {
  return "?1";  // Always mobile
}

// Returns Sec-CH-UA-Platform
std::string GetSecChUaPlatform() {
  return "\"Android\"";
}

// Returns Sec-CH-UA-Model (high-entropy, per profile)
std::string GetSecChUaModel() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "\"SM-A546B\"";
  return "\"" + store->GetProfile().model + "\"";
}

// Returns Sec-CH-UA-Platform-Version (high-entropy)
std::string GetSecChUaPlatformVersion() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "\"14.0.0\"";
  return "\"" + std::to_string(store->GetProfile().android_version) + ".0.0\"";
}

// Returns Sec-CH-UA-Full-Version-List (high-entropy)
std::string GetSecChUaFullVersionList() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return "\"Chromium\";v=\"" + std::string(kChromeVersionFull) + "\", "
           "\"Google Chrome\";v=\"" + std::string(kChromeVersionFull) + "\", "
           "\"Not-A.Brand\";v=\"99.0.0.0\"";
  }
  return store->GetProfile().ch_ua_full_version_list;
}

// Returns Sec-CH-UA-Arch
std::string GetSecChUaArch() {
  return "\"arm\"";
}

// Returns Sec-CH-UA-Bitness
std::string GetSecChUaBitness() {
  return "\"64\"";
}

// Returns Sec-CH-UA-WoW64
std::string GetSecChUaWow64() {
  return "?0";
}

// ====================================================================
// PATCH 3: Accept-Language Header
// File: net/http/http_util.cc or net/http/http_request_headers.cc
//
// Must match navigator.language / navigator.languages
// ====================================================================
std::string GetAcceptLanguage() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "en-IN,en;q=0.9,hi;q=0.8";

  const auto& p = store->GetProfile();
  // Build Accept-Language from the profile's language list
  // Format: primary,secondary;q=0.9,tertiary;q=0.8,...
  std::string result;
  for (size_t i = 0; i < p.languages.size(); ++i) {
    if (i > 0) result += ",";
    result += p.languages[i];
    if (i > 0) {
      // Decreasing quality values
      double q = 0.9 - (i - 1) * 0.1;
      if (q < 0.1) q = 0.1;
      char qbuf[16];
      snprintf(qbuf, sizeof(qbuf), ";q=%.1f", q);
      result += qbuf;
    }
  }
  return result;
}

// ====================================================================
// PATCH 4: Sec-CH-UA-Prefers-Color-Scheme
// File: services/network/ or components/embedder_support/
//
// Must match CSS prefers-color-scheme media query result.
// ====================================================================
std::string GetSecChPrefersColorScheme() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "dark";
  return store->GetProfile().prefers_dark_mode ? "dark" : "light";
}

// ====================================================================
// PATCH 5: Sec-CH-Viewport-Width / Sec-CH-DPR
// File: services/network/
//
// These Client Hints reveal screen properties. Must match profile.
// ====================================================================
std::string GetSecChDpr() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "2.75";

  char buf[16];
  snprintf(buf, sizeof(buf), "%.2f", store->GetProfile().density);
  return std::string(buf);
}

std::string GetSecChViewportWidth() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return "412";

  const auto& p = store->GetProfile();
  int css_width = static_cast<int>(p.screen_width / p.density);
  return std::to_string(css_width);
}

// ====================================================================
// PATCH 6: DNT Header (Do Not Track)
// File: net/http/http_request_headers.cc
//
// Don't send DNT header (default state = not set).
// Sending "DNT: 1" is uncommon and makes the browser MORE identifiable.
// ====================================================================
bool ShouldSendDNTHeader() {
  return false;  // Don't send DNT header
}

// ====================================================================
// PATCH 7: Upgrade-Insecure-Requests
// File: net/http/http_request_headers.cc
//
// Always send "Upgrade-Insecure-Requests: 1" on navigation requests.
// This is standard Chrome behavior.
// ====================================================================
bool ShouldSendUpgradeInsecureRequests() {
  return true;
}

// ====================================================================
// PATCH 8: Save-Data Header
// File: services/network/
//
// Don't send "Save-Data: on" — reveals data saver setting.
// Most users don't have Data Saver enabled.
// ====================================================================
bool ShouldSendSaveData() {
  return false;
}

// ====================================================================
// Full header set for a typical navigation request:
//
// GET / HTTP/2
// Host: example.com
// User-Agent: Mozilla/5.0 (Linux; Android 14; SM-S928B) AppleWebKit/537.36...
// Accept: text/html,application/xhtml+xml,...
// Accept-Language: en-IN,en;q=0.9,hi;q=0.8
// Accept-Encoding: gzip, deflate, br
// Sec-CH-UA: "Chromium";v="130", "Google Chrome";v="130", "Not?A_Brand";v="99"
// Sec-CH-UA-Mobile: ?1
// Sec-CH-UA-Platform: "Android"
// Sec-Fetch-Dest: document
// Sec-Fetch-Mode: navigate
// Sec-Fetch-Site: none
// Sec-Fetch-User: ?1
// Upgrade-Insecure-Requests: 1
//
// (High-entropy hints only sent after Accept-CH response):
// Sec-CH-UA-Model: "SM-S928B"
// Sec-CH-UA-Platform-Version: "14.0.0"
// Sec-CH-UA-Full-Version-List: "Chromium";v="130.0.6723.58",...
// ====================================================================

}  // namespace http
}  // namespace normal_browser
