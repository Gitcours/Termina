#include "ShaderFile.h"

#include "Engine/Core/FileSystem.h"
#include "Engine/Core/Logger.h"

#if defined(PLATFORM_MACOS)
    #include "Metal/Metal_ShaderConverter.h"
#endif

bool ShaderFile::Load(const std::string& path)
{
    LOG_INFO("Compiling shader file: %s", path.c_str());

    std::string source = FileSystem::ReadStringFromDisk(path);

    ShaderPreprocessor::Result preprocessResult = ShaderPreprocessor::Preprocess(source);

    // Generate all permutation combinations (power set)
    std::vector<std::vector<std::string>> allCombinations;

    // Add DEFAULT variant (empty combination)
    allCombinations.push_back({});

    // Generate all non-empty subsets
    uint64 numPermutations = preprocessResult.Permutations.size();
    for (uint64 i = 1; i < (uint64)((1 << numPermutations)); ++i) {
        std::vector<std::string> combination;
        for (uint64 j = 0; j < numPermutations; ++j) {
            if (i & (1 << j)) {
                combination.push_back(preprocessResult.Permutations[j]);
            }
        }
        allCombinations.push_back(combination);
    }

    // Compile each combination
    for (const auto& combination : allCombinations) {
        // Build variant key from combination
        std::string variantKey;
        if (combination.empty()) {
            variantKey = "DEFAULT";
        } else {
            for (const auto& perm : combination) {
                if (!variantKey.empty()) {
                    variantKey += "_";
                }
                variantKey += perm;
            }
        }

        // Compile each entry point with this combination
        for (auto& [entryPoint, type] : preprocessResult.EntryPoints) {
            ShaderCompiler::Arguments args;
            args.EntryPoint = entryPoint;
            args.Path = path;
            args.Source = preprocessResult.ProcessedSource; // Use processed source without pragmas
            args.Type = type;
            args.Defines = combination;
#if defined(PLATFORM_MACOS)
            args.PostProcessCallback = MetalShaderConverter::Convert;
#endif

            ShaderCompiler::Result compileResult = ShaderCompiler::Compile(args);
            if (!compileResult.Success) {
                return false;
            }
            Variants[variantKey].Bytecodes[type].first = entryPoint;
            Variants[variantKey].Bytecodes[type].second = std::move(compileResult.Bytecode);
            Variants[variantKey].VariantKey = variantKey;
            // Doing this so I can name all the pipelines later
        }
    }
    return true;
}

ShaderFile::Entry ShaderFile::GetDefaultEntry()
{
    return GetEntryWithVariants({ "DEFAULT" });
}

ShaderFile::Entry ShaderFile::GetEntryWithVariants(const std::vector<std::string>& variants)
{
    std::string variantKey;
    for (const auto& variant : variants) {
        if (!variantKey.empty()) {
            variantKey += "_";
        }
        variantKey += variant;
    }

    auto it = Variants.find(variantKey);
    if (it != Variants.end()) {
        return it->second;
    }

    return Entry{};
}
