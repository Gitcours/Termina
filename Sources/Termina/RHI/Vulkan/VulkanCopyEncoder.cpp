#include "VulkanCopyEncoder.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"

namespace Termina {
    VulkanCopyEncoder::VulkanCopyEncoder(VulkanRenderContext* context, const std::string& name)
        : m_Context(context)
    {
        context->PushMarker(name.empty() ? "Copy Pass" : name.c_str());
    }
    
    void VulkanCopyEncoder::CopyBufferToBuffer(RendererBuffer* srcBuffer, uint64 srcOffset, RendererBuffer* dstBuffer, uint64 dstOffset, uint64 size)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        vk::Buffer srcVkBuffer = static_cast<VulkanBuffer*>(srcBuffer)->GetVulkanBuffer();
        vk::Buffer dstVkBuffer = static_cast<VulkanBuffer*>(dstBuffer)->GetVulkanBuffer();
    
        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;
    
        cmdBuffer.copyBuffer(srcVkBuffer, dstVkBuffer, copyRegion);
    }
    
    void VulkanCopyEncoder::CopyBufferToBuffer(RendererBuffer* srcBuffer, RendererBuffer* dstBuffer)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        vk::Buffer srcVkBuffer = static_cast<VulkanBuffer*>(srcBuffer)->GetVulkanBuffer();
        vk::Buffer dstVkBuffer = static_cast<VulkanBuffer*>(dstBuffer)->GetVulkanBuffer();
    
        vk::BufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = srcBuffer->GetSize();
    
        cmdBuffer.copyBuffer(srcVkBuffer, dstVkBuffer, copyRegion);
    }
    
    void VulkanCopyEncoder::CopyBufferToTexture(RendererBuffer* srcBuffer, RendererTexture* dstTexture, const BufferTextureCopyRegion& region)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        vk::Buffer srcVkBuffer = static_cast<VulkanBuffer*>(srcBuffer)->GetVulkanBuffer();
        vk::Image dstVkImage = static_cast<VulkanTexture*>(dstTexture)->GetImage();
    
        vk::BufferImageCopy copyRegion{};
        copyRegion.bufferOffset = region.BufferOffset;
        copyRegion.bufferRowLength = region.BufferRowLength;
        copyRegion.bufferImageHeight = region.BufferImageHeight;
        copyRegion.imageSubresource.aspectMask = TextureFormatIsDepth(dstTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = region.MipLevel;
        copyRegion.imageSubresource.baseArrayLayer = region.BaseArrayLayer;
        copyRegion.imageSubresource.layerCount = region.LayerCount;
        copyRegion.imageOffset = vk::Offset3D{
            static_cast<int32>(region.TextureOffsetX),
            static_cast<int32>(region.TextureOffsetY),
            static_cast<int32>(region.TextureOffsetZ)
        };
        copyRegion.imageExtent = vk::Extent3D{
            static_cast<uint32>(region.TextureExtentWidth),
            static_cast<uint32>(region.TextureExtentHeight),
            static_cast<uint32>(region.TextureExtentDepth)
        };
    
        cmdBuffer.copyBufferToImage(
            srcVkBuffer,
            dstVkImage,
            vk::ImageLayout::eTransferDstOptimal,
            copyRegion
        );
    }
    
    void VulkanCopyEncoder::CopyTextureToBuffer(RendererTexture* srcTexture, RendererBuffer* dstBuffer, const BufferTextureCopyRegion& region)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        vk::Image srcVkImage = static_cast<VulkanTexture*>(srcTexture)->GetImage();
        vk::Buffer dstVkBuffer = static_cast<VulkanBuffer*>(dstBuffer)->GetVulkanBuffer();
    
        vk::BufferImageCopy copyRegion{};
        copyRegion.bufferOffset = region.BufferOffset;
        copyRegion.bufferRowLength = region.BufferRowLength;
        copyRegion.bufferImageHeight = region.BufferImageHeight;
        copyRegion.imageSubresource.aspectMask = TextureFormatIsDepth(srcTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = region.MipLevel;
        copyRegion.imageSubresource.baseArrayLayer = region.BaseArrayLayer;
        copyRegion.imageSubresource.layerCount = region.LayerCount;
        copyRegion.imageOffset = vk::Offset3D{
            static_cast<int32>(region.TextureOffsetX),
            static_cast<int32>(region.TextureOffsetY),
            static_cast<int32>(region.TextureOffsetZ)
        };
        copyRegion.imageExtent = vk::Extent3D{
            static_cast<uint32>(region.TextureExtentWidth),
            static_cast<uint32>(region.TextureExtentHeight),
            static_cast<uint32>(region.TextureExtentDepth)
        };
    
        cmdBuffer.copyImageToBuffer(
            srcVkImage,
            vk::ImageLayout::eTransferSrcOptimal,
            dstVkBuffer,
            copyRegion
        );
    }
    
    void VulkanCopyEncoder::CopyTextureToTexture(RendererTexture* srcTexture, RendererTexture* dstTexture, const TextureCopyRegion& region)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        vk::Image srcVkImage = static_cast<VulkanTexture*>(srcTexture)->GetImage();
        vk::Image dstVkImage = static_cast<VulkanTexture*>(dstTexture)->GetImage();
    
        vk::ImageCopy copyRegion{};
        copyRegion.srcSubresource.aspectMask = TextureFormatIsDepth(srcTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.srcSubresource.mipLevel = region.SrcMipLevel;
        copyRegion.srcSubresource.baseArrayLayer = region.SrcBaseArrayLayer;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = vk::Offset3D{
            static_cast<int32>(region.SrcOffsetX),
            static_cast<int32>(region.SrcOffsetY),
            static_cast<int32>(region.SrcOffsetZ)
        };
        copyRegion.dstSubresource.aspectMask = TextureFormatIsDepth(dstTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.dstSubresource.mipLevel = region.DstMipLevel;
        copyRegion.dstSubresource.baseArrayLayer = region.DstBaseArrayLayer;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = vk::Offset3D{
            static_cast<int32>(region.DstOffsetX),
            static_cast<int32>(region.DstOffsetY),
            static_cast<int32>(region.DstOffsetZ)
        };
        copyRegion.extent = vk::Extent3D{
            static_cast<uint32>(region.ExtentWidth),
            static_cast<uint32>(region.ExtentHeight),
            static_cast<uint32>(region.ExtentDepth)
        };
        
        cmdBuffer.copyImage(
            srcVkImage, vk::ImageLayout::eTransferSrcOptimal,
            dstVkImage, vk::ImageLayout::eTransferDstOptimal,
            copyRegion
        );
    }
    
    void VulkanCopyEncoder::CopyTextureToTexture(RendererTexture* srcTexture, RendererTexture* dstTexture)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
        VulkanTexture* srcVkTexture = static_cast<VulkanTexture*>(srcTexture);
        VulkanTexture* dstVkTexture = static_cast<VulkanTexture*>(dstTexture);
    
        vk::ImageCopy copyRegion{};
        copyRegion.srcSubresource.aspectMask = TextureFormatIsDepth(srcTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = vk::Offset3D{0, 0, 0};
        copyRegion.dstSubresource.aspectMask = TextureFormatIsDepth(dstTexture->GetDesc().Format) ?
            vk::ImageAspectFlagBits::eDepth :
            vk::ImageAspectFlagBits::eColor;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.baseArrayLayer = 0;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = vk::Offset3D{0, 0, 0};
        copyRegion.extent = vk::Extent3D{
            static_cast<uint32>(srcTexture->GetDesc().Width),
            static_cast<uint32>(srcTexture->GetDesc().Height),
            1
        };
    
        cmdBuffer.copyImage(
            srcVkTexture->GetImage(), vk::ImageLayout::eTransferSrcOptimal,
            dstVkTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal,
            copyRegion
        );
    }
    
    void VulkanCopyEncoder::ResetBuffer(RendererBuffer* buffer, uint64 offset, uint64 size)
    {
        vk::CommandBuffer cmdBuffer = m_Context->GetCommandBuffer();
    
        vk::Buffer vkBuffer = static_cast<VulkanBuffer*>(buffer)->GetVulkanBuffer();
        cmdBuffer.fillBuffer(vkBuffer, offset, size, 0);
    }
    
    void VulkanCopyEncoder::End()
    {
        m_Context->PopMarker();
        delete this;
    }
}
