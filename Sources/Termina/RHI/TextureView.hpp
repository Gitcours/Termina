#pragma once

#include <Termina/Core/Common.hpp>

#include "Texture.hpp"

#include <string>

namespace Termina {
    enum class TextureViewType
    {
        RENDER_TARGET,
        DEPTH_TARGET,
        SHADER_READ,
        SHADER_WRITE
    };
    
    enum class TextureViewDimension
    {
        TEXTURE_2D,
        TEXTURE_2D_ARRAY,
        TEXTURE_3D,
        TEXTURE_CUBE
    };
    
    constexpr uint32 VIEW_ALL_MIP_LEVELS = 0xFFFFFFFF;
    constexpr uint32 VIEW_ALL_ARRAY_LAYERS = 0xFFFFFFFF;
    
    struct TextureViewDesc
    {
        std::string Name;
        RendererTexture* Texture;
        TextureViewType Type = TextureViewType::SHADER_READ;
        TextureViewDimension Dimension = TextureViewDimension::TEXTURE_2D;
        TextureFormat Format = TextureFormat::UNDEFINED;
    
        uint32 MipStart = 0;
        uint32 MipEnd = VIEW_ALL_MIP_LEVELS;
        uint32 ArrayLayerStart = 0;
        uint32 ArrayLayerEnd = VIEW_ALL_ARRAY_LAYERS;
    
        TextureViewDesc& SetMipRange(uint32 mipStart, uint32 mipEnd = VIEW_ALL_MIP_LEVELS)
        {
            MipStart = mipStart;
            MipEnd = mipEnd;
            return *this;
        }
    
        TextureViewDesc& SetArrayLayerRange(uint32 layerStart, uint32 layerEnd = VIEW_ALL_ARRAY_LAYERS)
        {
            ArrayLayerStart = layerStart;
            ArrayLayerEnd = layerEnd;
            return *this;
        }
    
        TextureViewDesc& SetType(TextureViewType type)
        {
            Type = type;
            return *this;
        }
    
        TextureViewDesc& SetDimension(TextureViewDimension dimension)
        {
            Dimension = dimension;
            return *this;
        }
    
        TextureViewDesc& SetFormat(TextureFormat format)
        {
            Format = format;
            return *this;
        }
    
        TextureViewDesc& SetTexture(RendererTexture* texture)
        {
            Texture = texture;
            return *this;
        }
    
        static TextureViewDesc CreateDefault(RendererTexture* texture, TextureViewType type, TextureViewDimension dimension)
        {
            TextureViewDesc desc = {};
            desc.Type = type;
            desc.Texture = texture;
            desc.Dimension = dimension;
            desc.Format = texture->GetDesc().Format;
            desc.MipStart = 0;
            desc.MipEnd = VIEW_ALL_MIP_LEVELS;
            desc.ArrayLayerStart = 0;
            desc.ArrayLayerEnd = VIEW_ALL_ARRAY_LAYERS;
            return desc;
        }
    };
    
    class TextureView
    {
    public:
        virtual ~TextureView() = default;
    
        virtual int32 GetBindlessIndex() = 0;
    
        const TextureViewDesc& GetDesc() const { return m_Desc; }
    protected:
        TextureViewDesc m_Desc;
    };
}
