// Copyright 2024 Normal Browser Authors. All rights reserved.
// gpu_database.h — Maps SoC codenames to exact GPU specs.

#ifndef DEVICE_PROFILE_GENERATOR_GPU_DATABASE_H_
#define DEVICE_PROFILE_GENERATOR_GPU_DATABASE_H_

#include <string>
#include <vector>

#include "device_profile_generator/device_profile.h"

namespace normal_browser {

struct SoCProfile {
  std::string codename;           // "snapdragon_8_gen3"
  std::string display_name;       // "Snapdragon 8 Gen 3"
  std::string soc_codename;       // "sm8650"
  int cpu_cores;                  // 8
  std::string cpu_architecture;   // "1x Cortex-X4 + 3x A720 + 4x A520"
  std::string hardware_string;    // "qcom" / "mt6789" / "exynos" / "tensor"
  int soc_tier;                   // 0=flagship, 1=upper-mid, 2=mid, 3=budget

  // GPU specs (must be exact)
  std::string gl_renderer;
  std::string gl_vendor;
  std::string gl_version;
  int gl_max_texture_size;
  int gl_max_renderbuffer_size;
  int gl_max_viewport_dims[2];
  int gl_max_vertex_attribs;
  int gl_max_varying_vectors;
  int gl_max_combined_texture_units;
  float gl_aliased_line_width[2];
  float gl_aliased_point_size[2];
  std::string gl_shading_language_version;
  std::vector<std::string> gl_extensions;

  // Audio
  int preferred_sample_rate;
  int preferred_buffer_size;

  // WebGPU
  bool supports_webgpu;
  std::string webgpu_vendor;
  std::string webgpu_arch;
  std::string webgpu_device;
  std::string webgpu_description;
};

// Get the global SoC database
const std::vector<SoCProfile>& GetAllSoCProfiles();

// Lookup a SoC by codename
const SoCProfile* FindSoCProfile(const std::string& codename);

// Get SoCs compatible with a given tier
std::vector<const SoCProfile*> GetSoCsForTier(int tier);

}  // namespace normal_browser

#endif  // DEVICE_PROFILE_GENERATOR_GPU_DATABASE_H_
