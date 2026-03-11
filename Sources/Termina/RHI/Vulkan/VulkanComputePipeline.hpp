#pragma once

#include <Termina/RHI/ComputePipeline.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanComputePipeline : public ComputePipeline
    {
    public:
        VulkanComputePipeline(VulkanDevice* device, const ShaderModule& module, const std::string& name = "Compute Pipeline");
        ~VulkanComputePipeline();
    
        uint64 GetSize() const override { return m_PipelineSize; }
        
        vk::Pipeline GetPipelineState() { return m_PipelineState; }
    protected:
        VulkanDevice* m_ParentDevice;
        vk::Pipeline m_PipelineState;
    
        uint64 m_PipelineSize = 0;
    };
}
