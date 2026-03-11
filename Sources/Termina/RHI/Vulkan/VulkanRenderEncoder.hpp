#pragma once

#include <Termina/RHI/RenderContext.hpp>
#include <Termina/RHI/RenderEncoder.hpp>

namespace Termina {
    class VulkanRenderContext;

    class VulkanRenderEncoder : public RenderEncoder
    {
    public:
        VulkanRenderEncoder(VulkanRenderContext* context, const RenderEncoderInfo& descriptor);
        
        void SetViewport(float x, float y, float width, float height) override;
        void SetScissorRect(int left, int top, int right, int bottom) override;
        void SetPipeline(Pipeline* pipeline) override;
        void SetIndexBuffer(RendererBuffer* buffer) override;
        void SetConstants(uint32 size, const void* data) override;
    
        void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;
        void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) override;
    
        void End() override;
    private:
        VulkanRenderContext* m_Context;
    };
}
