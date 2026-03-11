#pragma once

#include <string>
#include <vector>
#include <functional>

#include <Termina/Core/Common.hpp>

#include "ShaderTypes.h"

class ShaderCompiler
{
public:
    struct Arguments
    {
        std::string Path;
        std::string Source;
        std::string EntryPoint;
        ShaderType Type;
        std::vector<std::string> Defines;

        // Example: PostProcessCallback = MetalShaderConverter::ConvertDXILToMetallib
        std::function<std::vector<uint8>(std::vector<uint8>, std::string)> PostProcessCallback;
    };

    struct Result
    {
        bool Success;
        std::vector<uint8> Bytecode;
        std::string EntryPoint;
        ShaderType Type;
    };

    static Result Compile(const Arguments& args);
};
