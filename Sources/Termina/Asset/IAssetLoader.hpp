#pragma once

#include <Termina/Core/Common.hpp>

#include <string>
#include <vector>

namespace Termina {

    /// Interface for loading assets of type T.
    /// Implement one per asset type and register it with AssetSystem.
    /// LoadFromPackage receives raw (decompressed) bytes from a .pak blob.
    template<typename T>
    struct IAssetLoader
    {
        virtual ~IAssetLoader() = default;
        virtual T* LoadFromDisk(const std::string& path) = 0;
        virtual T* LoadFromPackage(const uint8* data, size_t size) = 0;
        virtual std::vector<uint8> ExportToPackage(const T* asset) = 0;
    };

} // namespace Termina
