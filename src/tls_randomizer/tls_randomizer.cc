// Copyright 2024 Normal Browser Authors. All rights reserved.
// tls_randomizer.cc — TLS ClientHello randomization for JA3/JA4 uniqueness.
//
// Chrome's TLS fingerprint (JA3 hash) is nearly identical across all Chrome
// instances. Cloudflare, Akamai, and similar services use this to detect
// bots and non-standard browsers. This module randomizes the ClientHello
// while keeping it functionally identical to real Chrome.
//
// What we randomize:
// 1. GREASE values (2 random from valid set)
// 2. Extension order (shuffle non-critical extensions)
// 3. Signature algorithm order (shuffle within functional groups)
// 4. Supported group order (shuffle)
// 5. Certificate compression algorithm presence

#include "tls_randomizer/tls_randomizer.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

#include "base/strings/stringprintf.h"

namespace normal_browser {

namespace {

// =============================================================
// Seeded PRNG
// =============================================================
struct RNG {
  uint64_t state;
  explicit RNG(uint64_t s) : state(s ? s : 0x7E50FEEDULL) {}
  uint64_t Next() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
  int Range(int lo, int hi) {
    return lo + static_cast<int>(Next() % (hi - lo + 1));
  }
  bool RandBool() { return (Next() & 1) == 0; }

  // Fisher-Yates shuffle.
  template <typename T>
  void Shuffle(std::vector<T>& v) {
    for (int i = static_cast<int>(v.size()) - 1; i > 0; --i) {
      int j = static_cast<int>(Next() % (i + 1));
      std::swap(v[i], v[j]);
    }
  }
};

// =============================================================
// Valid GREASE values (RFC 8701)
// =============================================================
const uint16_t kGreaseValues[] = {
    0x0A0A, 0x1A1A, 0x2A2A, 0x3A3A, 0x4A4A,
    0x5A5A, 0x6A6A, 0x7A7A, 0x8A8A, 0x9A9A,
    0xAAAA, 0xBABA, 0xCACA, 0xDADA, 0xEAEA, 0xFAFA};

// =============================================================
// Chrome's real cipher suites (TLS 1.3 + TLS 1.2 fallback)
// =============================================================

// TLS 1.3 cipher suites (always present, order can vary):
const uint16_t kTLS13Ciphers[] = {
    0x1301,  // TLS_AES_128_GCM_SHA256
    0x1302,  // TLS_AES_256_GCM_SHA384
    0x1303,  // TLS_CHACHA20_POLY1305_SHA256
};

// TLS 1.2 cipher suites (Chrome real order, can be shuffled):
const uint16_t kTLS12Ciphers[] = {
    0xC02B,  // TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
    0xC02F,  // TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
    0xC02C,  // TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
    0xC030,  // TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
    0xCCA9,  // TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
    0xCCA8,  // TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256
};

// =============================================================
// Chrome's real TLS extensions
// =============================================================

// Extensions that Chrome always sends (order can be randomized):
const uint16_t kCoreExtensions[] = {
    0x0000,  // server_name (SNI)
    0x0017,  // extended_master_secret
    0xFF01,  // renegotiation_info
    0x000A,  // supported_groups
    0x000B,  // ec_point_formats
    0x0023,  // session_ticket
    0x0010,  // application_layer_protocol_negotiation (ALPN)
    0x0005,  // status_request (OCSP)
    0x000D,  // signature_algorithms
    0x002B,  // supported_versions
    0x002D,  // psk_key_exchange_modes
    0x0033,  // key_share
    0x001B,  // compress_certificate
    0x0012,  // signed_certificate_timestamp (SCT)
    0x0039,  // encrypted_client_hello (ECH) — newer Chrome
};

// Extensions Chrome sometimes sends (depends on version/config):
const uint16_t kOptionalExtensions[] = {
    0x001C,  // record_size_limit
    0x0015,  // padding
    0x0029,  // pre_shared_key
    0x002C,  // early_data
    0x0031,  // post_handshake_auth
    0x0011,  // application_settings (ALPS)
};

// =============================================================
// Supported groups (elliptic curves + key exchange)
// =============================================================
const uint16_t kSupportedGroups[] = {
    0x001D,  // x25519
    0x0017,  // secp256r1
    0x0018,  // secp384r1
    0x0019,  // secp521r1 (sometimes omitted)
    0x001E,  // x448 (sometimes omitted)
    0x0100,  // X25519Kyber768Draft00 (Chrome 124+, post-quantum)
};

// =============================================================
// Signature algorithms
// =============================================================
const uint16_t kSignatureAlgorithms[] = {
    0x0403,  // ecdsa_secp256r1_sha256
    0x0804,  // rsa_pss_rsae_sha256
    0x0401,  // rsa_pkcs1_sha256
    0x0503,  // ecdsa_secp384r1_sha384
    0x0805,  // rsa_pss_rsae_sha384
    0x0501,  // rsa_pkcs1_sha384
    0x0806,  // rsa_pss_rsae_sha512
    0x0601,  // rsa_pkcs1_sha512
};

uint16_t PickGrease(RNG& rng) {
  return kGreaseValues[rng.Range(0, 15)];
}

}  // namespace

TLSClientHelloConfig GenerateTLSConfig(uint64_t session_seed) {
  RNG rng(session_seed);
  TLSClientHelloConfig config;
  config.session_seed = session_seed;

  // 1. Pick 2-3 unique GREASE values for this session.
  config.grease_values.clear();
  for (int i = 0; i < 3; ++i) {
    uint16_t g;
    do {
      g = PickGrease(rng);
    } while (std::find(config.grease_values.begin(),
                       config.grease_values.end(),
                       g) != config.grease_values.end());
    config.grease_values.push_back(g);
  }

  // 2. Build cipher suite list.
  // Chrome format: GREASE, TLS1.3 ciphers, TLS1.2 ciphers
  config.cipher_suites.clear();
  config.cipher_suites.push_back(config.grease_values[0]);  // leading GREASE

  // TLS 1.3 ciphers (3 ciphers, shuffled).
  std::vector<uint16_t> tls13(std::begin(kTLS13Ciphers),
                               std::end(kTLS13Ciphers));
  rng.Shuffle(tls13);
  for (uint16_t c : tls13) config.cipher_suites.push_back(c);

  // TLS 1.2 ciphers (6 ciphers, shuffled within pairs).
  std::vector<uint16_t> tls12(std::begin(kTLS12Ciphers),
                               std::end(kTLS12Ciphers));
  // Shuffle in pairs (ECDSA+RSA variants together).
  for (size_t i = 0; i + 1 < tls12.size(); i += 2) {
    if (rng.RandBool()) std::swap(tls12[i], tls12[i + 1]);
  }
  for (uint16_t c : tls12) config.cipher_suites.push_back(c);

  // 3. Build extension list with randomized order.
  config.extensions.clear();
  config.extensions.push_back(config.grease_values[1]);  // leading GREASE

  // Core extensions (shuffled, but SNI must come first after GREASE).
  std::vector<uint16_t> core(std::begin(kCoreExtensions),
                              std::end(kCoreExtensions));
  // Keep SNI (0x0000) first, shuffle the rest.
  std::vector<uint16_t> tail(core.begin() + 1, core.end());
  rng.Shuffle(tail);
  config.extensions.push_back(core[0]);  // SNI first
  for (uint16_t e : tail) config.extensions.push_back(e);

  // Randomly include some optional extensions (probability 40% each).
  for (uint16_t opt : kOptionalExtensions) {
    if (rng.Range(1, 100) <= 40) {
      config.extensions.push_back(opt);
    }
  }

  // Trailing GREASE.
  config.extensions.push_back(config.grease_values[2]);

  // 4. Supported groups.
  config.supported_groups.clear();
  config.supported_groups.push_back(config.grease_values[0]);  // leading GREASE

  std::vector<uint16_t> groups(std::begin(kSupportedGroups),
                                std::end(kSupportedGroups));
  // x25519 usually first, then shuffle the rest.
  // Sometimes include, sometimes exclude secp521r1 / x448 / Kyber.
  std::vector<uint16_t> core_groups = {0x001D, 0x0017, 0x0018};  // always
  std::vector<uint16_t> optional_groups;
  if (rng.RandBool()) optional_groups.push_back(0x0019);  // secp521r1
  if (rng.RandBool()) optional_groups.push_back(0x001E);  // x448
  if (rng.RandBool()) optional_groups.push_back(0x0100);  // Kyber

  rng.Shuffle(optional_groups);
  for (uint16_t g : core_groups) config.supported_groups.push_back(g);
  for (uint16_t g : optional_groups) config.supported_groups.push_back(g);

  // 5. Signature algorithms (shuffled within functional equivalence).
  config.signature_algorithms.clear();
  std::vector<uint16_t> sigalgs(std::begin(kSignatureAlgorithms),
                                 std::end(kSignatureAlgorithms));
  // Shuffle in groups of 2 (ECDSA+RSA_PSS for same hash).
  for (size_t i = 0; i + 1 < sigalgs.size(); i += 2) {
    if (rng.RandBool()) std::swap(sigalgs[i], sigalgs[i + 1]);
  }
  config.signature_algorithms = sigalgs;

  // 6. EC point formats. Chrome always sends [0] (uncompressed).
  config.ec_point_formats = {0x00};

  // 7. ALPN — Chrome order.
  config.alpn_protocols = {"h2", "http/1.1"};

  // 8. PSK key exchange modes.
  config.psk_key_exchange_modes = {0x01};  // psk_dhe_ke

  // 9. Supported versions: TLS 1.3, TLS 1.2 (randomize order).
  config.supported_versions.clear();
  config.supported_versions.push_back(config.grease_values[1]);
  if (rng.RandBool()) {
    config.supported_versions.push_back(0x0304);  // TLS 1.3
    config.supported_versions.push_back(0x0303);  // TLS 1.2
  } else {
    config.supported_versions.push_back(0x0303);
    config.supported_versions.push_back(0x0304);
  }

  // 10. Compress certificate (brotli = 0x0002).
  config.compress_certificate_algos = {0x0002};  // brotli

  return config;
}

// ====================================================================
// GenerateTLSConfigFromProfile — Uses per-profile TLS parameters:
//   cipher_drop_count:     Drop 0-3 non-essential TLS 1.2 ciphers
//   num_grease_extensions: 2-4 GREASE values (not fixed at 3)
//   send_session_ticket:   Whether to include session_ticket extension
//   padding_target:        ClientHello padding target (512 or 1024)
// ====================================================================
TLSClientHelloConfig GenerateTLSConfigFromProfile(
    uint64_t session_seed,
    int cipher_drop_count,
    int num_grease_extensions,
    bool send_session_ticket,
    int padding_target) {

  RNG rng(session_seed);
  TLSClientHelloConfig config;
  config.session_seed = session_seed;

  // Clamp parameters.
  if (num_grease_extensions < 2) num_grease_extensions = 2;
  if (num_grease_extensions > 4) num_grease_extensions = 4;
  if (cipher_drop_count < 0) cipher_drop_count = 0;
  if (cipher_drop_count > 3) cipher_drop_count = 3;

  // 1. Pick GREASE values (variable count: 2-4).
  config.grease_values.clear();
  for (int i = 0; i < num_grease_extensions; ++i) {
    uint16_t g;
    do {
      g = PickGrease(rng);
    } while (std::find(config.grease_values.begin(),
                       config.grease_values.end(),
                       g) != config.grease_values.end());
    config.grease_values.push_back(g);
  }

  // 2. Build cipher suite list with optional dropping.
  config.cipher_suites.clear();
  config.cipher_suites.push_back(config.grease_values[0]);  // leading GREASE

  // TLS 1.3 ciphers (always all 3, shuffled).
  std::vector<uint16_t> tls13(std::begin(kTLS13Ciphers),
                               std::end(kTLS13Ciphers));
  rng.Shuffle(tls13);
  for (uint16_t c : tls13) config.cipher_suites.push_back(c);

  // TLS 1.2 ciphers — DROP 0-3 non-essential from the end.
  std::vector<uint16_t> tls12(std::begin(kTLS12Ciphers),
                               std::end(kTLS12Ciphers));
  // Shuffle in pairs.
  for (size_t i = 0; i + 1 < tls12.size(); i += 2) {
    if (rng.RandBool()) std::swap(tls12[i], tls12[i + 1]);
  }
  // Drop from the end (least important ciphers).
  int keep = static_cast<int>(tls12.size()) - cipher_drop_count;
  if (keep < 2) keep = 2;  // Always keep at least 2 TLS 1.2 ciphers
  for (int i = 0; i < keep; ++i) {
    config.cipher_suites.push_back(tls12[i]);
  }

  // 3. Build extension list.
  config.extensions.clear();
  config.extensions.push_back(
      config.grease_values[1 % num_grease_extensions]);  // leading GREASE

  // Core extensions (shuffled, SNI first).
  std::vector<uint16_t> core(std::begin(kCoreExtensions),
                              std::end(kCoreExtensions));

  // Conditionally remove session_ticket (0x0023) if not sending.
  if (!send_session_ticket) {
    core.erase(std::remove(core.begin(), core.end(), 0x0023), core.end());
  }

  // Keep SNI (0x0000) first, shuffle the rest.
  std::vector<uint16_t> tail(core.begin() + 1, core.end());
  rng.Shuffle(tail);
  config.extensions.push_back(core[0]);  // SNI first
  for (uint16_t e : tail) config.extensions.push_back(e);

  // Randomly include some optional extensions.
  for (uint16_t opt : kOptionalExtensions) {
    // Skip session_ticket and padding from optional — handled separately.
    if (opt == 0x0029 && !send_session_ticket) continue;
    if (opt == 0x0015) continue;  // padding handled below
    if (rng.Range(1, 100) <= 40) {
      config.extensions.push_back(opt);
    }
  }

  // Add padding extension if target is set.
  if (padding_target > 0) {
    config.extensions.push_back(0x0015);  // padding extension
  }

  // Trailing GREASE.
  config.extensions.push_back(
      config.grease_values[(num_grease_extensions - 1) % num_grease_extensions]);

  // 4. Supported groups (same logic as base version).
  config.supported_groups.clear();
  config.supported_groups.push_back(config.grease_values[0]);
  std::vector<uint16_t> core_groups = {0x001D, 0x0017, 0x0018};
  std::vector<uint16_t> optional_groups;
  if (rng.RandBool()) optional_groups.push_back(0x0019);
  if (rng.RandBool()) optional_groups.push_back(0x001E);
  if (rng.RandBool()) optional_groups.push_back(0x0100);
  rng.Shuffle(optional_groups);
  for (uint16_t g : core_groups) config.supported_groups.push_back(g);
  for (uint16_t g : optional_groups) config.supported_groups.push_back(g);

  // 5. Signature algorithms.
  config.signature_algorithms.clear();
  std::vector<uint16_t> sigalgs(std::begin(kSignatureAlgorithms),
                                 std::end(kSignatureAlgorithms));
  for (size_t i = 0; i + 1 < sigalgs.size(); i += 2) {
    if (rng.RandBool()) std::swap(sigalgs[i], sigalgs[i + 1]);
  }
  config.signature_algorithms = sigalgs;

  // 6-10: Same as base.
  config.ec_point_formats = {0x00};
  config.alpn_protocols = {"h2", "http/1.1"};
  config.psk_key_exchange_modes = {0x01};
  config.supported_versions.clear();
  config.supported_versions.push_back(
      config.grease_values[1 % num_grease_extensions]);
  if (rng.RandBool()) {
    config.supported_versions.push_back(0x0304);
    config.supported_versions.push_back(0x0303);
  } else {
    config.supported_versions.push_back(0x0303);
    config.supported_versions.push_back(0x0304);
  }
  config.compress_certificate_algos = {0x0002};

  // Store padding/session info.
  config.padding_target = padding_target;
  config.send_session_ticket = send_session_ticket;

  return config;
}

// =============================================================
// JA3 computation: md5(SSLver,Ciphers,Ext,EllCurves,EllCurvePointFmt)
// =============================================================
std::string ComputeJA3Hash(const TLSClientHelloConfig& config) {
  // JA3 format: SSLVersion,Ciphers,Extensions,EllipticCurves,ECPointFormats
  // Ciphers/Extensions/Curves are comma-separated decimal, GREASE removed.

  auto IsGrease = [](uint16_t v) -> bool {
    return (v & 0x0F0F) == 0x0A0A;
  };

  std::string ja3;
  // SSL version: TLS 1.2 = 771 (0x0303) is what JA3 typically records.
  ja3 += "771,";

  // Cipher suites (no GREASE).
  std::string ciphers;
  for (uint16_t c : config.cipher_suites) {
    if (IsGrease(c)) continue;
    if (!ciphers.empty()) ciphers += "-";
    ciphers += std::to_string(c);
  }
  ja3 += ciphers + ",";

  // Extensions (no GREASE).
  std::string exts;
  for (uint16_t e : config.extensions) {
    if (IsGrease(e)) continue;
    if (!exts.empty()) exts += "-";
    exts += std::to_string(e);
  }
  ja3 += exts + ",";

  // Elliptic curves (no GREASE).
  std::string curves;
  for (uint16_t g : config.supported_groups) {
    if (IsGrease(g)) continue;
    if (!curves.empty()) curves += "-";
    curves += std::to_string(g);
  }
  ja3 += curves + ",";

  // EC point formats.
  std::string ecpf;
  for (uint8_t p : config.ec_point_formats) {
    if (!ecpf.empty()) ecpf += "-";
    ecpf += std::to_string(p);
  }
  ja3 += ecpf;

  // In production, hash with MD5. For now return the raw string.
  // To compute actual hash: base::MD5String(ja3, &digest);
  return ja3;
}

std::string ComputeJA4(const TLSClientHelloConfig& config) {
  // JA4 format: {proto}{version}{SNI}{ciphers_count}{ext_count}_{hash}
  // This is a simplified JA4 computation.

  auto IsGrease = [](uint16_t v) -> bool {
    return (v & 0x0F0F) == 0x0A0A;
  };

  int cipher_count = 0;
  for (uint16_t c : config.cipher_suites) {
    if (!IsGrease(c)) cipher_count++;
  }

  int ext_count = 0;
  for (uint16_t e : config.extensions) {
    if (!IsGrease(e)) ext_count++;
  }

  bool has_sni = false;
  for (uint16_t e : config.extensions) {
    if (e == 0x0000) { has_sni = true; break; }
  }

  // JA4: t{13}{d|i}{cipher_count:02d}{ext_count:02d}_{first12_of_hash}
  // t = TCP, 13 = TLS 1.3, d = domain (SNI present), i = IP (no SNI)
  return base::StringPrintf("t13%c%02d%02d_...",
                            has_sni ? 'd' : 'i',
                            cipher_count, ext_count);
}

}  // namespace normal_browser
