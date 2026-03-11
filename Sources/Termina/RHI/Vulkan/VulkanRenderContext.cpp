#include "VulkanRenderContext.hpp"
#include "VulkanResource.hpp"
#include "VulkanTexture.hpp"
#include "VulkanRenderEncoder.hpp"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanCopyEncoder.hpp"
#include "VulkanComputeEncoder.hpp"

namespace Termina {
    VulkanRenderContext::VulkanRenderContext(VulkanDevice* device, bool singleTime)
        : m_SingleTime(singleTime)
        , m_ParentDevice(device)
    {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = device->GetCommandPool();
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;
    
        m_CommandBuffer = m_ParentDevice->GetVulkanDevice().allocateCommandBuffers(allocInfo).front();
    }
    
    VulkanRenderContext::~VulkanRenderContext()
    {
        m_ParentDevice->GetVulkanDevice().freeCommandBuffers(m_ParentDevice->GetCommandPool(), m_CommandBuffer);
    }
    
    void VulkanRenderContext::Reset()
    {
        m_CommandBuffer.reset();
    }
    
    void VulkanRenderContext::Begin()
    {
        m_CommandBuffer.begin(
            m_SingleTime ? vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit } 
                         : vk::CommandBufferBeginInfo{}
        );
    }
    
    void VulkanRenderContext::End()
    {
        m_CommandBuffer.end();
    }
    
    void VulkanRenderContext::Barrier(const TextureBarrier& textureBarrier)
    {
        TextureDesc desc = textureBarrier.TargetTexture->GetDesc();
        VulkanTexture* vkTexture = static_cast<VulkanTexture*>(textureBarrier.TargetTexture);
    
        vk::ImageSubresourceRange subresourceRange;
        subresourceRange.aspectMask = Any(desc.Usage, TextureUsage::DEPTH_TARGET) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subresourceRange.baseMipLevel = textureBarrier.BaseMipLevel;
        subresourceRange.levelCount = (textureBarrier.MipLevelCount == VIEW_ALL_MIP_LEVELS) ? desc.MipLevels : textureBarrier.MipLevelCount;
        subresourceRange.baseArrayLayer = textureBarrier.BaseArrayLayer;
        subresourceRange.layerCount = (textureBarrier.ArrayLayerCount == VIEW_ALL_ARRAY_LAYERS) ? desc.ArrayLayers : textureBarrier.ArrayLayerCount;
    
        vk::ImageMemoryBarrier2 barrier;
        barrier.srcStageMask = ConvertPipelineStageToVulkan(textureBarrier.TargetTexture->GetLastPipelineStage());
        barrier.dstStageMask = ConvertPipelineStageToVulkan(textureBarrier.DstStage);
        barrier.srcAccessMask = ConvertResourceAccessToVulkan(textureBarrier.TargetTexture->GetLastAccess());
        barrier.dstAccessMask = ConvertResourceAccessToVulkan(textureBarrier.DstAccess);
        barrier.oldLayout = ConvertTextureLayoutToVulkan(textureBarrier.TargetTexture->GetCurrentLayout());
        barrier.newLayout = ConvertTextureLayoutToVulkan(textureBarrier.NewLayout);
        barrier.subresourceRange = subresourceRange;
        barrier.image = vkTexture->GetImage();
    
        textureBarrier.TargetTexture->SetCurrentPipelineStage(textureBarrier.DstStage);
        textureBarrier.TargetTexture->SetCurrentAccess(textureBarrier.DstAccess);
        textureBarrier.TargetTexture->SetCurrentLayout(textureBarrier.NewLayout);
    
        vk::DependencyInfo dependencyInfo;
        dependencyInfo.pImageMemoryBarriers = &barrier;
        dependencyInfo.imageMemoryBarrierCount = 1;
    
        m_CommandBuffer.pipelineBarrier2(dependencyInfo);
    }
    
    void VulkanRenderContext::Barrier(const BufferBarrier& bufferBarrier)
    {
        VulkanBuffer* vkBuffer = static_cast<VulkanBuffer*>(bufferBarrier.TargetBuffer);
    
        vk::BufferMemoryBarrier2 barrier;
        barrier.srcStageMask = ConvertPipelineStageToVulkan(bufferBarrier.TargetBuffer->GetLastPipelineStage());
        barrier.dstStageMask = ConvertPipelineStageToVulkan(bufferBarrier.DstStage);
        barrier.srcAccessMask = ConvertResourceAccessToVulkan(bufferBarrier.TargetBuffer->GetLastAccess());
        barrier.dstAccessMask = ConvertResourceAccessToVulkan(bufferBarrier.DstAccess);
        barrier.buffer = vkBuffer->GetVulkanBuffer();
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;
    
        bufferBarrier.TargetBuffer->SetCurrentPipelineStage(bufferBarrier.DstStage);
        bufferBarrier.TargetBuffer->SetCurrentAccess(bufferBarrier.DstAccess);
    
        vk::DependencyInfo dependencyInfo;
        dependencyInfo.pBufferMemoryBarriers = &barrier;
        dependencyInfo.bufferMemoryBarrierCount = 1;
    
        m_CommandBuffer.pipelineBarrier2(dependencyInfo);
    }
    
    void VulkanRenderContext::Barrier(const BarrierGroup& barrierGroup)
    {
        std::vector<vk::ImageMemoryBarrier2> imageBarriers;
        imageBarriers.reserve(barrierGroup.TextureBarriers.size());
    
        std::vector<vk::BufferMemoryBarrier2> bufferBarriers;
        bufferBarriers.reserve(barrierGroup.BufferBarriers.size());
    
        for (const TextureBarrier& textureBarrier : barrierGroup.TextureBarriers) {
            if (!textureBarrier.TargetTexture) continue;
        
            TextureDesc desc = textureBarrier.TargetTexture->GetDesc();
            VulkanTexture* vkTexture = static_cast<VulkanTexture*>(textureBarrier.TargetTexture);
        
            vk::ImageSubresourceRange subresourceRange;
            subresourceRange.aspectMask = Any(desc.Usage, TextureUsage::DEPTH_TARGET) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
            subresourceRange.baseMipLevel = textureBarrier.BaseMipLevel;
            subresourceRange.levelCount = (textureBarrier.MipLevelCount == VIEW_ALL_MIP_LEVELS) ? desc.MipLevels : textureBarrier.MipLevelCount;
            subresourceRange.baseArrayLayer = textureBarrier.BaseArrayLayer;
            subresourceRange.layerCount = (textureBarrier.ArrayLayerCount == VIEW_ALL_ARRAY_LAYERS) ? desc.ArrayLayers : textureBarrier.ArrayLayerCount;
        
            vk::ImageMemoryBarrier2 barrier;
            barrier.srcStageMask = ConvertPipelineStageToVulkan(textureBarrier.TargetTexture->GetLastPipelineStage());
            barrier.dstStageMask = ConvertPipelineStageToVulkan(textureBarrier.DstStage);
            barrier.srcAccessMask = ConvertResourceAccessToVulkan(textureBarrier.TargetTexture->GetLastAccess());
            barrier.dstAccessMask = ConvertResourceAccessToVulkan(textureBarrier.DstAccess);
            barrier.oldLayout = ConvertTextureLayoutToVulkan(textureBarrier.TargetTexture->GetCurrentLayout());
            barrier.newLayout = ConvertTextureLayoutToVulkan(textureBarrier.NewLayout);
            barrier.subresourceRange = subresourceRange;
            barrier.image = vkTexture->GetImage();
        
            textureBarrier.TargetTexture->SetCurrentPipelineStage(textureBarrier.DstStage);
            textureBarrier.TargetTexture->SetCurrentAccess(textureBarrier.DstAccess);
            textureBarrier.TargetTexture->SetCurrentLayout(textureBarrier.NewLayout);
        
            imageBarriers.push_back(barrier);
        }
    
        for (const BufferBarrier& bufferBarrier : barrierGroup.BufferBarriers) {
            if (!bufferBarrier.TargetBuffer) continue;
        
            VulkanBuffer* vkBuffer = static_cast<VulkanBuffer*>(bufferBarrier.TargetBuffer);
        
            vk::BufferMemoryBarrier2 barrier;
            barrier.srcStageMask = ConvertPipelineStageToVulkan(bufferBarrier.TargetBuffer->GetLastPipelineStage());
            barrier.dstStageMask = ConvertPipelineStageToVulkan(bufferBarrier.DstStage);
            barrier.srcAccessMask = ConvertResourceAccessToVulkan(bufferBarrier.TargetBuffer->GetLastAccess());
            barrier.dstAccessMask = ConvertResourceAccessToVulkan(bufferBarrier.DstAccess);
            barrier.buffer = vkBuffer->GetVulkanBuffer();
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
        
            bufferBarrier.TargetBuffer->SetCurrentPipelineStage(bufferBarrier.DstStage);
            bufferBarrier.TargetBuffer->SetCurrentAccess(bufferBarrier.DstAccess);
        
            bufferBarriers.push_back(barrier);
        }
    
        vk::DependencyInfo dependencyInfo;
        dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
        dependencyInfo.imageMemoryBarrierCount = static_cast<uint32>(imageBarriers.size());
        dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
        dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32>(bufferBarriers.size());
        // TODO: Memory barriers
        m_CommandBuffer.pipelineBarrier2(dependencyInfo);
    }
    
    void VulkanRenderContext::PushMarker(const std::string& name)
    {
        vk::DebugUtilsLabelEXT labelInfo;
        labelInfo.pLabelName = name.c_str();
        m_CommandBuffer.beginDebugUtilsLabelEXT(labelInfo);
    }
    
    void VulkanRenderContext::PopMarker()
    {
        m_CommandBuffer.endDebugUtilsLabelEXT();
    }
    
    RenderEncoder* VulkanRenderContext::CreateRenderEncoder(const RenderEncoderInfo& info)
    {
        return new VulkanRenderEncoder(this, info);
    }
    
    CopyEncoder* VulkanRenderContext::CreateCopyEncoder(const std::string& name)
    {
        return new VulkanCopyEncoder(this, name);
    }
    
    ComputeEncoder* VulkanRenderContext::CreateComputeEncoder(const std::string& name)
    {
        return new VulkanComputeEncoder(this, name);
    }
}
