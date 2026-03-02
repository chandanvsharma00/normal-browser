// Copyright 2024 Normal Browser Authors. All rights reserved.
// http_header_patch.h — HTTP header spoofing function declarations.

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_NETWORK_HTTP_HEADER_PATCH_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_NETWORK_HTTP_HEADER_PATCH_H_

#include <string>
#include <vector>

namespace normal_browser {
namespace http {

// Chrome version constants (update at build time)
extern const char kChromeVersionMajor[];
extern const char kChromeVersionFull[];
extern const char kChromiumVersion[];

// User-Agent (full format — used when UA Reduction is disabled)
std::string BuildSpoofedUserAgent();

// User-Agent (reduced format — Chrome 110+ standard "Android 10; K" format)
std::string GetSpoofedReducedUserAgent();

// UserAgentMetadata override — CRITICAL for Client Hints
// Returns a struct with all fields needed for blink::UserAgentMetadata.
// This prevents real device model/version from leaking via Sec-CH-UA-Model.
struct SpoofedUserAgentMetadata {
  bool has_value = false;
  struct BrandVersion {
    std::string brand;
    std::string major_version;
    std::string full_version;
  };
  std::vector<BrandVersion> brand_version_list;
  std::string full_version;
  std::string platform;
  std::string platform_version;
  std::string architecture;
  std::string model;
  bool mobile = true;
  std::string bitness;
  bool wow64 = false;
};
SpoofedUserAgentMetadata GetSpoofedUserAgentMetadata();

// Client Hints (low-entropy — sent with every request)
std::string GetSecChUa();
std::string GetSecChUaMobile();
std::string GetSecChUaPlatform();

// Client Hints (high-entropy — sent only after Accept-CH)
std::string GetSecChUaModel();
std::string GetSecChUaPlatformVersion();
std::string GetSecChUaFullVersionList();
std::string GetSecChUaArch();
std::string GetSecChUaBitness();
std::string GetSecChUaWow64();

// Other headers
std::string GetAcceptLanguage();
std::string GetSecChPrefersColorScheme();
std::string GetSecChDpr();
std::string GetSecChViewportWidth();

// Header inclusion decisions
bool ShouldSendDNTHeader();
bool ShouldSendUpgradeInsecureRequests();
bool ShouldSendSaveData();

}  // namespace http
}  // namespace normal_browser

#endif  // NORMAL_BROWSER_CHROMIUM_PATCHES_NETWORK_HTTP_HEADER_PATCH_H_
