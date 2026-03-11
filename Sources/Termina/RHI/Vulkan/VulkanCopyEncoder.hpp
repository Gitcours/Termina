#pragma once

#include <Termina/RHI/CopyEncoder.hpp>

namespace Termina {
    class VulkanRenderContext;

    class VulkanCopyEncoder : public CopyEncoder
    {
    public:
        VulkanCopyEncoder(VulkanRenderContext* context, const std::string& name = "Copy Pass");
        ~VulkanCopyEncoder() override = default;
    
        void CopyBufferToBuffer(RendererBuffer* srcBuffer, uint64 srcOffset,
                                RendererBuffer* dstBuffer, uint64 dstOffset,
                                uint64 size) override;
        void CopyBufferToBuffer(RendererBuffer* srcBuffer,
                                RendererBuffer* dstBuffer) override;
        void CopyBufferToTexture(RendererBuffer* srcBuffer,
                                 RendererTexture* dstTexture,
                                 const BufferTextureCopyRegion& region) override;
        void CopyTextureToBuffer(RendererTexture* srcTexture,
                                 RendererBuffer* dstBuffer,
                                 const BufferTextureCopyRegion& region) override;
        void CopyTextureToTexture(RendererTexture* srcTexture,
                                  RendererTexture* dstTexture,
                                  const TextureCopyRegion& region) override;
        void CopyTextureToTexture(RendererTexture* srcTexture,
                                  RendererTexture* dstTexture) override;
        void ResetBuffer(RendererBuffer* buffer, uint64 offset, uint64 size) override;
        void End() override;
        
    private:
        VulkanRenderContext* m_Context;
    };
}
