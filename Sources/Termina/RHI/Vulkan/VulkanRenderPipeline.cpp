#include "VulkanRenderPipeline.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"

#include <Termina/Core/Logger.hpp>
#include <Termina/Core/Common.hpp>

#include <vector>

namespace Termina {
    VulkanRenderPipeline::VulkanRenderPipeline(VulkanDevice* device, const RenderPipelineDesc& desc)
        : m_ParentDevice(device)
    {
        m_Desc = desc;
    
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        std::vector<vk::ShaderModule> shaderModules;
    
        for (auto& [stage, module] : desc.ShaderModules) {
            vk::ShaderModuleCreateInfo shaderModuleCreateInfo{};
            shaderModuleCreateInfo.codeSize = module.Bytecode.size();
            shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32*>(module.Bytecode.data());
        
            vk::ShaderModule shaderModule = m_ParentDevice->GetVulkanDevice().createShaderModule(shaderModuleCreateInfo);
            shaderModules.push_back(shaderModule);
        
            vk::PipelineShaderStageCreateInfo shaderStageCreateInfo{};
            shaderStageCreateInfo.stage = VulkanConvertShaderTypeToVulkanFlag(stage);
            shaderStageCreateInfo.module = shaderModule;
            shaderStageCreateInfo.pName = module.EntryPoint.c_str();
        
            shaderStages.push_back(shaderStageCreateInfo);
        }
    
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
        inputAssemblyState.topology = VulkanConvertPipelineTopology(desc.Topology);
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;
    
        vk::PipelineRasterizationStateCreateInfo rasterizationState;
        rasterizationState.polygonMode = VulkanConvertPipelineFillMode(desc.FillMode);
        rasterizationState.cullMode = VulkanConvertPipelineCullMode(desc.CullMode);
        rasterizationState.frontFace = desc.CCW ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.depthClampEnable = desc.DepthClampEnabled;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.depthBiasConstantFactor = 0.0f;
        rasterizationState.depthBiasClamp = 0.0f;
        rasterizationState.depthBiasSlopeFactor = 0.0f;
    
        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
        for (uint64 i = 0; i < desc.ColorAttachmentFormats.size(); i++) {
            vk::PipelineColorBlendAttachmentState colorBlendAttachment;
            colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colorBlendAttachment.blendEnable = VK_FALSE;
            if (desc.EnableBlending) {
                colorBlendAttachment.blendEnable = VK_TRUE;
                colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
                colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
                colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
                colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
                colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
                colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
            }
            colorBlendAttachments.push_back(colorBlendAttachment);
        }
    
        vk::PipelineColorBlendStateCreateInfo colorBlendState;
        colorBlendState.attachmentCount = static_cast<uint32>(colorBlendAttachments.size());
        colorBlendState.pAttachments = colorBlendAttachments.data();
        colorBlendState.logicOpEnable = VK_FALSE;
    
        vk::PipelineDepthStencilStateCreateInfo depthStencilState;
        depthStencilState.depthTestEnable = desc.DepthReadEnabled;
        depthStencilState.depthWriteEnable = desc.DepthWriteEnabled;
        depthStencilState.depthCompareOp = VulkanConvertPipelineCompareOp(desc.DepthCompareOp);
        depthStencilState.depthBoundsTestEnable = VK_FALSE;
        depthStencilState.stencilTestEnable = VK_FALSE;
        depthStencilState.minDepthBounds = 0.0f;
        depthStencilState.maxDepthBounds = 1.0f;
    
        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
    
        vk::PipelineMultisampleStateCreateInfo multisampleState;
        multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
    
        // NOTE: More dynamic states? (Blend constants, depth bias, etc.)
        vk::PipelineDynamicStateCreateInfo dynamicState;
        std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        dynamicState.dynamicStateCount = static_cast<uint32>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
    
        vk::VertexInputBindingDescription vertexBindingDescription;
        vertexBindingDescription.binding = 0;
        vertexBindingDescription.stride = 0;
        vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;
    
        vk::PipelineVertexInputStateCreateInfo vertexInputState;
        vertexInputState.vertexBindingDescriptionCount = 1;
        vertexInputState.pVertexBindingDescriptions = &vertexBindingDescription;
        vertexInputState.vertexAttributeDescriptionCount = 0;
        vertexInputState.pVertexAttributeDescriptions = nullptr;
    
        vk::PipelineRenderingCreateInfo pipelineRenderingInfo;
        std::vector<vk::Format> colorAttachmentFormats;
        for (auto& format : desc.ColorAttachmentFormats) {
            colorAttachmentFormats.push_back(ConvertTextureFormatToVulkan(format));
        }
        pipelineRenderingInfo.colorAttachmentCount = static_cast<uint32>(colorAttachmentFormats.size());
        pipelineRenderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
        if (desc.DepthAttachmentFormat != TextureFormat::UNDEFINED) {
            pipelineRenderingInfo.depthAttachmentFormat = ConvertTextureFormatToVulkan(desc.DepthAttachmentFormat);
        }
    
        vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.stageCount = static_cast<uint32>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.layout = m_ParentDevice->GetBindlessManager()->GetPipelineLayout();
        pipelineCreateInfo.renderPass = nullptr;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.pNext = &pipelineRenderingInfo;
    
        // Create a pipeline cache to track pipeline size
        vk::PipelineCacheCreateInfo cacheCreateInfo{};
        vk::PipelineCache pipelineCache = m_ParentDevice->GetVulkanDevice().createPipelineCache(cacheCreateInfo);
    
        m_Pipeline = m_ParentDevice->GetVulkanDevice().createGraphicsPipeline(pipelineCache, pipelineCreateInfo).value;
        if (!m_Pipeline) {
            TN_ERROR("Failed to create Vulkan graphics pipeline");
        }
    
        // Get the pipeline cache data size
        auto cacheData = m_ParentDevice->GetVulkanDevice().getPipelineCacheData(pipelineCache);
        m_Size = cacheData.size();
    
        m_ParentDevice->GetVulkanDevice().destroyPipelineCache(pipelineCache);
    
        for (auto& module : shaderModules) {
            m_ParentDevice->GetVulkanDevice().destroyShaderModule(module);
        }
    
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectType = vk::ObjectType::ePipeline;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkPipeline>(m_Pipeline));
        nameInfo.pObjectName = desc.Name.c_str();
    
        m_ParentDevice->GetVulkanDevice().setDebugUtilsObjectNameEXT(nameInfo);
    }
    
    VulkanRenderPipeline::~VulkanRenderPipeline()
    {
        m_ParentDevice->GetVulkanDevice().destroyPipeline(m_Pipeline);
    }
    
    uint64 VulkanRenderPipeline::GetSize() const
    {
        return m_Size;
    }
}
