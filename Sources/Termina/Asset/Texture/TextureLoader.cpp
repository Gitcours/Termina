#include "TextureLoader.hpp"

#include <Termina/Core/Application.hpp>
#include <Termina/Renderer/Renderer.hpp>

#include <stb/stb_image.h>

namespace Termina {

    TextureAsset* TextureLoader::LoadFromDisk(const std::string& path)
    {
        int width, height, channels;
        stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels)
            return nullptr;

        const uint64 dataSize = static_cast<uint64>(width) * height * 4;

        auto* renderer = Application::GetSystem<RendererSystem>();

        TextureDesc desc;
        desc.SetSize(static_cast<uint32>(width), static_cast<uint32>(height))
            .SetFormat(TextureFormat::RGBA8_SRGB)
            .SetUsage(TextureUsage::SHADER_READ);

        RendererTexture* texture = renderer->GetDevice()->CreateTexture(desc);

        TextureUploadDesc uploadDesc;
        uploadDesc.Width  = static_cast<uint32>(width);
        uploadDesc.Height = static_cast<uint32>(height);
        uploadDesc.Format = TextureFormat::RGBA8_SRGB;

        renderer->GetGPUUploader()->QueueTextureUpload(texture, pixels, dataSize, uploadDesc);

        stbi_image_free(pixels);

        return new TextureAsset(texture);
    }

    TextureAsset* TextureLoader::LoadFromPackage(const uint8* /*data*/, size_t /*size*/)
    {
        return nullptr;
    }

    std::vector<uint8> TextureLoader::ExportToPackage(const TextureAsset* /*asset*/)
    {
        return {};
    }

    TextureAsset* TextureLoader::CreateDefaultAsset()
    {
        auto* renderer = Application::GetSystem<RendererSystem>();

        TextureDesc desc;
        desc.SetSize(1, 1)
            .SetFormat(TextureFormat::RGBA8_SRGB)
            .SetUsage(TextureUsage::SHADER_READ);

        RendererTexture* texture = renderer->GetDevice()->CreateTexture(desc);

        constexpr uint8 white[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

        TextureUploadDesc uploadDesc;
        uploadDesc.Width  = 1;
        uploadDesc.Height = 1;
        uploadDesc.Format = TextureFormat::RGBA8_SRGB;

        renderer->GetGPUUploader()->QueueTextureUpload(texture, white, sizeof(white), uploadDesc);

        return new TextureAsset(texture);
    }

    void TextureLoader::RegisterDefault(AssetSystem& assets)
    {
        assets.SetDefault<TextureAsset>(CreateDefaultAsset());
    }

} // namespace Termina
