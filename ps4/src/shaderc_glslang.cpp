// PS4: shaderc::Compiler (ps4/shaderc/libshaderc/include/shaderc/shaderc.hpp) over glslang 15 (the PS4 SDK's).
#include <shaderc/shaderc.hpp>

#include <mutex>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

namespace shaderc {

Compiler::Compiler() {
    static std::once_flag once;
    std::call_once(once, [] { glslang::InitializeProcess(); });
}

SpvCompilationResult Compiler::CompileGlslToSpv(const std::string& source, shaderc_shader_kind kind, const char* name,
                                                const CompileOptions& options) const {
    SpvCompilationResult result;
    const EShLanguage stage = kind == shaderc_vertex_shader ? EShLangVertex : EShLangFragment;
    glslang::TShader shader(stage);
    const char* text = source.c_str();
    const int length = static_cast<int>(source.size());
    const char* names[] = { name ? name : "shader" };
    shader.setStringsWithLengthsAndNames(&text, &length, names, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    const bool vulkan12 = options.mVersion >= shaderc_env_version_vulkan_1_2;
    shader.setEnvClient(glslang::EShClientVulkan, vulkan12 ? glslang::EShTargetVulkan_1_2 : glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, vulkan12 ? glslang::EShTargetSpv_1_5 : glslang::EShTargetSpv_1_3);
    const EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages)) {
        result.mStatus = shaderc_compilation_status_compilation_error;
        result.mMessage = std::string(shader.getInfoLog()) + shader.getInfoDebugLog();
        return result;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        result.mStatus = shaderc_compilation_status_compilation_error;
        result.mMessage = std::string(program.getInfoLog()) + program.getInfoDebugLog();
        return result;
    }
    glslang::SpvOptions spv;
    spv.disableOptimizer = true;
    spv.validate = false;
    spv.generateDebugInfo = false;
    std::vector<unsigned int> words;
    glslang::GlslangToSpv(*program.getIntermediate(stage), words, &spv);
    if (words.empty()) {
        result.mStatus = shaderc_compilation_status_internal_error;
        result.mMessage = "glslang produced no SPIR-V";
        return result;
    }
    result.mWords.assign(words.begin(), words.end());
    result.mStatus = shaderc_compilation_status_success;
    return result;
}

} // namespace shaderc
