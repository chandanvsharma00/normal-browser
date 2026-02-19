// Copyright 2024 Normal Browser Authors. All rights reserved.
// boringssl_tls_patch.h — Header for BoringSSL TLS ClientHello randomization.
//
// Provides functions that BoringSSL's handshake_client.cc and extensions.cc
// call to get randomized cipher/extension/group ordering per session.

#ifndef NORMAL_BROWSER_CHROMIUM_PATCHES_BORINGSSL_TLS_PATCH_H_
#define NORMAL_BROWSER_CHROMIUM_PATCHES_BORINGSSL_TLS_PATCH_H_

#include <cstdint>
#include <string>
#include <vector>

namespace normal_browser {
namespace tls {

// PATCH 1: Cipher list for ClientHello (already shuffled + GREASE injected)
std::vector<uint16_t> BuildRandomizedCipherList();

// PATCH 2: Extension type IDs in randomized order (SNI first, PSK last)
std::vector<uint16_t> BuildRandomizedExtensionOrder();

// PATCH 3: Supported groups / elliptic curves in randomized order
std::vector<uint16_t> BuildRandomizedSupportedGroups();

// PATCH 4: Signature algorithms in randomized order
std::vector<uint16_t> BuildRandomizedSignatureAlgorithms();

// PATCH 5: ALPN protocols (h2, http/1.1, optional h3)
std::vector<std::string> BuildRandomizedALPN();

// PATCH 6: Session GREASE values (2-4 unique RFC 8701 codepoints)
std::vector<uint16_t> GetSessionGreaseValues();

// PATCH 7: EC point formats
std::vector<uint8_t> BuildECPointFormats();

// PATCH 8: PSK key exchange modes
std::vector<uint8_t> BuildPSKKeyExchangeModes();

// PATCH 9: Supported TLS versions (1.3, 1.2, optional 1.1)
std::vector<uint16_t> BuildSupportedVersions();

// PATCH 10: Certificate compression algorithms
std::vector<uint16_t> BuildCompressCertificateAlgorithms();

// Integration: Apply randomization to SSL_CTX at socket init time
void ApplyTLSRandomization(void* ssl_ctx_ptr);

// Debug: Log the JA3/JA4 hash of current session's TLS profile
void LogTLSFingerprint();

// Reset cached config (called on identity rotation / ghost mode toggle).
void ResetTLSConfig();

}  // namespace tls
}  // namespace normal_browser

#endif  // NORMAL_BROWSER_CHROMIUM_PATCHES_BORINGSSL_TLS_PATCH_H_
