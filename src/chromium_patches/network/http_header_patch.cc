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
// CRITICAL FIX (2026-03-02):
//   The previous implementation only patched GetUserAgent() / BuildSpoofedUserAgent().
//   However, Chrome 110+ uses User-Agent Reduction which overrides the UA string
//   to a generic "Mozilla/5.0 (Linux; Android 10; K) ...Chrome/X.0.0.0..." format.
//   Additionally, FPJS reads the real device model via Sec-CH-UA-Model high-entropy
//   Client Hints, which are populated from blink::UserAgentMetadata — a SEPARATE
//   data structure that our previous patches did NOT intercept.
//
//   RESULT: Real device model "22081212G" (Xiaomi 12T Pro) leaked through Client
//   Hints, completely bypassing our User-Agent spoofing.
//
//   FIX: We now ALSO patch GetUserAgentMetadata() to override the UserAgentMetadata
//   struct, AND disable User-Agent Reduction so our full UA string is sent.
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
// NEW PATCH: UserAgentMetadata Override
// File: components/embedder_support/user_agent_utils.cc
//       → GetUserAgentMetadata()
//
// CRITICAL FIX: Chrome populates a blink::UserAgentMetadata struct that
// is used for ALL Client Hints headers AND for the reduced User-Agent.
// Previously we only patched the UA string, but Chrome's UA Reduction
// feature (active since Chrome 110) ignores the UA string and uses
// UserAgentMetadata to construct a reduced UA like:
//   "Mozilla/5.0 (Linux; Android 10; K) ...Chrome/130.0.0.0..."
//
// The real device model "22081212G" leaked through this struct via
// Sec-CH-UA-Model when FPJS requested high-entropy Client Hints.
//
// We MUST override GetUserAgentMetadata() to return profile values.
//
// INTEGRATION: In components/embedder_support/user_agent_utils.cc,
// replace the body of GetUserAgentMetadata() with a call to this:
// ====================================================================

// Returns a fully populated UserAgentMetadata matching the spoofed profile.
// The caller must copy these values into blink::UserAgentMetadata.
//
// APPLY IN: components/embedder_support/user_agent_utils.cc
//   blink::UserAgentMetadata GetUserAgentMetadata(...) {
//     auto spoofed = normal_browser::http::GetSpoofedUserAgentMetadata();
//     if (spoofed.has_value) return spoofed.metadata;
//     ... original code ...
//   }
struct SpoofedUserAgentMetadata {
  bool has_value = false;

  // Brand version list (Sec-CH-UA and Sec-CH-UA-Full-Version-List)
  struct BrandVersion {
    std::string brand;
    std::string major_version;
    std::string full_version;
  };
  std::vector<BrandVersion> brand_version_list;

  // Sec-CH-UA-Full-Version
  std::string full_version;

  // Sec-CH-UA-Platform
  std::string platform;

  // Sec-CH-UA-Platform-Version
  std::string platform_version;

  // Sec-CH-UA-Arch
  std::string architecture;

  // Sec-CH-UA-Model
  std::string model;

  // Sec-CH-UA-Mobile
  bool mobile = true;

  // Sec-CH-UA-Bitness
  std::string bitness;

  // Sec-CH-UA-WoW64
  bool wow64 = false;
};

SpoofedUserAgentMetadata GetSpoofedUserAgentMetadata() {
  SpoofedUserAgentMetadata meta;

  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    meta.has_value = false;
    return meta;
  }
  meta.has_value = true;

  const auto& p = store->GetProfile();
  std::string major = std::to_string(p.chrome_major_version);

  // Brand list: must include Chromium, Google Chrome, and a GREASE brand.
  // Order and GREASE brand name vary by Chrome version — use the profile's
  // pre-computed ch_ua or build from version info.
  meta.brand_version_list = {
      {"Chromium", major, p.chrome_version},
      {"Google Chrome", major, p.chrome_version},
      {"Not-A.Brand", "99", "99.0.0.0"},
  };

  meta.full_version = p.chrome_version;
  meta.platform = "Android";
  meta.platform_version = std::to_string(p.android_version) + ".0.0";
  meta.architecture = "arm";
  meta.model = p.model;  // ← THIS IS THE KEY FIX: spoofed model, not real
  meta.mobile = true;
  meta.bitness = "64";
  meta.wow64 = false;

  return meta;
}

// ====================================================================
// NEW PATCH: Disable User-Agent Reduction
// File: components/embedder_support/user_agent_utils.cc
//       → ShouldSendReducedUserAgent()  (or similar)
// File: content/common/user_agent.cc
//       → GetReducedUserAgent()
//
// Chrome 110+ sends a "reduced" User-Agent that replaces the real
// model with "K" and the real Android version with "10":
//   "Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 ..."
//
// This defeats our UA spoofing because it strips our fake model/version.
// We need to either:
//   a) Disable UA Reduction entirely (send full UA with spoofed values), OR
//   b) Override the reduced UA to use spoofed values
//
// Option (b) is better because real Chrome sends reduced UAs, and sending
// a full UA is itself a fingerprinting signal (means modified browser).
//
// APPLY IN: content/common/user_agent.cc → GetReducedUserAgent() or
//           components/embedder_support/user_agent_utils.cc
// ====================================================================
std::string GetSpoofedReducedUserAgent() {
  auto* store = DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return "Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Chrome/" +
           std::string(kChromeVersionMajor) + ".0.0.0 Mobile Safari/537.36";
  }

  const auto& p = store->GetProfile();
  // Build a reduced UA that matches Chrome's format but:
  // - Uses "Android 10; K" (standard reduced format, matches real Chrome)
  // - Uses the profile's Chrome MAJOR version only (X.0.0.0)
  // The real model is hidden behind "K" — this is correct Chrome behavior.
  // The actual model is delivered via Sec-CH-UA-Model (which we spoof above).
  char buf[512];
  snprintf(buf, sizeof(buf),
           "Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Chrome/%d.0.0.0 Mobile Safari/537.36",
           p.chrome_major_version);
  return std::string(buf);
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
