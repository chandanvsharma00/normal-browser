// Copyright 2024 Normal Browser Authors. All rights reserved.
// http_header_patch.h — HTTP header spoofing function declarations.

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_NETWORK_HTTP_HEADER_PATCH_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_NETWORK_HTTP_HEADER_PATCH_H_

#include <string>

namespace normal_browser {
namespace http {

// Chrome version constants (update at build time)
extern const char kChromeVersionMajor[];
extern const char kChromeVersionFull[];
extern const char kChromiumVersion[];

// User-Agent
std::string BuildSpoofedUserAgent();

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
