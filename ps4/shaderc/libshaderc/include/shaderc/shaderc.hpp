// PS4: the part of shaderc's C++ API libultraship's Vulkan renderer uses (gfx_vulkan.cpp, CompileGlslToSpirv),
// over glslang, which the PS4 SDK (SMW's sdk-1 sysroot) already has built. GLSL to SPIR-V for Vulkan 1.1; no SPIR-V
// optimizer pass (RADV's ACO optimizes the shaders anyway), so SPIRV-Tools is not needed.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

typedef enum {
    shaderc_target_env_vulkan,
    shaderc_target_env_opengl,
    shaderc_target_env_default = shaderc_target_env_vulkan
} shaderc_target_env;

typedef enum {
    shaderc_env_version_vulkan_1_0 = ((1u << 22)),
    shaderc_env_version_vulkan_1_1 = ((1u << 22) | (1 << 12)),
    shaderc_env_version_vulkan_1_2 = ((1u << 22) | (2 << 12)),
} shaderc_env_version;

typedef enum {
    shaderc_optimization_level_zero,
    shaderc_optimization_level_size,
    shaderc_optimization_level_performance,
} shaderc_optimization_level;

typedef enum {
    shaderc_vertex_shader,
    shaderc_fragment_shader,
} shaderc_shader_kind;

typedef enum {
    shaderc_compilation_status_success = 0,
    shaderc_compilation_status_invalid_stage = 1,
    shaderc_compilation_status_compilation_error = 2,
    shaderc_compilation_status_internal_error = 3,
} shaderc_compilation_status;

namespace shaderc {

class CompileOptions {
  public:
    void SetTargetEnvironment(shaderc_target_env env, uint32_t version) {
        mEnv = env;
        mVersion = version;
    }
    void SetOptimizationLevel(shaderc_optimization_level level) {
        mLevel = level;
    }
    shaderc_target_env mEnv = shaderc_target_env_vulkan;
    uint32_t mVersion = shaderc_env_version_vulkan_1_1;
    shaderc_optimization_level mLevel = shaderc_optimization_level_zero;
};

class SpvCompilationResult {
  public:
    using const_iterator = std::vector<uint32_t>::const_iterator;
    shaderc_compilation_status GetCompilationStatus() const {
        return mStatus;
    }
    const std::string& GetErrorMessage() const {
        return mMessage;
    }
    const_iterator cbegin() const {
        return mWords.cbegin();
    }
    const_iterator cend() const {
        return mWords.cend();
    }
    const_iterator begin() const {
        return mWords.cbegin();
    }
    const_iterator end() const {
        return mWords.cend();
    }
    shaderc_compilation_status mStatus = shaderc_compilation_status_internal_error;
    std::string mMessage;
    std::vector<uint32_t> mWords;
};

class Compiler {
  public:
    Compiler();
    bool IsValid() const {
        return true;
    }
    SpvCompilationResult CompileGlslToSpv(const std::string& source, shaderc_shader_kind kind, const char* name,
                                          const CompileOptions& options) const;
};

} // namespace shaderc
