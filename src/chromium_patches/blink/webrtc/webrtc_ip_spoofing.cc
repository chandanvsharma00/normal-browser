// Copyright 2024 Normal Browser Authors. All rights reserved.
// webrtc_ip_spoofing.cc — Prevent real local IP leak via WebRTC ICE candidates.
//
// FILES TO MODIFY:
//   third_party/blink/renderer/modules/peerconnection/
//     rtc_peer_connection.cc
//   content/renderer/media/webrtc/
//     peer_connection_dependency_factory.cc
//
// PROBLEM:
//   RTCPeerConnection.createOffer() → ICE candidates → contains real local IP.
//   Fingerprinters call: new RTCPeerConnection({iceServers:[]}).createOffer()
//   and parse the SDP to extract 192.168.x.x or 10.x.x.x.
//
// SOLUTION:
//   Intercept ICE candidate generation and replace real local IPs with
//   the fake IP from the RendererProfile.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cstring>
#include <regex>
#include <string>

namespace blink {

// ====================================================================
// PATCH 1: Replace local IPs in ICE candidate SDP lines.
//
// Where to call: In RTCPeerConnection::OnIceCandidate(), before the
// candidate is dispatched to JavaScript.
//
// ICE candidate format contains lines like:
//   "candidate:... 192.168.1.100 ..."
//   "a=candidate:... 10.0.0.5 ..."
//
// We match private IPv4 ranges and replace with profile's fake_local_ip.
// ====================================================================

namespace {

// Regex matching private IPv4 addresses in ICE candidate strings.
// Matches: 192.168.x.x, 10.x.x.x, 172.16-31.x.x
const std::regex kPrivateIPv4Regex(
    R"(\b(192\.168\.\d{1,3}\.\d{1,3}|10\.\d{1,3}\.\d{1,3}\.\d{1,3}|172\.(1[6-9]|2\d|3[01])\.\d{1,3}\.\d{1,3})\b)");

// Also match IPv6 link-local (fe80::...)
const std::regex kLinkLocalIPv6Regex(
    R"(\bfe80:[0-9a-fA-F:]+\b)");

}  // namespace

std::string SpoofICECandidateIP(const std::string& candidate_sdp) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return candidate_sdp;
  const auto& profile = client->GetProfile();
  if (profile.fake_local_ip.empty()) return candidate_sdp;

  // Replace all private IPv4 addresses with the spoofed one.
  std::string result = std::regex_replace(
      candidate_sdp, kPrivateIPv4Regex, profile.fake_local_ip);

  // Replace link-local IPv6 with a fake derived from the IPv4.
  // e.g., fe80::1 (minimal, won't match real device fingerprint).
  result = std::regex_replace(
      result, kLinkLocalIPv6Regex, "fe80::1");

  return result;
}

// ====================================================================
// PATCH 2: Filter mDNS ICE candidates.
//
// Chrome 74+ uses mDNS for local candidate obfuscation:
//   "candidate:... abcd1234-5678-9012-3456-789012345678.local ..."
//
// This is normally safe, but the UUID is stable across sessions
// (derived from MAC address). We need to filter or replace the UUID
// to prevent cross-session correlation.
//
// Where to call: Same as PATCH 1.
// ====================================================================
std::string SpoofMDNSCandidate(const std::string& candidate_sdp) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return candidate_sdp;
  const auto& profile = client->GetProfile();

  // Match mDNS hostname pattern: UUID.local
  static const std::regex kMDNSRegex(
      R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\.local\b)");

  // Generate a deterministic fake UUID from the canvas_noise_seed.
  // This ensures the same profile always produces the same mDNS hostname,
  // but different profiles produce different ones.
  uint32_t seed_bits = 0;
  std::memcpy(&seed_bits, &profile.canvas_noise_seed, sizeof(seed_bits));

  char fake_uuid[48];
  snprintf(fake_uuid, sizeof(fake_uuid),
           "%08x-%04x-%04x-%04x-%08x%04x.local",
           seed_bits,
           static_cast<uint16_t>(seed_bits >> 4),
           static_cast<uint16_t>(0x4000 | (seed_bits & 0x0FFF)),
           static_cast<uint16_t>(0x8000 | ((seed_bits >> 16) & 0x3FFF)),
           seed_bits ^ 0xDEADBEEF,
           static_cast<uint16_t>(seed_bits >> 8));

  return std::regex_replace(candidate_sdp, kMDNSRegex,
                            std::string(fake_uuid));
}

// ====================================================================
// PATCH 3: Full ICE candidate spoofing — call both patches.
//
// Where to integrate:
//   In third_party/blink/renderer/modules/peerconnection/
//     rtc_peer_connection.cc → OnIceCandidate() or similar.
//
//   Before dispatching the candidate event:
//     candidate_sdp = SpoofWebRTCCandidate(candidate_sdp);
// ====================================================================
std::string SpoofWebRTCCandidate(const std::string& candidate_sdp) {
  std::string result = SpoofICECandidateIP(candidate_sdp);
  result = SpoofMDNSCandidate(result);
  return result;
}

// ====================================================================
// PATCH 4: Prevent WebRTC from revealing number of network interfaces.
//
// Where to call: In the network interface enumeration path,
//   content/renderer/media/webrtc/peer_connection_dependency_factory.cc
//
// Returns a spoofed network interface list with just one WiFi interface.
// This prevents fingerprinting via the number/type of network interfaces.
// ====================================================================
bool ShouldFilterNetworkInterfaces() {
  // Always filter in Ghost Mode — expose only one interface.
  return true;
}

int GetSpoofedNetworkInterfaceCount() {
  // Real devices typically show 1-3 interfaces.
  // Return 1 (just WiFi) for maximum privacy.
  return 1;
}

}  // namespace blink
