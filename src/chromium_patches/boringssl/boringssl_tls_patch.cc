// Copyright 2024 Normal Browser Authors. All rights reserved.
// boringssl_tls_patch.cc — BoringSSL TLS ClientHello randomization.
//
// FILES TO MODIFY:
//   third_party/boringssl/src/ssl/handshake_client.cc
//   third_party/boringssl/src/ssl/extensions.cc
//   third_party/boringssl/src/ssl/ssl_lib.cc
//   net/socket/ssl_client_socket_impl.cc
//
// ARCHITECTURE:
//   DeviceProfile stores a TLSProfile with seed-based fields
//   (cipher_shuffle_seed, extension_shuffle_seed, etc.).
//   At runtime, we call GenerateTLSConfig() from tls_randomizer.h to
//   produce a fully-expanded TLSClientHelloConfig with actual cipher
//   suite vectors, extension orderings, etc.
//
//   The generated config is cached per-session and used by all the
//   PATCH functions below to provide deterministic TLS randomization.

#include "device_profile_generator/profile_store.h"
#include "tls_randomizer/tls_randomizer.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace normal_browser {
namespace tls {

// ====================================================================
// Cached TLS config — generated once per session from profile seeds.
// ====================================================================
namespace {

TLSClientHelloConfig g_cached_config;
bool g_config_initialized = false;

const TLSClientHelloConfig& GetCachedConfig() {
  if (!g_config_initialized) {
    auto* store = normal_browser::DeviceProfileStore::Get();
    if (store && store->HasProfile()) {
      // Use the full TLSProfile parameters for expanded config.
      const auto& tls = store->GetProfile().tls_profile;
      g_cached_config = GenerateTLSConfigFromProfile(
          tls.cipher_shuffle_seed,
          tls.cipher_drop_count,
          tls.num_grease_extensions,
          tls.send_session_ticket,
          tls.padding_target);
      g_config_initialized = true;
    }
  }
  return g_cached_config;
}

// Default Chrome cipher suite order (fallback when no profile).
std::vector<uint16_t> DefaultCipherList() {
  return {
      0x1301,  // TLS_AES_128_GCM_SHA256
      0x1302,  // TLS_AES_256_GCM_SHA384
      0x1303,  // TLS_CHACHA20_POLY1305_SHA256
      0xC02B,  // TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
      0xC02F,  // TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
      0xC02C,  // TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
      0xC030,  // TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
      0xCCA9,  // TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305
      0xCCA8,  // TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305
      0xC013,  // TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
      0xC014,  // TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
      0x009C,  // TLS_RSA_WITH_AES_128_GCM_SHA256
      0x009D,  // TLS_RSA_WITH_AES_256_GCM_SHA384
      0x002F,  // TLS_RSA_WITH_AES_128_CBC_SHA
      0x0035,  // TLS_RSA_WITH_AES_256_CBC_SHA
  };
}

}  // namespace

// ====================================================================
// PATCH 1: Cipher Suite Randomization
// File: third_party/boringssl/src/ssl/handshake_client.cc
// Called from ssl_add_client_cipher_suites().
// ====================================================================
std::vector<uint16_t> BuildRandomizedCipherList() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return DefaultCipherList();
  }
  const auto& config = GetCachedConfig();
  return config.cipher_suites;
}

// ====================================================================
// PATCH 2: Extension Reordering
// File: third_party/boringssl/src/ssl/extensions.cc
// CONSTRAINT: SNI must be first, PSK must be last (RFC 8446 §4.2.11).
// ====================================================================
std::vector<uint16_t> BuildRandomizedExtensionOrder() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {
        0x0000, 0x0017, 0xFF01, 0x000A, 0x000B, 0x0023, 0x0010,
        0x0005, 0x000D, 0x0012, 0x002B, 0x002D, 0x0033, 0x001B,
        0x0015, 0x0029,
    };
  }
  const auto& config = GetCachedConfig();
  return config.extensions;
}

// ====================================================================
// PATCH 3: Supported Groups (Elliptic Curves)
// ====================================================================
std::vector<uint16_t> BuildRandomizedSupportedGroups() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x001D, 0x0017, 0x0018};  // X25519, P-256, P-384
  }
  const auto& config = GetCachedConfig();
  return config.supported_groups;
}

// ====================================================================
// PATCH 4: Signature Algorithms
// ====================================================================
std::vector<uint16_t> BuildRandomizedSignatureAlgorithms() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x0403, 0x0804, 0x0401, 0x0503, 0x0805, 0x0501, 0x0806, 0x0601};
  }
  const auto& config = GetCachedConfig();
  return config.signature_algorithms;
}

// ====================================================================
// PATCH 5: ALPN Protocol Order
// ====================================================================
std::vector<std::string> BuildRandomizedALPN() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {"h2", "http/1.1"};
  }
  const auto& config = GetCachedConfig();
  return config.alpn_protocols;
}

// ====================================================================
// PATCH 6: GREASE Values
// ====================================================================
static const uint16_t kGreaseValues[] = {
    0x0A0A, 0x1A1A, 0x2A2A, 0x3A3A, 0x4A4A,
    0x5A5A, 0x6A6A, 0x7A7A, 0x8A8A, 0x9A9A,
    0xAAAA, 0xBABA, 0xCACA, 0xDADA, 0xEAEA, 0xFAFA,
};

std::vector<uint16_t> GetSessionGreaseValues() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x0A0A, 0x4A4A};
  }
  const auto& config = GetCachedConfig();
  return config.grease_values;
}

// ====================================================================
// PATCH 7: EC Point Formats
// ====================================================================
std::vector<uint8_t> BuildECPointFormats() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x00};
  }
  const auto& config = GetCachedConfig();
  return config.ec_point_formats;
}

// ====================================================================
// PATCH 8: PSK Key Exchange Modes
// ====================================================================
std::vector<uint8_t> BuildPSKKeyExchangeModes() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x01};
  }
  const auto& config = GetCachedConfig();
  return config.psk_key_exchange_modes;
}

// ====================================================================
// PATCH 9: Supported Versions
// ====================================================================
std::vector<uint16_t> BuildSupportedVersions() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x0304, 0x0303};
  }
  const auto& config = GetCachedConfig();
  return config.supported_versions;
}

// ====================================================================
// PATCH 10: compress_certificate Extension
// ====================================================================
std::vector<uint16_t> BuildCompressCertificateAlgorithms() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) {
    return {0x0002};
  }
  const auto& config = GetCachedConfig();
  return config.compress_certificate_algos;
}

// ====================================================================
// INTEGRATION: net/socket/ssl_client_socket_impl.cc
// Called from SSLClientSocketImpl::Init() after SSL_CTX creation.
// ====================================================================
void ApplyTLSRandomization(void* ssl_ctx_ptr) {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return;

  // Force config generation/caching on first use.
  (void)GetCachedConfig();

  // The actual cipher list and extension ordering are applied via
  // the individual PATCH functions above, which BoringSSL calls
  // during ClientHello construction. This function ensures the
  // cached config is ready.
}

// ====================================================================
// Reset cached config (called on identity rotation).
// ====================================================================
void ResetTLSConfig() {
  g_config_initialized = false;
  g_cached_config = TLSClientHelloConfig();
}

// ====================================================================
// Debug: Log TLS fingerprint (JA3/JA4) of current session.
// ====================================================================
void LogTLSFingerprint() {
  auto* store = normal_browser::DeviceProfileStore::Get();
  if (!store || !store->HasProfile()) return;

  const auto& config = GetCachedConfig();
  // Compute JA3/JA4 for debugging/verification.
  // std::string ja3 = ComputeJA3Hash(config);
  // std::string ja4 = ComputeJA4(config);
  // LOG(INFO) << "Normal Browser TLS JA3: " << ja3;
  // LOG(INFO) << "Normal Browser TLS JA4: " << ja4;
  (void)config;  // Suppress unused warning when logging is disabled.
}

}  // namespace tls
}  // namespace normal_browser
