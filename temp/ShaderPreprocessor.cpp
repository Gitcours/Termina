#include "ShaderPreprocessor.h"

#include <sstream>
#include <string>

static std::unordered_map<std::string, ShaderType> s_ShaderTypeMap = {
    {"vertex", ShaderType::VERTEX},
    {"pixel", ShaderType::PIXEL},
    {"fragment", ShaderType::PIXEL}, // Alias for pixel shader
    {"compute", ShaderType::COMPUTE},
    {"mesh", ShaderType::MESH},
    {"task", ShaderType::TASK},
};

ShaderPreprocessor::Result ShaderPreprocessor::Preprocess(const std::string& source)
{
    ShaderPreprocessor::Result result;
    std::stringstream processedSource;

    std::string line;
    std::stringstream sourceStream(source);
    while (std::getline(sourceStream, line)) {
        bool isCustomPragma = false;

        if (line.find("#pragma") != std::string::npos) {
            size_t typeStart = line.find(" ") + 1;
            size_t typeEnd = line.find(" ", typeStart);
            size_t nameStart = typeEnd + 1;

            std::string typeStr = line.substr(typeStart, typeEnd - typeStart);
            std::string nameStr = line.substr(nameStart);

            auto it = s_ShaderTypeMap.find(typeStr);
            if (it != s_ShaderTypeMap.end()) {
                result.EntryPoints[nameStr] = it->second;
                isCustomPragma = true; // Don't include this line in processed source
            }

            if (typeStr == "permutation") {
                size_t permStart = line.find(" ", line.find("permutation") + 11) + 1;
                std::string permStr = line.substr(permStart);
                result.Permutations.push_back(permStr);
                isCustomPragma = true; // Don't include this line in processed source
            }
        }

        if (!isCustomPragma) {
            processedSource << line << "\n";
        }
    }

    result.ProcessedSource = processedSource.str();
    return result;
}
