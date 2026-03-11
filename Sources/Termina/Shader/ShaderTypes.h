#pragma once

#include <Termina/Core/Common.hpp>

#include <vector>
#include <string>

enum class ShaderType
{
    VERTEX,
    PIXEL,
    COMPUTE,
    MESH,
    TASK
};

struct ShaderModule
{
    ShaderType Type = ShaderType::VERTEX;
    std::string EntryPoint = "";
    std::vector<uint8> Bytecode = {};
};
