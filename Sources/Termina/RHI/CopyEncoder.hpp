#pragma once

#include <Termina/Core/Common.hpp>

#include "Buffer.hpp"
#include "Texture.hpp"

namespace Termina {
    struct BufferTextureCopyRegion
    {
        uint64 BufferOffset;
        uint32 BufferRowLength;
        uint32 BufferImageHeight;
        uint32 MipLevel;
        uint32 BaseArrayLayer;
        uint32 LayerCount;
        uint32 TextureOffsetX;
        uint32 TextureOffsetY;
        uint32 TextureOffsetZ;
        uint32 TextureExtentWidth;
        uint32 TextureExtentHeight;
        uint32 TextureExtentDepth;
    
        BufferTextureCopyRegion& SetBufferOffset(uint64 offset)
        {
            BufferOffset = offset;
            return *this;
        }
    
        BufferTextureCopyRegion& SetBufferRowLength(uint32 rowLength)
        {
            BufferRowLength = rowLength;
            return *this;
        }
    
        BufferTextureCopyRegion& SetBufferImageHeight(uint32 imageHeight)
        {
            BufferImageHeight = imageHeight;
            return *this;
        }
    
        BufferTextureCopyRegion& SetTextureOffset(uint32 x, uint32 y, uint32 z)
        {
            TextureOffsetX = x;
            TextureOffsetY = y;
            TextureOffsetZ = z;
            return *this;
        }
    
        BufferTextureCopyRegion& SetTextureExtent(uint32 width, uint32 height, uint32 depth)
        {
            TextureExtentWidth = width;
            TextureExtentHeight = height;
            TextureExtentDepth = depth;
            return *this;
        }
    
        BufferTextureCopyRegion& SetMipLevel(uint32 mipLevel)
        {
            MipLevel = mipLevel;
            return *this;
        }
    
        BufferTextureCopyRegion& SetBaseArrayLayer(uint32 baseArrayLayer)
        {
            BaseArrayLayer = baseArrayLayer;
            return *this;
        }
    
        BufferTextureCopyRegion& SetLayerCount(uint32 layerCount)
        {
            LayerCount = layerCount;
            return *this;
        }
    };
    
    struct TextureCopyRegion
    {
        uint32 SrcMipLevel;
        uint32 SrcBaseArrayLayer;
        uint32 SrcOffsetX;
        uint32 SrcOffsetY;
        uint32 SrcOffsetZ;
        uint32 DstMipLevel;
        uint32 DstBaseArrayLayer;
        uint32 DstOffsetX;
        uint32 DstOffsetY;
        uint32 DstOffsetZ;
        uint32 ExtentWidth;
        uint32 ExtentHeight;
        uint32 ExtentDepth;
    
        TextureCopyRegion& SetSrcOffset(uint32 x, uint32 y, uint32 z)
        {
            SrcOffsetX = x;
            SrcOffsetY = y;
            SrcOffsetZ = z;
            return *this;
        }
    
        TextureCopyRegion& SetDstOffset(uint32 x, uint32 y, uint32 z)
        {
            DstOffsetX = x;
            DstOffsetY = y;
            DstOffsetZ = z;
            return *this;
        }
    
        TextureCopyRegion& SetExtent(uint32 width, uint32 height, uint32 depth)
        {
            ExtentWidth = width;
            ExtentHeight = height;
            ExtentDepth = depth;
            return *this;
        }
    
        TextureCopyRegion& SetSrcMipLevel(uint32 mipLevel)
        {
            SrcMipLevel = mipLevel;
            return *this;
        }
    
        TextureCopyRegion& SetDstMipLevel(uint32 mipLevel)
        {
            DstMipLevel = mipLevel;
            return *this;
        }
    
        TextureCopyRegion& SetSrcBaseArrayLayer(uint32 baseArrayLayer)
        {
            SrcBaseArrayLayer = baseArrayLayer;
            return *this;
        }
    
        TextureCopyRegion& SetDstBaseArrayLayer(uint32 baseArrayLayer)
        {
            DstBaseArrayLayer = baseArrayLayer;
            return *this;
        }
    };
    
    class CopyEncoder
    {
    public:
        virtual ~CopyEncoder() = default;
    
        virtual void CopyBufferToBuffer(RendererBuffer* srcBuffer, uint64 srcOffset,
                                        RendererBuffer* dstBuffer, uint64 dstOffset,
                                        uint64 size) = 0;
        virtual void CopyBufferToBuffer(RendererBuffer* srcBuffer,
                                        RendererBuffer* dstBuffer) = 0;
        virtual void CopyBufferToTexture(RendererBuffer* srcBuffer,
                                         RendererTexture* dstTexture,
                                         const BufferTextureCopyRegion& region) = 0;
        virtual void CopyTextureToBuffer(RendererTexture* srcTexture,
                                         RendererBuffer* dstBuffer,
                                         const BufferTextureCopyRegion& region) = 0;
        virtual void CopyTextureToTexture(RendererTexture* srcTexture,
                                          RendererTexture* dstTexture,
                                          const TextureCopyRegion& region) = 0;
        virtual void CopyTextureToTexture(RendererTexture* srcTexture,
                                          RendererTexture* dstTexture) = 0;
        virtual void ResetBuffer(RendererBuffer* buffer, uint64 offset, uint64 size) = 0;
        virtual void End() = 0;
    };
}
