#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <iostream>
#include <map>

enum SHADER_TYPE { VERT, FRAG, UNKNOWN };

class ShaderCompiler {
public:
    static std::vector<uint32_t> compileAssembly(const std::filesystem::path& filePath,
                                                 SHADER_TYPE type);
};