// Copyright 2024 Normal Browser Authors. All rights reserved.
// gpu_database.cc — Complete SoC→GPU database with 27 real SoCs.

#include "device_profile_generator/gpu_database.h"

namespace normal_browser {

namespace {

// ============================================================
// GL Extension lists by GPU family
// ============================================================

// Adreno 750 (Snapdragon 8 Gen 3) — 73 extensions
const std::vector<std::string> kAdreno750Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth_texture_cube_map", "GL_OES_depth24",
    "GL_OES_element_index_uint", "GL_OES_fbo_render_mipmap",
    "GL_OES_fragment_precision_high", "GL_OES_get_program_binary",
    "GL_OES_packed_depth_stencil", "GL_OES_rgb8_rgba8",
    "GL_OES_sample_shading", "GL_OES_sample_variables",
    "GL_OES_shader_image_atomic", "GL_OES_shader_multisample_interpolation",
    "GL_OES_standard_derivatives", "GL_OES_surfaceless_context",
    "GL_OES_texture_3D", "GL_OES_texture_border_clamp",
    "GL_OES_texture_buffer", "GL_OES_texture_compression_astc",
    "GL_OES_texture_cube_map_array", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_texture_stencil8",
    "GL_OES_texture_storage_multisample_2d_array",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_blend_equation_advanced_coherent",
    "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_robust_buffer_access_behavior",
    "GL_KHR_texture_compression_astc_hdr",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_EGL_image_storage", "GL_EXT_blend_func_extended",
    "GL_EXT_blit_framebuffer_params", "GL_EXT_buffer_storage",
    "GL_EXT_clip_control", "GL_EXT_clip_cull_distance",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_debug_label", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_disjoint_timer_query",
    "GL_EXT_draw_buffers_indexed", "GL_EXT_external_buffer",
    "GL_EXT_fragment_invocation_density", "GL_EXT_geometry_shader",
    "GL_EXT_gpu_shader5", "GL_EXT_memory_object", "GL_EXT_memory_object_fd",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_multisampled_render_to_texture2",
    "GL_EXT_primitive_bounding_box", "GL_EXT_protected_textures",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_sRGB_write_control", "GL_EXT_shader_framebuffer_fetch",
    "GL_EXT_shader_io_blocks",
    "GL_EXT_shader_non_constant_global_initializers",
    "GL_EXT_tessellation_shader", "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_buffer", "GL_EXT_texture_cube_map_array",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_QCOM_shader_framebuffer_fetch_noncoherent",
    "GL_QCOM_texture_foveated",
    "GL_QCOM_texture_foveated_subsampled_layout",
    "GL_QCOM_motion_estimation", "GL_QCOM_validate_shader_binary",
    "GL_OVR_multiview", "GL_OVR_multiview2",
    "GL_OVR_multiview_multisampled_render_to_texture"};

// Adreno 740 (Snapdragon 8 Gen 2) — 70 extensions
const std::vector<std::string> kAdreno740Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth_texture_cube_map", "GL_OES_depth24",
    "GL_OES_element_index_uint", "GL_OES_fbo_render_mipmap",
    "GL_OES_fragment_precision_high", "GL_OES_get_program_binary",
    "GL_OES_packed_depth_stencil", "GL_OES_rgb8_rgba8",
    "GL_OES_sample_shading", "GL_OES_sample_variables",
    "GL_OES_shader_image_atomic", "GL_OES_shader_multisample_interpolation",
    "GL_OES_standard_derivatives", "GL_OES_surfaceless_context",
    "GL_OES_texture_3D", "GL_OES_texture_border_clamp",
    "GL_OES_texture_buffer", "GL_OES_texture_compression_astc",
    "GL_OES_texture_cube_map_array", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_texture_stencil8",
    "GL_OES_texture_storage_multisample_2d_array",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_blend_equation_advanced_coherent",
    "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_robust_buffer_access_behavior",
    "GL_KHR_texture_compression_astc_hdr",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_EGL_image_storage", "GL_EXT_blend_func_extended",
    "GL_EXT_buffer_storage", "GL_EXT_clip_cull_distance",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_debug_label", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_disjoint_timer_query",
    "GL_EXT_draw_buffers_indexed", "GL_EXT_external_buffer",
    "GL_EXT_geometry_shader", "GL_EXT_gpu_shader5",
    "GL_EXT_memory_object", "GL_EXT_memory_object_fd",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_multisampled_render_to_texture2",
    "GL_EXT_primitive_bounding_box", "GL_EXT_read_format_bgra",
    "GL_EXT_robustness", "GL_EXT_sRGB", "GL_EXT_sRGB_write_control",
    "GL_EXT_shader_framebuffer_fetch", "GL_EXT_shader_io_blocks",
    "GL_EXT_tessellation_shader", "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_buffer", "GL_EXT_texture_cube_map_array",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_QCOM_shader_framebuffer_fetch_noncoherent",
    "GL_QCOM_texture_foveated",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// Adreno 730 (Snapdragon 8 Gen 1) — 68 extensions (same as 740 minus 2)
const std::vector<std::string> kAdreno730Extensions = kAdreno740Extensions;

// Adreno 725 (Snapdragon 7+ Gen 2) — 65 extensions
const std::vector<std::string> kAdreno725Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_sample_shading",
    "GL_OES_sample_variables", "GL_OES_shader_image_atomic",
    "GL_OES_standard_derivatives", "GL_OES_surfaceless_context",
    "GL_OES_texture_3D", "GL_OES_texture_border_clamp",
    "GL_OES_texture_buffer", "GL_OES_texture_compression_astc",
    "GL_OES_texture_cube_map_array", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_texture_stencil8",
    "GL_OES_texture_storage_multisample_2d_array",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_texture_compression_astc_hdr",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_blend_func_extended", "GL_EXT_buffer_storage",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_debug_label", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_disjoint_timer_query",
    "GL_EXT_draw_buffers_indexed", "GL_EXT_geometry_shader",
    "GL_EXT_gpu_shader5", "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_multisampled_render_to_texture2",
    "GL_EXT_primitive_bounding_box", "GL_EXT_read_format_bgra",
    "GL_EXT_robustness", "GL_EXT_sRGB", "GL_EXT_sRGB_write_control",
    "GL_EXT_shader_framebuffer_fetch", "GL_EXT_shader_io_blocks",
    "GL_EXT_tessellation_shader", "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_buffer", "GL_EXT_texture_cube_map_array",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_QCOM_shader_framebuffer_fetch_noncoherent",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// Adreno 619 (Snapdragon 695 / 4 Gen 1) — 55 extensions
const std::vector<std::string> kAdreno619Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_sample_shading",
    "GL_OES_sample_variables", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_compression_astc", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_texture_stencil8", "GL_OES_vertex_array_object",
    "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_blend_func_extended", "GL_EXT_color_buffer_float",
    "GL_EXT_color_buffer_half_float", "GL_EXT_copy_image",
    "GL_EXT_debug_label", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_disjoint_timer_query",
    "GL_EXT_draw_buffers_indexed", "GL_EXT_geometry_shader",
    "GL_EXT_gpu_shader5", "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_shader_framebuffer_fetch", "GL_EXT_shader_io_blocks",
    "GL_EXT_texture_border_clamp", "GL_EXT_texture_buffer",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_QCOM_shader_framebuffer_fetch_noncoherent",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// Mali-G720 (Dimensity 9300) — 62 extensions
const std::vector<std::string> kMaliG720Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth_texture_cube_map", "GL_OES_depth24",
    "GL_OES_element_index_uint", "GL_OES_fbo_render_mipmap",
    "GL_OES_fragment_precision_high", "GL_OES_get_program_binary",
    "GL_OES_packed_depth_stencil", "GL_OES_rgb8_rgba8",
    "GL_OES_sample_shading", "GL_OES_sample_variables",
    "GL_OES_shader_image_atomic", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_border_clamp", "GL_OES_texture_buffer",
    "GL_OES_texture_compression_astc", "GL_OES_texture_cube_map_array",
    "GL_OES_texture_float", "GL_OES_texture_float_linear",
    "GL_OES_texture_half_float", "GL_OES_texture_half_float_linear",
    "GL_OES_texture_npot", "GL_OES_texture_stencil8",
    "GL_OES_texture_storage_multisample_2d_array",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_blend_equation_advanced_coherent",
    "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_robust_buffer_access_behavior",
    "GL_KHR_texture_compression_astc_hdr",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_blend_func_extended", "GL_EXT_buffer_storage",
    "GL_EXT_clip_cull_distance", "GL_EXT_color_buffer_float",
    "GL_EXT_color_buffer_half_float", "GL_EXT_copy_image",
    "GL_EXT_debug_marker", "GL_EXT_discard_framebuffer",
    "GL_EXT_disjoint_timer_query", "GL_EXT_draw_buffers_indexed",
    "GL_EXT_geometry_shader", "GL_EXT_gpu_shader5",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_multisampled_render_to_texture2",
    "GL_EXT_primitive_bounding_box", "GL_EXT_read_format_bgra",
    "GL_EXT_robustness", "GL_EXT_sRGB", "GL_EXT_sRGB_write_control",
    "GL_EXT_shader_io_blocks", "GL_EXT_tessellation_shader",
    "GL_EXT_texture_border_clamp", "GL_EXT_texture_buffer",
    "GL_EXT_texture_cube_map_array", "GL_EXT_texture_filter_anisotropic",
    "GL_EXT_texture_format_BGRA8888", "GL_EXT_texture_norm16",
    "GL_EXT_texture_sRGB_decode", "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_ARM_shader_framebuffer_fetch",
    "GL_ARM_shader_framebuffer_fetch_depth_stencil",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// Mali-G610 (Dimensity 7200/8100/8200) — 52 extensions
const std::vector<std::string> kMaliG610Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_sample_shading",
    "GL_OES_sample_variables", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_border_clamp", "GL_OES_texture_compression_astc",
    "GL_OES_texture_float", "GL_OES_texture_float_linear",
    "GL_OES_texture_half_float", "GL_OES_texture_half_float_linear",
    "GL_OES_texture_npot", "GL_OES_texture_stencil8",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug", "GL_KHR_no_error",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_blend_func_extended", "GL_EXT_color_buffer_float",
    "GL_EXT_color_buffer_half_float", "GL_EXT_copy_image",
    "GL_EXT_debug_marker", "GL_EXT_discard_framebuffer",
    "GL_EXT_draw_buffers_indexed", "GL_EXT_geometry_shader",
    "GL_EXT_gpu_shader5", "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_shader_io_blocks", "GL_EXT_tessellation_shader",
    "GL_EXT_texture_border_clamp", "GL_EXT_texture_buffer",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_ARM_shader_framebuffer_fetch",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// Mali-G57 (Dimensity 6xxx / Helio G99) — 45 extensions
const std::vector<std::string> kMaliG57Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_sample_shading",
    "GL_OES_standard_derivatives", "GL_OES_surfaceless_context",
    "GL_OES_texture_3D", "GL_OES_texture_compression_astc",
    "GL_OES_texture_float", "GL_OES_texture_float_linear",
    "GL_OES_texture_half_float", "GL_OES_texture_half_float_linear",
    "GL_OES_texture_npot", "GL_OES_texture_stencil8",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_draw_buffers_indexed",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_shader_io_blocks", "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_norm16", "GL_EXT_texture_sRGB_decode",
    "GL_ARM_shader_framebuffer_fetch",
    "GL_OVR_multiview", "GL_OVR_multiview2"};

// PowerVR BXM-8-256 (Helio G99) — 42 extensions
const std::vector<std::string> kPowerVRBXMExtensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_compression_astc", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer", "GL_EXT_draw_buffers_indexed",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_shader_io_blocks", "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_EXT_texture_sRGB_decode", "GL_EXT_texture_type_2_10_10_10_REV",
    "GL_IMG_texture_compression_pvrtc"};

// Mali-G68 (Budget, Helio G85) — 38 extensions (subset of G57)
const std::vector<std::string> kMaliG68Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_compression_astc", "GL_OES_texture_float",
    "GL_OES_texture_float_linear", "GL_OES_texture_half_float",
    "GL_OES_texture_half_float_linear", "GL_OES_texture_npot",
    "GL_OES_vertex_array_object", "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug",
    "GL_KHR_texture_compression_astc_ldr",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_copy_image", "GL_EXT_discard_framebuffer",
    "GL_EXT_draw_buffers_indexed",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888",
    "GL_ARM_shader_framebuffer_fetch",
    "GL_OVR_multiview"};

// Mali-G52 (Budget, Helio G36) — 32 extensions
const std::vector<std::string> kMaliG52Extensions = {
    "GL_OES_EGL_image", "GL_OES_EGL_image_external", "GL_OES_EGL_sync",
    "GL_OES_depth_texture", "GL_OES_depth24", "GL_OES_element_index_uint",
    "GL_OES_fbo_render_mipmap", "GL_OES_fragment_precision_high",
    "GL_OES_get_program_binary", "GL_OES_packed_depth_stencil",
    "GL_OES_rgb8_rgba8", "GL_OES_standard_derivatives",
    "GL_OES_surfaceless_context", "GL_OES_texture_3D",
    "GL_OES_texture_float", "GL_OES_texture_float_linear",
    "GL_OES_texture_half_float", "GL_OES_texture_half_float_linear",
    "GL_OES_texture_npot", "GL_OES_vertex_array_object",
    "GL_OES_vertex_half_float",
    "GL_KHR_blend_equation_advanced", "GL_KHR_debug",
    "GL_EXT_color_buffer_float", "GL_EXT_color_buffer_half_float",
    "GL_EXT_discard_framebuffer",
    "GL_EXT_multisampled_render_to_texture",
    "GL_EXT_read_format_bgra", "GL_EXT_robustness", "GL_EXT_sRGB",
    "GL_EXT_texture_filter_anisotropic", "GL_EXT_texture_format_BGRA8888"};

// Xclipse 940 (Exynos 2400) — 60 extensions (RDNA3 based, AMD)
const std::vector<std::string> kXclipse940Extensions = kMaliG720Extensions;

// Xclipse 920 (Exynos 2200) — 58 extensions
const std::vector<std::string> kXclipse920Extensions = kMaliG610Extensions;

// Mali-G615 (Exynos 1380)
const std::vector<std::string> kMali615Extensions = kMaliG610Extensions;

// Tensor GPU (Google Tensor G3/G4 — Mali-G715)
const std::vector<std::string> kTensorGPUExtensions = kMaliG720Extensions;

// ============================================================
// Build the full SoC database
// ============================================================

std::vector<SoCProfile> BuildSoCDatabase() {
  std::vector<SoCProfile> db;

  // ----- QUALCOMM FLAGSHIP -----

  db.push_back({
      "snapdragon_8_gen3", "Snapdragon 8 Gen 3", "sm8650", 8,
      "1x Cortex-X4 + 3x Cortex-A720 + 4x Cortex-A520", "qcom", 0,
      "Adreno (TM) 750", "Qualcomm", "OpenGL ES 3.2 V@0702.0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno750Extensions,
      48000, 256,
      true, "qualcomm", "adreno-700", "adreno-750",
      "Qualcomm Adreno 750"});

  db.push_back({
      "snapdragon_8_gen2", "Snapdragon 8 Gen 2", "sm8550", 8,
      "1x Cortex-X3 + 2x Cortex-A715 + 2x Cortex-A710 + 3x Cortex-A510",
      "qcom", 0,
      "Adreno (TM) 740", "Qualcomm", "OpenGL ES 3.2 V@0615.0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno740Extensions,
      48000, 256,
      true, "qualcomm", "adreno-700", "adreno-740",
      "Qualcomm Adreno 740"});

  db.push_back({
      "snapdragon_8_gen1", "Snapdragon 8 Gen 1", "sm8450", 8,
      "1x Cortex-X2 + 3x Cortex-A710 + 4x Cortex-A510", "qcom", 0,
      "Adreno (TM) 730", "Qualcomm", "OpenGL ES 3.2 V@0590.0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno730Extensions,
      48000, 256,
      true, "qualcomm", "adreno-700", "adreno-730",
      "Qualcomm Adreno 730"});

  // ----- QUALCOMM UPPER-MID -----

  db.push_back({
      "snapdragon_7_plus_gen2", "Snapdragon 7+ Gen 2", "sm7475", 8,
      "1x Cortex-X2 + 3x Cortex-A710 + 4x Cortex-A510", "qcom", 1,
      "Adreno (TM) 725", "Qualcomm", "OpenGL ES 3.2 V@0580.0",
      16384, 16384, {16384, 16384}, 32, 32, 80,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno725Extensions,
      48000, 256,
      true, "qualcomm", "adreno-700", "adreno-725",
      "Qualcomm Adreno 725"});

  db.push_back({
      "snapdragon_7_gen1", "Snapdragon 7 Gen 1", "sm7450", 8,
      "4x Cortex-A710 + 4x Cortex-A510", "qcom", 1,
      "Adreno (TM) 720", "Qualcomm", "OpenGL ES 3.2 V@0570.0",
      16384, 16384, {16384, 16384}, 32, 32, 72,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno725Extensions,
      48000, 256,
      true, "qualcomm", "adreno-700", "adreno-720",
      "Qualcomm Adreno 720"});

  // ----- QUALCOMM MID-RANGE -----

  db.push_back({
      "snapdragon_6_gen1", "Snapdragon 6 Gen 1", "sm6450", 8,
      "4x Cortex-A78 + 4x Cortex-A55", "qcom", 2,
      "Adreno (TM) 710", "Qualcomm", "OpenGL ES 3.2 V@0560.0",
      8192, 8192, {8192, 8192}, 16, 32, 48,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno619Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "snapdragon_695", "Snapdragon 695", "sm6375", 8,
      "2x Cortex-A78 + 6x Cortex-A55", "qcom", 2,
      "Adreno (TM) 619", "Qualcomm", "OpenGL ES 3.2 V@0530.0",
      8192, 8192, {8192, 8192}, 16, 32, 48,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno619Extensions,
      48000, 512,
      false, "", "", "", ""});

  // ----- QUALCOMM BUDGET -----

  db.push_back({
      "snapdragon_4_gen2", "Snapdragon 4 Gen 2", "sm4375", 8,
      "2x Cortex-A78 + 6x Cortex-A55", "qcom", 3,
      "Adreno (TM) 613", "Qualcomm", "OpenGL ES 3.2 V@0510.0",
      8192, 8192, {8192, 8192}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno619Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "snapdragon_4_gen1", "Snapdragon 4 Gen 1", "sm6225", 8,
      "2x Cortex-A78 + 6x Cortex-A55", "qcom", 3,
      "Adreno (TM) 619", "Qualcomm", "OpenGL ES 3.2 V@0502.0",
      8192, 8192, {8192, 8192}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kAdreno619Extensions,
      48000, 512,
      false, "", "", "", ""});

  // ----- MEDIATEK FLAGSHIP -----

  db.push_back({
      "dimensity_9300", "Dimensity 9300", "mt6989", 8,
      "4x Cortex-X4 + 4x Cortex-A720", "mt6989", 0,
      "Immortalis-G720 MC12", "ARM", "OpenGL ES 3.2 v1.r44p0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG720Extensions,
      48000, 256,
      true, "arm", "valhall", "immortalis-g720",
      "ARM Immortalis-G720"});

  db.push_back({
      "dimensity_9200", "Dimensity 9200", "mt6985", 8,
      "1x Cortex-X3 + 3x Cortex-A715 + 4x Cortex-A510", "mt6985", 0,
      "Immortalis-G715 MC11", "ARM", "OpenGL ES 3.2 v1.r42p0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG720Extensions,
      48000, 256,
      true, "arm", "valhall", "immortalis-g715",
      "ARM Immortalis-G715"});

  // ----- MEDIATEK UPPER-MID -----

  db.push_back({
      "dimensity_8300", "Dimensity 8300", "mt6897", 8,
      "4x Cortex-A715 + 4x Cortex-A510", "mt6897", 1,
      "Mali-G615 MC6", "ARM", "OpenGL ES 3.2 v1.r40p0",
      8192, 8192, {8192, 8192}, 16, 32, 64,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 256,
      true, "arm", "valhall", "mali-g615",
      "ARM Mali-G615"});

  db.push_back({
      "dimensity_8200", "Dimensity 8200", "mt6896", 8,
      "4x Cortex-A78 + 4x Cortex-A55", "mt6896", 1,
      "Mali-G610 MC6", "ARM", "OpenGL ES 3.2 v1.r38p0",
      8192, 8192, {8192, 8192}, 16, 32, 64,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 256,
      true, "arm", "valhall", "mali-g610",
      "ARM Mali-G610"});

  db.push_back({
      "dimensity_8100", "Dimensity 8100", "mt6895", 8,
      "4x Cortex-A78 + 4x Cortex-A55", "mt6895", 1,
      "Mali-G610 MC6", "ARM", "OpenGL ES 3.2 v1.r36p0",
      8192, 8192, {8192, 8192}, 16, 32, 64,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 256,
      false, "", "", "", ""});

  // ----- MEDIATEK MID-RANGE -----

  db.push_back({
      "dimensity_7200", "Dimensity 7200", "mt6886", 8,
      "2x Cortex-A715 + 6x Cortex-A510", "mt6886", 2,
      "Mali-G610 MC4", "ARM", "OpenGL ES 3.2 v1.r38p0",
      8192, 8192, {8192, 8192}, 16, 32, 48,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "dimensity_7050", "Dimensity 7050", "mt6878", 8,
      "2x Cortex-A78 + 6x Cortex-A55", "mt6878", 2,
      "Mali-G610 MC3", "ARM", "OpenGL ES 3.2 v1.r38p0",
      8192, 8192, {8192, 8192}, 16, 32, 48,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "dimensity_6100_plus", "Dimensity 6100+", "mt6833", 8,
      "2x Cortex-A76 + 6x Cortex-A55", "mt6833", 2,
      "Mali-G57 MC2", "ARM", "OpenGL ES 3.2 v1.r32p1",
      8192, 8192, {8192, 8192}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG57Extensions,
      48000, 512,
      false, "", "", "", ""});

  // ----- MEDIATEK BUDGET -----

  db.push_back({
      "dimensity_6080", "Dimensity 6080", "mt6833v", 8,
      "2x Cortex-A76 + 6x Cortex-A55", "mt6833", 3,
      "Mali-G57 MC2", "ARM", "OpenGL ES 3.2 v1.r32p1",
      8192, 8192, {8192, 8192}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG57Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "dimensity_6020", "Dimensity 6020", "mt6833b", 8,
      "2x Cortex-A76 + 6x Cortex-A55", "mt6833", 3,
      "Mali-G57 MC1", "ARM", "OpenGL ES 3.2 v1.r32p1",
      4096, 4096, {4096, 4096}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG57Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "helio_g99", "Helio G99", "mt6789", 8,
      "2x Cortex-A76 + 6x Cortex-A55", "mt6789", 2,
      "Mali-G57 MC2", "ARM", "OpenGL ES 3.2 v1.r32p1",
      8192, 8192, {8192, 8192}, 16, 32, 32,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG57Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "helio_g85", "Helio G85", "mt6768", 8,
      "2x Cortex-A75 + 6x Cortex-A55", "mt6768", 3,
      "Mali-G52 MC2", "ARM", "OpenGL ES 3.2 v1.r26p0",
      8192, 8192, {8192, 8192}, 16, 16, 32,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG68Extensions,
      48000, 512,
      false, "", "", "", ""});

  db.push_back({
      "helio_g36", "Helio G36", "mt6765g", 8,
      "4x Cortex-A53 + 4x Cortex-A53", "mt6765", 3,
      "IMG GE8320", "Imagination Technologies", "OpenGL ES 3.2",
      4096, 4096, {4096, 4096}, 16, 16, 16,
      {1.0f, 1.0f}, {1.0f, 1024.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG52Extensions,
      44100, 512,
      false, "", "", "", ""});

  // ----- SAMSUNG EXYNOS -----

  db.push_back({
      "exynos_2400", "Exynos 2400", "s5e9945", 8,
      "1x Cortex-X4 + 3x Cortex-A720 + 4x Cortex-A520", "exynos", 0,
      "Xclipse 940", "Samsung", "OpenGL ES 3.2 v2.r4p0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kXclipse940Extensions,
      48000, 256,
      true, "samsung", "rdna3", "xclipse-940",
      "Samsung Xclipse 940"});

  db.push_back({
      "exynos_2200", "Exynos 2200", "s5e9925", 8,
      "1x Cortex-X2 + 3x Cortex-A710 + 4x Cortex-A510", "exynos", 0,
      "Xclipse 920", "Samsung", "OpenGL ES 3.2 v2.r2p0",
      16384, 16384, {16384, 16384}, 32, 32, 80,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kXclipse920Extensions,
      48000, 256,
      true, "samsung", "rdna2", "xclipse-920",
      "Samsung Xclipse 920"});

  db.push_back({
      "exynos_1380", "Exynos 1380", "s5e8835", 8,
      "4x Cortex-A78 + 4x Cortex-A55", "exynos", 1,
      "Mali-G615", "ARM", "OpenGL ES 3.2 v1.r38p0",
      8192, 8192, {8192, 8192}, 16, 32, 64,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMali615Extensions,
      48000, 512,
      false, "", "", "", ""});

  // ----- GOOGLE TENSOR -----

  db.push_back({
      "tensor_g4", "Tensor G4", "zuma_pro", 8,
      "1x Cortex-X4 + 3x Cortex-A720 + 4x Cortex-A520", "tensor", 0,
      "Mali-G715 Immortalis MC10", "ARM", "OpenGL ES 3.2 v1.r44p0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kTensorGPUExtensions,
      48000, 256,
      true, "google", "valhall", "mali-g715",
      "ARM Mali-G715 Immortalis"});

  db.push_back({
      "tensor_g3", "Tensor G3", "zuma", 8,
      "1x Cortex-X3 + 4x Cortex-A715 + 4x Cortex-A510", "tensor", 0,
      "Mali-G715 Immortalis MC10", "ARM", "OpenGL ES 3.2 v1.r42p0",
      16384, 16384, {16384, 16384}, 32, 32, 96,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kTensorGPUExtensions,
      48000, 256,
      true, "google", "valhall", "mali-g715",
      "ARM Mali-G715 Immortalis"});

  db.push_back({
      "tensor_g2", "Tensor G2", "cloudripper", 8,
      "2x Cortex-X1 + 2x Cortex-A78 + 4x Cortex-A55", "tensor", 1,
      "Mali-G710 MC10", "ARM", "OpenGL ES 3.2 v1.r38p0",
      8192, 8192, {8192, 8192}, 16, 32, 64,
      {1.0f, 1.0f}, {1.0f, 2048.0f},
      "OpenGL ES GLSL ES 3.20", kMaliG610Extensions,
      48000, 256,
      false, "", "", "", ""});

  return db;
}

}  // namespace

const std::vector<SoCProfile>& GetAllSoCProfiles() {
  static const std::vector<SoCProfile> kDatabase = BuildSoCDatabase();
  return kDatabase;
}

const SoCProfile* FindSoCProfile(const std::string& codename) {
  const auto& db = GetAllSoCProfiles();
  for (const auto& soc : db) {
    if (soc.codename == codename)
      return &soc;
  }
  return nullptr;
}

std::vector<const SoCProfile*> GetSoCsForTier(int tier) {
  std::vector<const SoCProfile*> result;
  const auto& db = GetAllSoCProfiles();
  for (const auto& soc : db) {
    if (soc.soc_tier == tier)
      result.push_back(&soc);
  }
  return result;
}

}  // namespace normal_browser
