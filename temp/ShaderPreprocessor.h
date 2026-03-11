#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ShaderTypes.h"

class ShaderPreprocessor
{
public:
    struct Result
    {
        std::unordered_map<std::string, ShaderType> EntryPoints;
        std::vector<std::string> Permutations;
        std::string ProcessedSource; // Source with pragmas removed
    };

    static Result Preprocess(const std::string& source);
};
