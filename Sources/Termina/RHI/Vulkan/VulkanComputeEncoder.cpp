#include "VulkanComputeEncoder.hpp"
#include "VulkanComputePipeline.hpp"
#include "VulkanRenderContext.hpp"
#include "VulkanDevice.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    VulkanComputeEncoder::VulkanComputeEncoder(VulkanRenderContext* ctx, const std::string& name)
        : m_ParentCtx(ctx)
    {
        ctx->PushMarker(name);
    }
    
    void VulkanComputeEncoder::SetPipeline(Pipeline* pipeline)
    {
        if (pipeline->GetType() != PipelineType::Compute)
        {
            TN_ERROR("VulkanComputeEncoder::SetPipeline: Attempted to set a non-compute pipeline on a compute encoder.");
            return;
        }
    
        vk::PipelineLayout globalPipelineLayout = m_ParentCtx->GetParentDevice()->GetBindlessManager()->GetPipelineLayout();
        vk::DescriptorSet globalDescriptorSet = m_ParentCtx->GetParentDevice()->GetBindlessManager()->GetDescriptorSet();
    
        VulkanComputePipeline* vkPipeline = static_cast<VulkanComputePipeline*>(pipeline);
        m_ParentCtx->GetCommandBuffer().bindPipeline(vk::PipelineBindPoint::eCompute, vkPipeline->GetPipelineState());
        m_ParentCtx->GetCommandBuffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, globalPipelineLayout, 0, globalDescriptorSet, {});
    }
    
    void VulkanComputeEncoder::SetConstants(uint32 size, const void* data)
    {
        vk::PipelineLayout globalPipelineLayout = m_ParentCtx->GetParentDevice()->GetBindlessManager()->GetPipelineLayout();
    
        m_ParentCtx->GetCommandBuffer().pushConstants(globalPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, size, data);
    }
    
    void VulkanComputeEncoder::Dispatch(uint32 x, uint32 y, uint32 z, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ)
    {
        (void)groupSizeX;
        (void)groupSizeY;
        (void)groupSizeZ;
    
        m_ParentCtx->GetCommandBuffer().dispatch(x, y, z);
    }
    
    void VulkanComputeEncoder::End()
    {
        m_ParentCtx->PopMarker();
        delete this;
    }
}
