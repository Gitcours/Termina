#pragma once

#include "Pipeline.hpp"
#include "Buffer.hpp"

#include <Termina/Core/Common.hpp>

namespace Termina {
    class RenderEncoder
    {
    public:
        virtual ~RenderEncoder() = default;

        virtual void SetViewport(float x, float y, float width, float height) = 0;
        virtual void SetScissorRect(int left, int top, int right, int bottom) = 0;
        virtual void SetPipeline(Pipeline* pipeline) = 0;
        virtual void SetIndexBuffer(RendererBuffer* buffer) = 0;
        virtual void SetConstants(uint32 size, const void* data) = 0;

        virtual void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) = 0;
        virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) = 0;
        
        virtual void End() = 0;
};
}
