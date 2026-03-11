#include "VulkanComputePipeline.hpp"
#include "VulkanDevice.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* device, const ShaderModule& module, const std::string& name)
        : m_ParentDevice(device)
    {
        vk::ShaderModuleCreateInfo shaderModuleCreateInfo{};
        shaderModuleCreateInfo.codeSize = module.Bytecode.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32*>(module.Bytecode.data());
    
        vk::ShaderModule shaderModule = device->GetVulkanDevice().createShaderModule(shaderModuleCreateInfo);
    
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageCreateInfo.module = shaderModule;
        shaderStageCreateInfo.pName = module.EntryPoint.c_str();
    
        vk::ComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.stage = shaderStageCreateInfo;
        pipelineCreateInfo.layout = device->GetBindlessManager()->GetPipelineLayout();
        
        m_PipelineState = device->GetVulkanDevice().createComputePipeline(nullptr, pipelineCreateInfo).value;
        if (!m_PipelineState) {
            TN_ERROR("Failed to create Vulkan compute pipeline");
        }
        
        // Name
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectType = vk::ObjectType::ePipeline;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkPipeline>(m_PipelineState));
        nameInfo.pObjectName = name.c_str();
        
        device->GetVulkanDevice().setDebugUtilsObjectNameEXT(nameInfo);
    }
    
    VulkanComputePipeline::~VulkanComputePipeline()
    {
        m_ParentDevice->GetVulkanDevice().destroyPipeline(m_PipelineState);
    }
}
