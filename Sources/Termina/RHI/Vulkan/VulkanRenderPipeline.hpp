#pragma once

#include <Termina/RHI/RenderPipeline.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanRenderPipeline : public RenderPipeline
    {
    public:
        VulkanRenderPipeline(VulkanDevice* device, const RenderPipelineDesc& desc);
        ~VulkanRenderPipeline();
    
        uint64 GetSize() const override;
    
        vk::Pipeline GetVulkanPipeline() const { return m_Pipeline; }
    private:
        VulkanDevice* m_ParentDevice;
    
        vk::Pipeline m_Pipeline;
    
        uint64 m_Size;
    };
    
    inline vk::PrimitiveTopology VulkanConvertPipelineTopology(PipelineTopology topology)
    {
        switch (topology)
        {
        case PipelineTopology::TRIANGLES: return vk::PrimitiveTopology::eTriangleList;
        case PipelineTopology::LINES:     return vk::PrimitiveTopology::eLineList;
        case PipelineTopology::POINTS:    return vk::PrimitiveTopology::ePointList;
        default:                          return vk::PrimitiveTopology::eTriangleList;
        }
    }
    
    inline vk::PolygonMode VulkanConvertPipelineFillMode(PipelineFillMode fillMode)
    {
        switch (fillMode)
        {
        case PipelineFillMode::SOLID:     return vk::PolygonMode::eFill;
        case PipelineFillMode::WIREFRAME: return vk::PolygonMode::eLine;
        default:                          return vk::PolygonMode::eFill;
        }
    }
    
    inline vk::CullModeFlagBits VulkanConvertPipelineCullMode(PipelineCullMode cullMode)
    {
        switch (cullMode)
        {
        case PipelineCullMode::NONE:  return vk::CullModeFlagBits::eNone;
        case PipelineCullMode::FRONT: return vk::CullModeFlagBits::eFront;
        case PipelineCullMode::BACK:  return vk::CullModeFlagBits::eBack;
        default:                      return vk::CullModeFlagBits::eBack;
        }
    }
    
    inline vk::CompareOp VulkanConvertPipelineCompareOp(PipelineCompareOp compareOp)
    {
        switch (compareOp)
        {
        case PipelineCompareOp::NEVER:          return vk::CompareOp::eNever;
        case PipelineCompareOp::LESS:           return vk::CompareOp::eLess;
        case PipelineCompareOp::EQUAL:          return vk::CompareOp::eEqual;
        case PipelineCompareOp::LESS_EQUAL:     return vk::CompareOp::eLessOrEqual;
        case PipelineCompareOp::GREATER:        return vk::CompareOp::eGreater;
        case PipelineCompareOp::NOT_EQUAL:      return vk::CompareOp::eNotEqual;
        case PipelineCompareOp::GREATER_EQUAL:  return vk::CompareOp::eGreaterOrEqual;
        case PipelineCompareOp::ALWAYS:         return vk::CompareOp::eAlways;
        default:                                return vk::CompareOp::eLess;
        }
    }
    
    inline vk::ShaderStageFlagBits VulkanConvertShaderTypeToVulkanFlag(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::VERTEX:   return vk::ShaderStageFlagBits::eVertex;
        case ShaderType::PIXEL:    return vk::ShaderStageFlagBits::eFragment;
        case ShaderType::COMPUTE:  return vk::ShaderStageFlagBits::eCompute;
        case ShaderType::MESH:     return vk::ShaderStageFlagBits::eMeshEXT;
        case ShaderType::TASK:     return vk::ShaderStageFlagBits::eTaskEXT;
        default:                   return vk::ShaderStageFlagBits::eVertex;
        }
    }
}
