// Copyright 2024 Normal Browser Authors. All rights reserved.
// audio_spoofing.cc — AudioContext / Web Audio fingerprint spoofing.
//
// Files to modify in Chromium:
//   third_party/blink/renderer/modules/webaudio/oscillator_node.cc
//   third_party/blink/renderer/modules/webaudio/dynamics_compressor_node.cc
//   third_party/blink/renderer/modules/webaudio/audio_context.cc
//   third_party/blink/renderer/modules/webaudio/offline_audio_context.cc
//
// Strategy:
//   1. OscillatorNode: inject sub-sample phase offset per session.
//   2. DynamicsCompressorNode: bias knee/threshold/ratio by ±0.0001.
//   3. AudioContext.sampleRate & baseLatency: match SoC profile.
//   4. AnalyserNode.getFloatFrequencyData: add ±epsilon noise.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <cmath>
#include <cstring>

namespace blink {

// ====================================================================
// Audio Noise Utility
// Deterministic hash for audio sample perturbation.
// ====================================================================
namespace {

uint64_t AudioHash(uint64_t seed, uint64_t index) {
  uint64_t h = seed ^ (index * 0x517CC1B727220A95ULL);
  h ^= h >> 33;
  h *= 0xFF51AFD7ED558CCDULL;
  h ^= h >> 33;
  h *= 0xC4CEB9FE1A85EC53ULL;
  h ^= h >> 33;
  return h;
}

// Convert float seed to uint64 for hashing (bit-preserving).
uint64_t SeedToU64(float seed) {
  uint32_t bits = 0;
  memcpy(&bits, &seed, sizeof(bits));
  return static_cast<uint64_t>(bits);
}

// Returns a small noise value in [-magnitude, +magnitude]
double AudioNoise(uint64_t seed, uint64_t index, double magnitude) {
  uint64_t h = AudioHash(seed, index);
  double normalized = static_cast<double>(h & 0xFFFFFFFF) /
                      static_cast<double>(0xFFFFFFFF) * 2.0 - 1.0;
  return normalized * magnitude;
}

}  // namespace

// ====================================================================
// AudioContext — sampleRate and baseLatency spoofing
// ====================================================================
float GetSpoofedSampleRate() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 48000.0f;
  return static_cast<float>(client->GetProfile().audio_sample_rate);
}

double GetSpoofedBaseLatency() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 0.01;

  const auto& p = client->GetProfile();
  return static_cast<double>(p.audio_buffer_size) /
         static_cast<double>(p.audio_sample_rate);
}

double GetSpoofedOutputLatency() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return 0.02;

  const auto& p = client->GetProfile();
  double base = static_cast<double>(p.audio_buffer_size) /
                static_cast<double>(p.audio_sample_rate);
  uint64_t seed = SeedToU64(p.audio_context_seed);
  double jitter = AudioNoise(seed, 0xAD10EA70ULL, 0.002);
  return base * 2.0 + jitter;
}

// ====================================================================
// OscillatorNode — sub-sample phase offset
// ====================================================================
double ApplyOscillatorPhaseOffset(double virtual_read_index,
                                  unsigned channel) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return virtual_read_index;

  uint64_t seed = SeedToU64(client->GetProfile().audio_context_seed);
  if (seed == 0) return virtual_read_index;

  double offset = AudioNoise(seed, 0x05C00000ULL + channel, 0.001);
  return virtual_read_index + offset;
}

// ====================================================================
// DynamicsCompressorNode — parameter bias
// ====================================================================
struct CompressorParams {
  float threshold;
  float knee;
  float ratio;
  float attack;
  float release;
};

CompressorParams ApplyCompressorBias(const CompressorParams& original) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return original;

  uint64_t seed = SeedToU64(client->GetProfile().audio_context_seed);
  if (seed == 0) return original;

  CompressorParams biased = original;
  biased.threshold += static_cast<float>(
      AudioNoise(seed, 0xC0E7BE00ULL, 0.0001));
  biased.knee += static_cast<float>(
      AudioNoise(seed, 0xC0EBEE00ULL, 0.0001));
  biased.ratio += static_cast<float>(
      AudioNoise(seed, 0xC0E2A700ULL, 0.00005));
  biased.attack += static_cast<float>(
      AudioNoise(seed, 0xC0EA7B00ULL, 0.00001));
  biased.release += static_cast<float>(
      AudioNoise(seed, 0xC0E2E100ULL, 0.00001));

  return biased;
}

// ====================================================================
// REMOVED: ApplyAnalyserNoise()
//
// Post-processing noise on AnalyserNode output is DETECTABLE by FPJS.
// FPJS calls getFloatFrequencyData() multiple times on the same audio —
// if the base signal is identical but noise differs from known device
// patterns, it flags tampering. The compressor bias (above) already
// produces naturally different audio fingerprints by modifying the
// actual DSP parameters. Output-stage noise is counterproductive.
// ====================================================================

// ====================================================================
// REMOVED: ApplyTimeDomainNoise()
//
// Same issue as ApplyAnalyserNoise() — post-processing time domain
// data adds detectable artifacts. Removed to avoid FPJS tampering flag.
// ====================================================================

// ====================================================================
// REMOVED: ApplyOfflineRenderNoise()
//
// This was the MOST DETECTABLE audio tampering signal. Modifying
// every 17th sample in the offline render output creates a sparse
// noise pattern identical to Brave's audio farbling. FPJS's standard
// test (OscillatorNode → DynamicsCompressorNode → startRendering())
// specifically checks for this pattern.
//
// The correct approach is already implemented above:
// - ApplyCompressorBias() modifies DynamicsCompressor thresholds
//   by ±0.0001, which changes the compression curve and produces
//   naturally different output samples — indistinguishable from
//   real hardware variation in audio processing.
// - ApplyOscillatorPhaseOffset() adds sub-sample phase offset
//   which affects the base signal generation.
// These together produce unique audio fingerprints without any
// detectable post-processing artifacts.
// ====================================================================

}  // namespace blink
