// Copyright 2024 Normal Browser Authors. All rights reserved.
// tls_randomizer.h — Per-session TLS ClientHello randomization for
// unique JA3/JA4 hashes that defeat Cloudflare/Akamai TLS fingerprinting.

#ifndef NORMAL_BROWSER_TLS_RANDOMIZER_TLS_RANDOMIZER_H_
#define NORMAL_BROWSER_TLS_RANDOMIZER_TLS_RANDOMIZER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace normal_browser {

// Represents a single TLS cipher suite by IANA value.
struct TLSCipherSuite {
  uint16_t value;
  std::string name;
};

// A fully specified TLS ClientHello configuration.
struct TLSClientHelloConfig {
  // Cipher suites in ClientHello order.
  std::vector<uint16_t> cipher_suites;

  // TLS extensions in offer order.
  std::vector<uint16_t> extensions;

  // Supported elliptic curves / named groups.
  std::vector<uint16_t> supported_groups;

  // Signature algorithms.
  std::vector<uint16_t> signature_algorithms;

  // EC point formats.
  std::vector<uint8_t> ec_point_formats;

  // ALPN protocols (order matters for JA3).
  std::vector<std::string> alpn_protocols;

  // PSK key exchange modes.
  std::vector<uint8_t> psk_key_exchange_modes;

  // Supported TLS versions.
  std::vector<uint16_t> supported_versions;

  // Compress certificates algorithms.
  std::vector<uint16_t> compress_certificate_algos;

  // Session seed for reproducibility.
  uint64_t session_seed;

  // GREASE values (randomly selected per-session).
  std::vector<uint16_t> grease_values;

  // Padding target bytes (512 or 1024, 0 = no padding).
  int padding_target = 0;

  // Whether to include session ticket extension. 
  bool send_session_ticket = true;
};

// Forward declare TLSProfile from device_profile.h.
struct TLSProfile;

// Generate a randomized TLS ClientHello config that:
// 1. Looks like real Chrome (same cipher suites, same extension types)
// 2. Has different ordering/GREASE values per session → unique JA3/JA4
// 3. Is valid TLS — won't cause handshake failures
TLSClientHelloConfig GenerateTLSConfig(uint64_t session_seed);

// Generate with full TLSProfile (uses cipher_drop_count, num_grease,
// padding_target, send_session_ticket from profile).
TLSClientHelloConfig GenerateTLSConfigFromProfile(
    uint64_t session_seed,
    int cipher_drop_count,
    int num_grease_extensions,
    bool send_session_ticket,
    int padding_target);

// Compute the JA3 hash of a config (for testing/verification).
std::string ComputeJA3Hash(const TLSClientHelloConfig& config);

// Compute the JA4 fingerprint string.
std::string ComputeJA4(const TLSClientHelloConfig& config);

}  // namespace normal_browser

#endif  // NORMAL_BROWSER_TLS_RANDOMIZER_TLS_RANDOMIZER_H_
