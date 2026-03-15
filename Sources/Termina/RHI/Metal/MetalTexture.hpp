#pragma once

#include <Termina/RHI/Texture.hpp>

#include <Metal/Metal.h>

namespace Termina {
    class MetalDevice;

    class MetalTexture : public RendererTexture
    {
    public:
        MetalTexture() = default;
        MetalTexture(MetalDevice* device, const TextureDesc& desc);
        ~MetalTexture() override;

        void SetName(const std::string& name) override;
        void Resize(uint32 width, uint32 height) override;

        id<MTLTexture> GetTexture() const { return m_Texture; }

    private:
        friend class MetalSurface;

        MetalDevice* m_Parent;
        bool m_ShouldDestroy = true;

        id<MTLTexture> m_Texture;
    };

    inline MTLPixelFormat ConvertTextureFormatToMetal(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::UNDEFINED:     return MTLPixelFormatInvalid;
            case TextureFormat::R8_UNORM:      return MTLPixelFormatR8Unorm;
            case TextureFormat::RGBA8_UNORM:   return MTLPixelFormatRGBA8Unorm;
            case TextureFormat::RGBA8_SRGB:    return MTLPixelFormatRGBA8Unorm_sRGB;
            case TextureFormat::BGRA8_UNORM:   return MTLPixelFormatBGRA8Unorm;
            case TextureFormat::D32_FLOAT:     return MTLPixelFormatDepth32Float;
            case TextureFormat::RGBA16_FLOAT:  return MTLPixelFormatRGBA16Float;
            case TextureFormat::BC7_UNORM:     return MTLPixelFormatBC7_RGBAUnorm;
            case TextureFormat::BC7_SRGB:      return MTLPixelFormatBC7_RGBAUnorm_sRGB;
            case TextureFormat::ASTC6X6_UNORM: return MTLPixelFormatASTC_6x6_LDR;
            case TextureFormat::ASTC6X6_SRGB:  return MTLPixelFormatASTC_6x6_sRGB;
        }
    }

    inline MTLTextureUsage ConvertTextureUsageToMetal(TextureUsage usage)
    {
        MTLTextureUsage flags = 0;
        if (Any(usage, TextureUsage::RENDER_TARGET))
            flags |= MTLTextureUsageRenderTarget;
        if (Any(usage, TextureUsage::DEPTH_TARGET))
            flags |= MTLTextureUsageRenderTarget;
        if (Any(usage, TextureUsage::SHADER_READ))
            flags |= MTLTextureUsageShaderRead;
        if (Any(usage, TextureUsage::SHADER_WRITE))
            flags |= MTLTextureUsageShaderWrite;
        return flags;
    }

    inline MTLTextureType ConvertTextureTypeToMetal(const TextureDesc& desc)
    {
        if (desc.IsCubeMap) return MTLTextureTypeCube;
        if (desc.Depth > 1) return MTLTextureType3D;
        if (desc.ArrayLayers > 1) return MTLTextureType2DArray;
        return MTLTextureType2D;
    }
} // namespace Termina
