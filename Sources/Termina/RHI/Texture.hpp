#pragma once

#include <Termina/Core/Common.hpp>

#include "Resource.hpp"

#include <string>
#include <vector>

namespace Termina {
    enum class TextureFormat
    {
        UNDEFINED,
        R8_UNORM,
        RGBA8_UNORM,
        RGBA8_SRGB,
        BGRA8_UNORM,
        D32_FLOAT,
        RGBA16_FLOAT,

        // Available everywhere except iOS
        BC7_UNORM,
        BC7_SRGB,

        /// Only supported on macOS
        ASTC6X6_UNORM,
        ASTC6X6_SRGB
    };

    enum class TextureUsage
    {
        RENDER_TARGET = BIT(0),
        DEPTH_TARGET = BIT(1),
        SHADER_READ = BIT(2),
        SHADER_WRITE = BIT(3)
    };
    ENUM_CLASS_FLAG_OPERATORS(TextureUsage);

    enum class TextureLayout
    {
        UNDEFINED,
        GENERAL,
        READ_ONLY,
        COLOR_ATTACHMENT,
        DEPTH_ATTACHMENT,
        DEPTH_STENCIL_READ,
        DEPTH_STENCIL_WRITE,
        TRANSFER_SRC,
        TRANSFER_DST,
        PRESENT
    };

    struct TextureDesc
    {
        uint32 Width = 1;
        uint32 Height = 1;
        uint32 Depth = 1;
        uint32 ArrayLayers = 1;
        uint32 MipLevels = 1;
        TextureFormat Format = TextureFormat::RGBA8_UNORM;
        TextureUsage Usage = TextureUsage::SHADER_READ;
        bool IsCubeMap = false;

        TextureDesc& SetSize(uint32 width, uint32 height, uint32 depth = 1)
        {
            Width = width;
            Height = height;
            Depth = depth;
            return *this;
        }

        TextureDesc& SetArrayLayers(uint32 layers)
        {
            ArrayLayers = layers;
            return *this;
        }

        TextureDesc& SetMipLevels(uint32 mipLevels)
        {
            MipLevels = mipLevels;
            return *this;
        }

        TextureDesc& SetFormat(TextureFormat format)
        {
            Format = format;
            return *this;
        }

        TextureDesc& SetUsage(TextureUsage usage)
        {
            Usage = usage;
            return *this;
        }
    };

    class RendererTexture : public RendererResource
    {
    public:
        virtual ~RendererTexture() = default;

        virtual void SetName(const std::string& name) = 0;
        virtual void Resize(uint32 width, uint32 height) = 0;

        const TextureDesc& GetDesc() const { return m_Desc; }

        TextureLayout GetCurrentLayout() const { return m_CurrentLayout; }
        void SetCurrentLayout(TextureLayout layout) { m_CurrentLayout = layout; }
    protected:
        TextureDesc m_Desc;

        TextureLayout m_CurrentLayout = TextureLayout::UNDEFINED;
    };

    inline uint32 TextureFormatBPP(TextureFormat format)
    {
        switch (format) {
        case TextureFormat::R8_UNORM:
            return 1;
        case TextureFormat::RGBA8_UNORM:
        case TextureFormat::RGBA8_SRGB:
        case TextureFormat::BGRA8_UNORM:
            return 4;
        case TextureFormat::D32_FLOAT:
            return 4;
        case TextureFormat::BC7_UNORM:
        case TextureFormat::BC7_SRGB:
            return 16;
        case TextureFormat::ASTC6X6_UNORM:
        case TextureFormat::ASTC6X6_SRGB:
            return 16;
        default:
            return 0;
        }
    }

    inline bool TextureFormatIsCompressed(TextureFormat format)
    {
        switch (format) {
        case TextureFormat::BC7_UNORM:
        case TextureFormat::BC7_SRGB:
        case TextureFormat::ASTC6X6_UNORM:
        case TextureFormat::ASTC6X6_SRGB:
            return true;
        default:
            return false;
        }
    }

    inline bool TextureFormatIsSRGB(TextureFormat format)
    {
        switch (format) {
        case TextureFormat::RGBA8_SRGB:
        case TextureFormat::BC7_SRGB:
        case TextureFormat::ASTC6X6_SRGB:
            return true;
        default:
            return false;
        }
    }

    inline bool TextureFormatIsDepth(TextureFormat format)
    {
        switch (format) {
        case TextureFormat::D32_FLOAT:
            return true;
        default:
            return false;
        }
    }
}
