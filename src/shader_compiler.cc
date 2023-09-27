#include "shader_compiler.hh"

static std::map<SHADER_TYPE, shaderc_shader_kind> shaderTypeMapping = {
    {VERT, shaderc_shader_kind::shaderc_vertex_shader},
    {FRAG, shaderc_shader_kind::shaderc_fragment_shader}};

std::vector<uint32_t> ShaderCompiler::compileAssembly(const std::filesystem::path& filePath,
                                                      SHADER_TYPE type) {
    if (!std::filesystem::is_regular_file(filePath)) {
        std::cerr << filePath << " is not a valid file path\n";
        return std::vector<uint32_t>();
    }
    std::ifstream ifs(filePath);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
        content.c_str(), shaderTypeMapping[type], filePath.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }

    return {module.cbegin(), module.cend()};
}