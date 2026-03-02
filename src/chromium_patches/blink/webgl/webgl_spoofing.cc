// Copyright 2024 Normal Browser Authors. All rights reserved.
// webgl_spoofing.cc — WebGL fingerprint spoofing patches.
//
// Files to modify in Chromium:
//   gpu/command_buffer/service/gles2_cmd_decoder.cc
//   gpu/config/gpu_info.cc
//   third_party/angle/src/libANGLE/renderer/ (for precision hints)

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include "gpu/config/gpu_info.h"
#include <cstdio>

namespace gpu {

// ====================================================================
// GPU Info Overrides
// File: gpu/config/gpu_info.cc
// Override GL_RENDERER, GL_VENDOR, GL_VERSION strings.
// ====================================================================

// ORIGINAL: returns real GPU info from driver
// REPLACEMENT: returns profile GPU info

std::string GpuInfo::gl_renderer() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().gl_renderer;
  }
  return gl_renderer_;
}

std::string GpuInfo::gl_vendor() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().gl_vendor;
  }
  return gl_vendor_;
}

std::string GpuInfo::gl_version() const {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (client && client->IsReady()) {
    return client->GetProfile().gl_version;
  }
  return gl_version_;
}

}  // namespace gpu

namespace gpu::gles2 {

// ====================================================================
// GLES2 Command Decoder — getParameter() overrides
// File: gpu/command_buffer/service/gles2_cmd_decoder.cc
//
// When JavaScript calls gl.getParameter(gl.RENDERER), etc., it routes
// through the GPU command buffer. Intercept here.
// ====================================================================

// Called from GLES2DecoderImpl::DoGetString()
const char* GetSpoofedGLString(unsigned int name) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return nullptr;

  const auto& p = client->GetProfile();
  switch (name) {
    case 0x1F01:  // GL_RENDERER
      return p.gl_renderer.c_str();
    case 0x1F00:  // GL_VENDOR
      return p.gl_vendor.c_str();
    case 0x1F02:  // GL_VERSION
      return p.gl_version.c_str();
    case 0x8B8C:  // GL_SHADING_LANGUAGE_VERSION
      return p.gl_shading_language_version.c_str();
    default:
      return nullptr;
  }
}

// Called from GLES2DecoderImpl::DoGetIntegerv()
bool GetSpoofedGLInteger(unsigned int pname, int* result) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;

  const auto& p = client->GetProfile();
  switch (pname) {
    case 0x0D33:  // GL_MAX_TEXTURE_SIZE
      *result = p.gl_max_texture_size;
      return true;
    case 0x84E8:  // GL_MAX_RENDERBUFFER_SIZE
      *result = p.gl_max_renderbuffer_size;
      return true;
    case 0x8872:  // GL_MAX_TEXTURE_IMAGE_UNITS
      *result = p.gl_max_combined_texture_units;
      return true;
    case 0x8869:  // GL_MAX_VERTEX_ATTRIBS
      *result = p.gl_max_vertex_attribs;
      return true;
    case 0x8DFC:  // GL_MAX_VARYING_VECTORS
      *result = p.gl_max_varying_vectors;
      return true;
    default:
      return false;
  }
}

// Called from GLES2DecoderImpl::DoGetFloatv()
bool GetSpoofedGLFloat(unsigned int pname, float* result) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;

  const auto& p = client->GetProfile();
  switch (pname) {
    case 0x846D:  // GL_ALIASED_POINT_SIZE_RANGE
      result[0] = p.gl_aliased_point_size[0];
      result[1] = p.gl_aliased_point_size[1];
      return true;
    case 0x846E:  // GL_ALIASED_LINE_WIDTH_RANGE
      result[0] = p.gl_aliased_line_width[0];
      result[1] = p.gl_aliased_line_width[1];
      return true;
    default:
      return false;
  }
}

// GL_MAX_VIEWPORT_DIMS
bool GetSpoofedGLViewportDims(int* result) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return false;

  const auto& p = client->GetProfile();
  result[0] = p.gl_max_viewport_dims[0];
  result[1] = p.gl_max_viewport_dims[1];
  return true;
}

// ====================================================================
// GL Extensions spoofing
// File: gpu/command_buffer/service/gles2_cmd_decoder.cc
// Function: DoGetString(GL_EXTENSIONS)
//
// Return only extensions from the spoofed GPU profile.
// ====================================================================
std::string GetSpoofedGLExtensions() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return "";

  const auto& exts = client->GetProfile().gl_extensions;
  std::string result;
  for (size_t i = 0; i < exts.size(); ++i) {
    if (i > 0) result += " ";
    result += exts[i];
  }
  return result;
}

// ====================================================================
// REMOVED: ApplyWebGLShaderNoise()
//
// Injecting a constant into fragment shaders was DETECTABLE by FPJS:
// 1. The "// nb" comment and "_nb_bias" constant name are searchable
//    in compiled shader source (FPJS calls getShaderSource()).
// 2. Even without the name, the extra constant changes shader
//    compilation timing and output in a way that doesn't match any
//    known GPU driver behavior.
// 3. FPJS compares WebGL canvas renders against known GPU baselines.
//    The shader modification produces output that matches no real GPU,
//    triggering anomaly detection.
//
// The correct WebGL fingerprint variation is ALREADY achieved by:
// - GL_RENDERER / GL_VENDOR / GL_VERSION string spoofing (above)
// - GL extension list from the GPU database (per-SoC)
// - GL parameter limits (max texture size, etc.)
// These make WebGL report as a different GPU, which naturally produces
// different renders due to different driver implementations.
//
// Real browsers do NOT modify shader source — this was the most
// obvious tampering signal in the WebGL stack.
// ====================================================================

}  // namespace gpu::gles2
