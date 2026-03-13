#pragma once

#include <Termina/Asset/AssetSystem.hpp>
#include <Termina/Asset/IAssetLoader.hpp>
#include "TextureAsset.hpp"

namespace Termina {

    /// Loads textures from disk using stb_image (forced to 4 channels / RGBA8_SRGB).
    /// Requires access to the RendererSystem to create GPU textures and queue uploads.
    class TextureLoader : public IAssetLoader<TextureAsset>
    {
    public:
        TextureAsset* LoadFromDisk(const std::string& path) override;

        /// Not yet implemented — package loading will be added with the asset package system.
        TextureAsset* LoadFromPackage(const uint8* data, size_t size) override;

        /// Not yet implemented — package export will be added with the asset package system.
        std::vector<uint8> ExportToPackage(const TextureAsset* asset) override;

        /// Creates a 1x1 opaque white texture for use as a fallback.
        TextureAsset* CreateDefaultAsset();

        /// Creates a 1x1 opaque white texture and registers it as the default
        /// TextureAsset in the given AssetSystem.
        void RegisterDefault(AssetSystem& assets);
    };

} // namespace Termina
