#include "VulkanRenderEncoder.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanTextureView.hpp"
#include "VulkanRenderPipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

namespace Termina {
    VulkanRenderEncoder::VulkanRenderEncoder(VulkanRenderContext* context, const RenderEncoderInfo& descriptor)
        : m_Context(context)
    {
        context->PushMarker(descriptor.Name.empty() ? "Render Pass" : descriptor.Name);
    
        // Transition all resources to appropriate states
        BarrierGroup barrierGroup;
    
        for (const auto& colorTexture : descriptor.ColorAttachments) {
            RendererTexture* texture = colorTexture->GetDesc().Texture;
            if (!texture) continue;
        
            TextureBarrier barrier;
            barrier.TargetTexture = texture;
            barrier.DstStage = PipelineStage::COLOR_ATTACHMENT_OUTPUT;
            barrier.DstAccess = ResourceAccess::COLOR_ATTACHMENT_WRITE;
            barrier.NewLayout = TextureLayout::COLOR_ATTACHMENT;
        
            barrierGroup.TextureBarriers.push_back(barrier);
        }
    
        if (descriptor.DepthAttachment) {
            RendererTexture* texture = descriptor.DepthAttachment->GetDesc().Texture;
            if (texture) {
                TextureBarrier barrier;
                barrier.TargetTexture = texture;
                barrier.DstStage = PipelineStage::EARLY_FRAGMENT_TESTS;
                barrier.DstAccess = ResourceAccess::DEPTH_STENCIL_ATTACHMENT_WRITE;
                barrier.NewLayout = TextureLayout::DEPTH_ATTACHMENT;
            
                barrierGroup.TextureBarriers.push_back(barrier);
            }
        }
        m_Context->Barrier(barrierGroup);
    
        // Begin render pass
        std::vector<vk::RenderingAttachmentInfo> colorAttachments;
        colorAttachments.reserve(descriptor.ColorAttachments.size());
        for (size_t i = 0; i < descriptor.ColorAttachments.size(); i++) {
            vk::ClearColorValue clearColor;
            clearColor.setFloat32({ descriptor.ColorClearValues[i].r, descriptor.ColorClearValues[i].g, descriptor.ColorClearValues[i].b, descriptor.ColorClearValues[i].a });
        
            VulkanTextureView* vkTextureView = static_cast<VulkanTextureView*>(descriptor.ColorAttachments[i]);
            vk::RenderingAttachmentInfo colorAttachment;
            colorAttachment.imageView = vkTextureView->GetVkImageView();
            colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            if (descriptor.ColorClearFlags[i]) {
                colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                colorAttachment.clearValue = vk::ClearValue(clearColor);
            } else {
                colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
            }
            colorAttachments.push_back(colorAttachment);
        }
    
        vk::RenderingAttachmentInfo depthAttachment;
        if (descriptor.DepthAttachment) {
            vk::ClearDepthStencilValue clearDepth;
            clearDepth.depth = 1.0f;
        
            VulkanTextureView* vkTextureView = static_cast<VulkanTextureView*>(descriptor.DepthAttachment);
            depthAttachment.imageView = vkTextureView->GetVkImageView();
            depthAttachment.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            if (descriptor.DepthClearFlag) {
                depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
                depthAttachment.clearValue = vk::ClearValue(clearDepth);
            } else {
                depthAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
            }
            if (descriptor.DepthStoreFlag) {
                depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
            } else {
                depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
            }
        }
    
        vk::RenderingInfo renderingInfo;
        renderingInfo.colorAttachmentCount = static_cast<uint32>(colorAttachments.size());
        renderingInfo.pColorAttachments = colorAttachments.data();
        if (descriptor.DepthAttachment) {
            renderingInfo.pDepthAttachment = &depthAttachment;
        }
        renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
        renderingInfo.renderArea.extent = vk::Extent2D{ descriptor.Width, descriptor.Height };
        renderingInfo.layerCount = 1;
        renderingInfo.viewMask = 0;
    
        m_Context->GetCommandBuffer().beginRendering(renderingInfo);
    }
    
    void VulkanRenderEncoder::SetPipeline(Pipeline* pipeline)
    {
        if (pipeline->GetType() == PipelineType::Graphics) {
            VulkanRenderPipeline* vkPipeline = static_cast<VulkanRenderPipeline*>(pipeline);
            vk::DescriptorSet globalSet = m_Context->GetParentDevice()->GetBindlessManager()->GetDescriptorSet();
            
            m_Context->GetCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline->GetVulkanPipeline());
            m_Context->GetCommandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Context->GetParentDevice()->GetBindlessManager()->GetPipelineLayout(), 0, 1, &globalSet, 0, nullptr);
        }
    }
    
    void VulkanRenderEncoder::SetIndexBuffer(RendererBuffer* buffer)
    {
        VulkanBuffer* vkBuffer = static_cast<VulkanBuffer*>(buffer);
        vk::IndexType indexType = buffer->GetStride() == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32;
        m_Context->GetCommandBuffer().bindIndexBuffer(vkBuffer->GetVulkanBuffer(), 0, indexType);
    }
    
    void VulkanRenderEncoder::SetConstants(uint32 size, const void* data)
    {
        m_Context->GetCommandBuffer().pushConstants(m_Context->GetParentDevice()->GetBindlessManager()->GetPipelineLayout(), vk::ShaderStageFlagBits::eAll, 0, size, data);
    }
    
    void VulkanRenderEncoder::SetViewport(float x, float y, float width, float height)
    {
        vk::Viewport viewport;
        viewport.width = width;
        viewport.height = -height;
        viewport.x = x;
        viewport.y = y + height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    
        m_Context->GetCommandBuffer().setViewport(0, 1, &viewport);
    }
    
    void VulkanRenderEncoder::SetScissorRect(int left, int top, int right, int bottom)
    {
        vk::Rect2D scissor;
        scissor.offset.x = static_cast<int32_t>(left);
        scissor.offset.y = static_cast<int32_t>(top);
        scissor.extent.width = static_cast<uint>(right - left);
        scissor.extent.height = static_cast<uint>(bottom - top);
    
        m_Context->GetCommandBuffer().setScissor(0, 1, &scissor);
    }
    
    void VulkanRenderEncoder::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
    {
        m_Context->GetCommandBuffer().draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    
    void VulkanRenderEncoder::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance)
    {
        m_Context->GetCommandBuffer().drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    
    void VulkanRenderEncoder::End()
    {
        m_Context->GetCommandBuffer().endRendering();
        m_Context->PopMarker();
        delete this;
    }
}
