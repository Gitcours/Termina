#pragma once

#include <Termina/RHI/ComputeEncoder.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanRenderContext;

    class VulkanComputeEncoder : public ComputeEncoder
    {
    public:
        VulkanComputeEncoder(VulkanRenderContext* ctx, const std::string& name);
        ~VulkanComputeEncoder() = default;
    
        void SetPipeline(Pipeline* pipeline) override;
        void SetConstants(uint32 size, const void* data) override;
    
        void Dispatch(uint32 x, uint32 y, uint32 z, uint32 groupSizeX, uint32 groupSizeY, uint32 groupSizeZ) override;
        void End() override;
    private:
        VulkanRenderContext* m_ParentCtx;
    };
}
