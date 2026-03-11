#pragma once

#include <string>
#include <vector>

#include <GLM/glm.hpp>

#include "Resource.hpp"
#include "TextureView.hpp"
#include "RenderEncoder.hpp"
#include "CopyEncoder.hpp"
#include "ComputeEncoder.hpp"

namespace Termina {
    struct RenderEncoderInfo
    {
        std::string Name = "Render Pass";
        std::vector<TextureView*> ColorAttachments;
        std::vector<bool> ColorClearFlags;
        std::vector<glm::vec4> ColorClearValues;

        uint32 Width = 0;
        uint32 Height = 0;

        TextureView* DepthAttachment = nullptr;
        bool DepthClearFlag = true;
        bool DepthStoreFlag = true;

        RenderEncoderInfo& SetName(const std::string& name)
        {
            Name = name;
            return *this;
        }

        RenderEncoderInfo& AddColorAttachment(TextureView* textureView, bool clearFlag = true, const glm::vec4& clearValue = glm::vec4(0.0f))
        {
            ColorAttachments.push_back(textureView);
            ColorClearFlags.push_back(clearFlag);
            ColorClearValues.push_back(clearValue);
            return *this;
        }

        RenderEncoderInfo& SetDepthAttachment(TextureView* textureView, bool clearFlag = true, bool storeFlag = true)
        {
            DepthAttachment = textureView;
            DepthClearFlag = clearFlag;
            DepthStoreFlag = storeFlag;
            return *this;
        }

        RenderEncoderInfo& SetDimensions(uint32 width, uint32 height)
        {
            Width = width;
            Height = height;
            return *this;
        }
    };

    struct TextureBarrier
    {
        PipelineStage DstStage;
        ResourceAccess DstAccess;
        TextureLayout NewLayout;
        RendererTexture* TargetTexture;

        uint32 BaseMipLevel = 0;
        uint32 MipLevelCount = VIEW_ALL_MIP_LEVELS;
        uint32 BaseArrayLayer = 0;
        uint32 ArrayLayerCount = VIEW_ALL_ARRAY_LAYERS;

        TextureBarrier& SetMipLevels(uint32 baseMipLevel, uint32 mipLevelCount)
        {
            BaseMipLevel = baseMipLevel;
            MipLevelCount = mipLevelCount;
            return *this;
        }

        TextureBarrier& SetArrayLayers(uint32 baseArrayLayer, uint32 arrayLayerCount)
        {
            BaseArrayLayer = baseArrayLayer;
            ArrayLayerCount = arrayLayerCount;
            return *this;
        }

        TextureBarrier& SetTargetTexture(RendererTexture* texture)
        {
            TargetTexture = texture;
            return *this;
        }

        TextureBarrier& SetDstStage(PipelineStage stage)
        {
            DstStage = stage;
            return *this;
        }

        TextureBarrier& SetDstAccess(ResourceAccess access)
        {
            DstAccess = access;
            return *this;
        }

        TextureBarrier& SetNewLayout(TextureLayout layout)
        {
            NewLayout = layout;
            return *this;
        }
    };

    struct BufferBarrier
    {
        PipelineStage DstStage;
        ResourceAccess DstAccess;
        RendererBuffer* TargetBuffer;

        BufferBarrier& SetTargetBuffer(RendererBuffer* buffer)
        {
            TargetBuffer = buffer;
            return *this;
        }

        BufferBarrier& SetDstStage(PipelineStage stage)
        {
            DstStage = stage;
            return *this;
        }

        BufferBarrier& SetDstAccess(ResourceAccess access)
        {
            DstAccess = access;
            return *this;
        }
    };

    struct BarrierGroup
    {
        std::vector<TextureBarrier> TextureBarriers;
        std::vector<BufferBarrier> BufferBarriers;
    };

    class RenderContext
    {
    public:
        virtual ~RenderContext() = default;

        virtual void Reset() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void Barrier(const TextureBarrier& textureBarrier) = 0;
        virtual void Barrier(const BufferBarrier& bufferBarrier) = 0;
        virtual void Barrier(const BarrierGroup& barrierGroup) = 0;

        virtual void PushMarker(const std::string& name) = 0;
        virtual void PopMarker() = 0;

        virtual RenderEncoder* CreateRenderEncoder(const RenderEncoderInfo& info) = 0;
        virtual CopyEncoder* CreateCopyEncoder(const std::string& name = "Copy Pass") = 0;
        virtual ComputeEncoder* CreateComputeEncoder(const std::string& name = "Compute Pass") = 0;
};
}
