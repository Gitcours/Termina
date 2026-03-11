#pragma once

#include <Termina/RHI/RenderContext.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanRenderContext : public RenderContext
    {
    public:
        VulkanRenderContext(VulkanDevice* device, bool singleTime = false);
        ~VulkanRenderContext() override;
    
        void Reset() override;
        void Begin() override;
        void End() override;
    
        void Barrier(const TextureBarrier& textureBarrier) override;
        void Barrier(const BufferBarrier& bufferBarrier) override;
        void Barrier(const BarrierGroup& barrierGroup) override;
    
        RenderEncoder* CreateRenderEncoder(const RenderEncoderInfo& info) override;
        CopyEncoder* CreateCopyEncoder(const std::string& name = "Copy Pass") override;
        ComputeEncoder* CreateComputeEncoder(const std::string& name = "Compute Pass") override;
    
        vk::CommandBuffer& GetCommandBuffer() { return m_CommandBuffer; }
    
        void PushMarker(const std::string& name) override;
        void PopMarker() override;
    
        VulkanDevice* GetParentDevice() const { return m_ParentDevice; }
    private:
        bool m_SingleTime = false;
    
        VulkanDevice* m_ParentDevice;
        vk::CommandBuffer m_CommandBuffer;
    };
}
