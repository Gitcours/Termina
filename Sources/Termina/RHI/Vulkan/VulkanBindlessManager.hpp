#pragma once

#include <Termina/Core/FreeList.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;
    class VulkanTextureView;
    class VulkanBufferView;
    class VulkanSampler;
    
    class VulkanBindlessManager
    {
    public:
        VulkanBindlessManager(VulkanDevice* device);
        ~VulkanBindlessManager();
    
        int32 WriteTextureSRV(VulkanTextureView* view);
        int32 WriteTextureUAV(VulkanTextureView* view);
        int32 WriteBufferCBV(VulkanBufferView* view);
        int32 WriteBufferSRV(VulkanBufferView* view);
        int32 WriteBufferUAV(VulkanBufferView* view);
        void FreeResource(int32 index) { m_ResourceAllocator.Free(index); }
    
        int32 WriteSampler(VulkanSampler* sampler);
        void FreeSampler(int32 index) { m_SamplerAllocator.Free(index); }

        int32 WriteAccelerationStructure(vk::AccelerationStructureKHR as);
        void  FreeAccelerationStructure(int32 index) { m_AccelerationStructureAllocator.Free(index); }
    
        vk::DescriptorSetLayout GetLayout() const { return m_Layout; }
        vk::DescriptorSet GetDescriptorSet() const { return m_Set; }
        vk::DescriptorPool GetDescriptorPool() const { return m_Pool; }
        vk::PipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
    private:
        VulkanDevice* m_ParentDevice;
    
        vk::DescriptorSetLayout m_Layout;
        vk::DescriptorSet m_Set;
        vk::DescriptorPool m_Pool;
        vk::PipelineLayout m_PipelineLayout;
    
        FreeList m_ResourceAllocator;
        FreeList m_SamplerAllocator;
        FreeList m_AccelerationStructureAllocator;
    };
}
