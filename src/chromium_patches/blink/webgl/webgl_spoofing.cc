// Copyright 2024 Normal Browser Authors. All rights reserved.
// webgl_spoofing.cc — WebGL fingerprint spoofing patches.
//
// Compiled as part of blink core sources. All functions are in
// blink::webgl_spoofing namespace and called from the patched
// webgl_rendering_context_base.cc.

#include "third_party/blink/renderer/core/ghost_profile_client.h"

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {
namespace webgl_spoofing {

// Per-GPU-family WebGL extension sets.
// These are the ACTUAL extension lists Chrome returns on real devices
// with these GPUs, captured from real Android devices.

namespace {

// Adreno 750/740/730 (Snapdragon 8 Gen 1/2/3) — flagship Qualcomm
const std::vector<std::string> kAdrenoFlagshipWebGLExtensions = {
    "ANGLE_instanced_arrays",
    "EXT_blend_minmax",
    "EXT_color_buffer_half_float",
    "EXT_disjoint_timer_query",
    "EXT_float_blend",
    "EXT_frag_depth",
    "EXT_shader_texture_lod",
    "EXT_texture_compression_bptc",
    "EXT_texture_compression_rgtc",
    "EXT_texture_filter_anisotropic",
    "EXT_sRGB",
    "KHR_parallel_shader_compile",
    "OES_element_index_uint",
    "OES_fbo_render_mipmap",
    "OES_standard_derivatives",
    "OES_texture_float",
    "OES_texture_float_linear",
    "OES_texture_half_float",
    "OES_texture_half_float_linear",
    "OES_vertex_array_object",
    "WEBGL_color_buffer_float",
    "WEBGL_compressed_texture_astc",
    "WEBGL_compressed_texture_etc",
    "WEBGL_compressed_texture_etc1",
    "WEBGL_debug_renderer_info",
    "WEBGL_debug_shaders",
    "WEBGL_depth_texture",
    "WEBGL_draw_buffers",
    "WEBGL_lose_context",
    "WEBGL_multi_draw",
};

// Adreno 725/720/710/619/613 — mid-range/budget Qualcomm
const std::vector<std::string> kAdrenoMidWebGLExtensions = {
    "ANGLE_instanced_arrays",
    "EXT_blend_minmax",
    "EXT_color_buffer_half_float",
    "EXT_disjoint_timer_query",
    "EXT_float_blend",
    "EXT_frag_depth",
    "EXT_shader_texture_lod",
    "EXT_texture_filter_anisotropic",
    "EXT_sRGB",
    "KHR_parallel_shader_compile",
    "OES_element_index_uint",
    "OES_fbo_render_mipmap",
    "OES_standard_derivatives",
    "OES_texture_float",
    "OES_texture_float_linear",
    "OES_texture_half_float",
    "OES_texture_half_float_linear",
    "OES_vertex_array_object",
    "WEBGL_color_buffer_float",
    "WEBGL_compressed_texture_astc",
    "WEBGL_compressed_texture_etc",
    "WEBGL_compressed_texture_etc1",
    "WEBGL_debug_renderer_info",
    "WEBGL_debug_shaders",
    "WEBGL_depth_texture",
    "WEBGL_draw_buffers",
    "WEBGL_lose_context",
    "WEBGL_multi_draw",
};

// Mali-G720/G715/G710 (Dimensity 9xxx, Tensor G3/G4, Exynos 2400)
const std::vector<std::string> kMaliFlagshipWebGLExtensions = {
    "ANGLE_instanced_arrays",
    "EXT_blend_minmax",
    "EXT_color_buffer_half_float",
    "EXT_disjoint_timer_query",
    "EXT_float_blend",
    "EXT_frag_depth",
    "EXT_shader_texture_lod",
    "EXT_texture_compression_bptc",
    "EXT_texture_filter_anisotropic",
    "EXT_sRGB",
    "KHR_parallel_shader_compile",
    "OES_element_index_uint",
    "OES_fbo_render_mipmap",
    "OES_standard_derivatives",
    "OES_texture_float",
    "OES_texture_float_linear",
    "OES_texture_half_float",
    "OES_texture_half_float_linear",
    "OES_vertex_array_object",
    "WEBGL_color_buffer_float",
    "WEBGL_compressed_texture_astc",
    "WEBGL_compressed_texture_etc",
    "WEBGL_compressed_texture_etc1",
    "WEBGL_debug_renderer_info",
    "WEBGL_debug_shaders",
    "WEBGL_depth_texture",
    "WEBGL_draw_buffers",
    "WEBGL_lose_context",
    "WEBGL_multi_draw",
};

// Mali-G610/G57/G52/G68 — mid-range/budget ARM Mali
const std::vector<std::string> kMaliMidWebGLExtensions = {
    "ANGLE_instanced_arrays",
    "EXT_blend_minmax",
    "EXT_color_buffer_half_float",
    "EXT_float_blend",
    "EXT_frag_depth",
    "EXT_shader_texture_lod",
    "EXT_texture_filter_anisotropic",
    "EXT_sRGB",
    "KHR_parallel_shader_compile",
    "OES_element_index_uint",
    "OES_fbo_render_mipmap",
    "OES_standard_derivatives",
    "OES_texture_float",
    "OES_texture_float_linear",
    "OES_texture_half_float",
    "OES_texture_half_float_linear",
    "OES_vertex_array_object",
    "WEBGL_color_buffer_float",
    "WEBGL_compressed_texture_astc",
    "WEBGL_compressed_texture_etc",
    "WEBGL_compressed_texture_etc1",
    "WEBGL_debug_renderer_info",
    "WEBGL_debug_shaders",
    "WEBGL_depth_texture",
    "WEBGL_draw_buffers",
    "WEBGL_lose_context",
};

// Mali-G52/G36 budget / PowerVR — low-end
const std::vector<std::string> kBudgetWebGLExtensions = {
    "ANGLE_instanced_arrays",
    "EXT_blend_minmax",
    "EXT_color_buffer_half_float",
    "EXT_frag_depth",
    "EXT_shader_texture_lod",
    "EXT_texture_filter_anisotropic",
    "EXT_sRGB",
    "OES_element_index_uint",
    "OES_standard_derivatives",
    "OES_texture_float",
    "OES_texture_float_linear",
    "OES_texture_half_float",
    "OES_texture_half_float_linear",
    "OES_vertex_array_object",
    "WEBGL_color_buffer_float",
    "WEBGL_compressed_texture_etc",
    "WEBGL_compressed_texture_etc1",
    "WEBGL_debug_renderer_info",
    "WEBGL_depth_texture",
    "WEBGL_draw_buffers",
    "WEBGL_lose_context",
};

const std::vector<std::string>& GetWebGLExtensionsForGPU(
    const std::string& gl_renderer, int soc_tier) {
  // Detect GPU family from gl_renderer string
  if (gl_renderer.find("Adreno") != std::string::npos) {
    // Adreno 7xx = flagship, 6xx = mid/budget
    if (gl_renderer.find("750") != std::string::npos ||
        gl_renderer.find("740") != std::string::npos ||
        gl_renderer.find("730") != std::string::npos) {
      return kAdrenoFlagshipWebGLExtensions;
    }
    return kAdrenoMidWebGLExtensions;
  }
  if (gl_renderer.find("Mali") != std::string::npos ||
      gl_renderer.find("Immortalis") != std::string::npos) {
    if (gl_renderer.find("G720") != std::string::npos ||
        gl_renderer.find("G715") != std::string::npos ||
        gl_renderer.find("G710") != std::string::npos ||
        gl_renderer.find("Immortalis") != std::string::npos) {
      return kMaliFlagshipWebGLExtensions;
    }
    if (gl_renderer.find("G52") != std::string::npos ||
        gl_renderer.find("GE8") != std::string::npos) {
      return kBudgetWebGLExtensions;
    }
    return kMaliMidWebGLExtensions;
  }
  if (gl_renderer.find("Xclipse") != std::string::npos) {
    return kMaliFlagshipWebGLExtensions;  // Samsung RDNA-based, similar to Mali
  }
  // Fallback by tier
  if (soc_tier <= 1) return kMaliFlagshipWebGLExtensions;
  if (soc_tier == 2) return kMaliMidWebGLExtensions;
  return kBudgetWebGLExtensions;
}

}  // anonymous namespace

// ====================================================================
// GetSpoofedWebGLExtensions — called from getSupportedExtensions()
//
// HOOK POINT in webgl_rendering_context_base.cc:
//   In WebGLRenderingContextBase::getSupportedExtensions(), REPLACE
//   the original return with:
//     auto spoofed = blink::webgl_spoofing::GetSpoofedWebGLExtensions();
//     if (!spoofed.empty()) {
//       Vector<String> result;
//       for (const auto& e : spoofed)
//         result.push_back(String::FromUTF8(e.c_str()));
//       return result;
//     }
//   before the original logic.
// ====================================================================
std::vector<std::string> GetSpoofedWebGLExtensions() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return {};

  const auto& p = client->GetProfile();
  const auto& exts = GetWebGLExtensionsForGPU(p.gl_renderer, p.soc_tier);
  return exts;
}

// ====================================================================
// ShouldEnableWebGLExtension — called from getExtension()
//
// HOOK POINT in webgl_rendering_context_base.cc:
//   In WebGLRenderingContextBase::getExtension(), BEFORE enabling
//   any extension, check:
//     if (!blink::webgl_spoofing::ShouldEnableWebGLExtension(
//             name.Utf8()))
//       return nullptr;
// ====================================================================
bool ShouldEnableWebGLExtension(const std::string& name) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return true;  // Allow all if no profile

  const auto& p = client->GetProfile();
  const auto& exts = GetWebGLExtensionsForGPU(p.gl_renderer, p.soc_tier);

  for (const auto& allowed : exts) {
    if (allowed == name) return true;
  }
  return false;
}

// ====================================================================
// GL Parameter Spoofing — matching signatures expected by the patched
// webgl_rendering_context_base.cc
// ====================================================================

// Returns spoofed GL string for RENDERER/VENDOR/VERSION/SHADING_LANGUAGE
// Used as: auto spoofed = GetSpoofedGLString(pname);
//          if (spoofed) return WebGLAny(state, *spoofed);
std::optional<String> GetSpoofedGLString(unsigned int pname) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return std::nullopt;

  const auto& p = client->GetProfile();
  switch (pname) {
    case 0x1F01:  // GL_RENDERER
      return String::FromUTF8(p.gl_renderer.c_str());
    case 0x1F00:  // GL_VENDOR
      return String::FromUTF8(p.gl_vendor.c_str());
    case 0x1F02:  // GL_VERSION
      return String::FromUTF8(p.gl_version.c_str());
    case 0x8B8C:  // GL_SHADING_LANGUAGE_VERSION
      return String::FromUTF8(p.gl_shading_language_version.c_str());
    default:
      return std::nullopt;
  }
}

// Returns spoofed integer GL parameter
std::optional<int> GetSpoofedGLInteger(unsigned int pname) {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return std::nullopt;

  const auto& p = client->GetProfile();
  switch (pname) {
    case 0x0D33:  // GL_MAX_TEXTURE_SIZE
      return p.gl_max_texture_size;
    case 0x84E8:  // GL_MAX_RENDERBUFFER_SIZE
      return p.gl_max_renderbuffer_size;
    case 0x8872:  // GL_MAX_TEXTURE_IMAGE_UNITS
      return p.gl_max_combined_texture_units;
    case 0x8869:  // GL_MAX_VERTEX_ATTRIBS
      return p.gl_max_vertex_attribs;
    case 0x8DFC:  // GL_MAX_VARYING_VECTORS
      return p.gl_max_varying_vectors;
    default:
      return std::nullopt;
  }
}

// Returns spoofed float pair (aliased point size, line width ranges)
bool GetSpoofedGLFloatPair(unsigned int pname, float* result) {
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

// GL Extensions — returns a WTF::String of space-separated GL extensions.
// Called from getSupportedExtensions() and split by ' '.
std::optional<String> GetSpoofedGLExtensions() {
  auto* client = normal_browser::GhostProfileClient::Get();
  if (!client || !client->IsReady()) return std::nullopt;

  // Return WebGL extension names (not GL extension names)
  const auto& p = client->GetProfile();
  const auto& webgl_exts = GetWebGLExtensionsForGPU(p.gl_renderer, p.soc_tier);

  if (webgl_exts.empty()) return std::nullopt;

  StringBuilder sb;
  for (size_t i = 0; i < webgl_exts.size(); ++i) {
    if (i > 0) sb.Append(' ');
    sb.Append(String::FromUTF8(webgl_exts[i].c_str()));
  }
  return sb.ToString();
}

}  // namespace webgl_spoofing
}  // namespace blink
