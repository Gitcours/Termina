#pragma once

#include <Termina/RHI/Buffer.hpp>

#include <vulkan/vulkan.hpp>
#include <VMA/vk_mem_alloc.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanBuffer : public RendererBuffer
    {
    public:
        VulkanBuffer(VulkanDevice* device, const BufferDesc& desc);
        ~VulkanBuffer() override;
    
        void SetName(const std::string& name) override;
        void* Map() override;
        void Unmap() override;
        uint64 GetGPUAddress() const override;
    
        vk::Buffer GetVulkanBuffer() const { return m_Buffer; }
        vma::Allocation GetVulkanAllocation() const { return m_Allocation; }
        vma::AllocationInfo GetVulkanAllocationInfo() const { return m_AllocationInfo; }
    private:
        VulkanDevice* m_Device;
    
        vk::Buffer m_Buffer;
        vma::Allocation m_Allocation;
        vma::AllocationInfo m_AllocationInfo;
    };
}
