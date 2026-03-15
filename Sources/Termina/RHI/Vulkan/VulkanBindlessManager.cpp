#include "VulkanBindlessManager.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTextureView.hpp"
#include "VulkanBufferView.hpp"
#include "VulkanSampler.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    constexpr uint64 MAX_BINDLESS_RESOURCES = 400'000;
    constexpr uint64 MAX_BINDLESS_SAMPLERS = 2'000;
    constexpr uint64 MAX_BINDLESS_ACCELERATION_STRUCTURES = 8;
    
    VulkanBindlessManager::VulkanBindlessManager(VulkanDevice* device)
        : m_ParentDevice(device),
          m_ResourceAllocator(MAX_BINDLESS_RESOURCES), 
          m_SamplerAllocator(MAX_BINDLESS_SAMPLERS),
          m_AccelerationStructureAllocator(MAX_BINDLESS_ACCELERATION_STRUCTURES)
    {
        std::vector<vk::DescriptorType> cbvSrvUavTypes = {
            vk::DescriptorType::eSampledImage,
            vk::DescriptorType::eStorageImage,
            vk::DescriptorType::eUniformTexelBuffer,
            vk::DescriptorType::eStorageTexelBuffer,
            vk::DescriptorType::eUniformBuffer,
            vk::DescriptorType::eStorageBuffer
        };
    
        vk::MutableDescriptorTypeListEXT cbvSrvUavTypeList;
        cbvSrvUavTypeList.setDescriptorTypes(cbvSrvUavTypes);
    
        vk::MutableDescriptorTypeCreateInfoEXT mutableInfo;
        mutableInfo.setMutableDescriptorTypeLists({ cbvSrvUavTypeList });
    
        vk::DescriptorSetLayoutBinding cbvSrvUavBinding;
        cbvSrvUavBinding.setBinding(0);
        cbvSrvUavBinding.setDescriptorType(vk::DescriptorType::eMutableEXT);
        cbvSrvUavBinding.setDescriptorCount(MAX_BINDLESS_RESOURCES);
        cbvSrvUavBinding.setStageFlags(vk::ShaderStageFlagBits::eAll);
    
        vk::DescriptorSetLayoutBinding samplerBinding;
        samplerBinding.setBinding(1);
        samplerBinding.setDescriptorType(vk::DescriptorType::eSampler);
        samplerBinding.setDescriptorCount(MAX_BINDLESS_SAMPLERS);
        samplerBinding.setStageFlags(vk::ShaderStageFlagBits::eAll);
    
        vk::DescriptorSetLayoutBinding accelerationStructureBinding;
        accelerationStructureBinding.setBinding(2);
        accelerationStructureBinding.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        accelerationStructureBinding.setDescriptorCount(MAX_BINDLESS_ACCELERATION_STRUCTURES);
        accelerationStructureBinding.setStageFlags(vk::ShaderStageFlagBits::eAll);
    
        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            cbvSrvUavBinding,
            samplerBinding,
        };
        std::vector<vk::DescriptorBindingFlags> bindingFlags = {
            vk::DescriptorBindingFlagBits::eUpdateAfterBind,
            vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        };
        if (m_ParentDevice->SupportsRaytracing()) {
            bindings.push_back(accelerationStructureBinding);
            bindingFlags.push_back(vk::DescriptorBindingFlagBits::eUpdateAfterBind);
        }
    
        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
        bindingFlagsInfo.setBindingFlags(bindingFlags);
        bindingFlagsInfo.setPNext(&mutableInfo);
    
        vk::DescriptorSetLayoutCreateInfo layoutInfo;
        layoutInfo.setBindings(bindings);
        layoutInfo.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool);
        layoutInfo.setPNext(&bindingFlagsInfo);
    
        m_Layout = m_ParentDevice->GetVulkanDevice().createDescriptorSetLayout(layoutInfo);
        if (!m_Layout) {
            TN_ERROR("Failed to create Vulkan bindless descriptor set layout");
            return;
        }
    
        // Pool
        std::vector<vk::DescriptorPoolSize> poolSizes = {
            { vk::DescriptorType::eMutableEXT, MAX_BINDLESS_RESOURCES },
            { vk::DescriptorType::eSampler, MAX_BINDLESS_SAMPLERS }
        };
        if (m_ParentDevice->SupportsRaytracing()) {
            poolSizes.push_back({ vk::DescriptorType::eAccelerationStructureKHR, MAX_BINDLESS_ACCELERATION_STRUCTURES });
        }
    
        vk::DescriptorPoolCreateInfo poolInfo;
        poolInfo.setMaxSets(1);
        poolInfo.setPoolSizes(poolSizes);
        poolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    
        m_Pool = m_ParentDevice->GetVulkanDevice().createDescriptorPool(poolInfo);
        if (!m_Pool) {
            TN_ERROR("Failed to create Vulkan bindless descriptor pool");
            return;
        }
    
        // Descriptor set
        std::vector<vk::DescriptorSetLayout> layouts = { m_Layout };
    
        vk::DescriptorSetAllocateInfo allocInfo;
        allocInfo.setDescriptorPool(m_Pool);
        allocInfo.setSetLayouts(layouts);
        allocInfo.setDescriptorSetCount(1);
    
        auto result = m_ParentDevice->GetVulkanDevice().allocateDescriptorSets(&allocInfo, &m_Set);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to allocate Vulkan bindless descriptor set");
            return;
        }
    
        // Pipeline layout
        vk::PushConstantRange pushConstantRange;
        pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eAll);
        pushConstantRange.setOffset(0);
        pushConstantRange.setSize(128);
    
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        pipelineLayoutInfo.setSetLayouts(layouts);
        pipelineLayoutInfo.setPushConstantRanges({  pushConstantRange  });
    
        m_PipelineLayout = m_ParentDevice->GetVulkanDevice().createPipelineLayout(pipelineLayoutInfo);
    }
    
    VulkanBindlessManager::~VulkanBindlessManager()
    {
        m_ParentDevice->GetVulkanDevice().destroyPipelineLayout(m_PipelineLayout);
        m_ParentDevice->GetVulkanDevice().freeDescriptorSets(m_Pool, m_Set);
        m_ParentDevice->GetVulkanDevice().destroyDescriptorSetLayout(m_Layout);
        m_ParentDevice->GetVulkanDevice().destroyDescriptorPool(m_Pool);
    }
    
    int32 VulkanBindlessManager::WriteTextureSRV(VulkanTextureView* view)
    {
        int32 index = m_ResourceAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless texture SRV descriptor - resource pool exhausted");
            TN_ERROR("Consider increasing MAX_BINDLESS_RESOURCES or check for resource leaks");
            return -1;
        }
    
        vk::DescriptorImageInfo imageInfo;
        imageInfo.setImageView(view->GetVkImageView());
        imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(0);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eSampledImage);
        writeInfo.setImageInfo(imageInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
    
        return index;
    }
    
    int32 VulkanBindlessManager::WriteTextureUAV(VulkanTextureView* view)
    {
        int32 index = m_ResourceAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless texture UAV descriptor - resource pool exhausted");
            TN_ERROR("Consider increasing MAX_BINDLESS_RESOURCES or check for resource leaks");
            return -1;
        }
    
        vk::DescriptorImageInfo imageInfo;
        imageInfo.setImageView(view->GetVkImageView());
        imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(0);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eStorageImage);
        writeInfo.setImageInfo(imageInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
    
        return index;
    }
    
    int32 VulkanBindlessManager::WriteBufferCBV(VulkanBufferView* view)
    {
        int32 index = m_ResourceAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless buffer CBV descriptor");
            return -1;
        }
    
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(reinterpret_cast<VulkanBuffer*>(view->GetDesc().Buffer)->GetVulkanBuffer());
        bufferInfo.setOffset(0);
        bufferInfo.setRange(VK_WHOLE_SIZE);
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(0);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eUniformBuffer);
        writeInfo.setBufferInfo(bufferInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
        return index;
    }
    
    int32 VulkanBindlessManager::WriteBufferSRV(VulkanBufferView* view)
    {
        int32 index = m_ResourceAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless buffer SRV descriptor");
            return -1;
        }
    
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(reinterpret_cast<VulkanBuffer*>(view->GetDesc().Buffer)->GetVulkanBuffer());
        bufferInfo.setOffset(0);
        bufferInfo.setRange(VK_WHOLE_SIZE);
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(0);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eStorageBuffer);
        writeInfo.setBufferInfo(bufferInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
        return index;
    }
    
    int32 VulkanBindlessManager::WriteBufferUAV(VulkanBufferView* view)
    {
        int32 index = m_ResourceAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless buffer UAV descriptor");
            return -1;
        }
    
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(reinterpret_cast<VulkanBuffer*>(view->GetDesc().Buffer)->GetVulkanBuffer());
        bufferInfo.setOffset(0);
        bufferInfo.setRange(VK_WHOLE_SIZE);
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(0);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eStorageBuffer);
        writeInfo.setBufferInfo(bufferInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
        return index;
    }
    
    int32 VulkanBindlessManager::WriteAccelerationStructure(vk::AccelerationStructureKHR as)
    {
        if (!m_ParentDevice->SupportsRaytracing()) return -1;

        int32 index = m_AccelerationStructureAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless acceleration structure descriptor - pool exhausted");
            return -1;
        }

        vk::WriteDescriptorSetAccelerationStructureKHR asInfo;
        asInfo.setAccelerationStructureCount(1);
        asInfo.setPAccelerationStructures(&as);

        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(2);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
        writeInfo.setDescriptorCount(1);
        writeInfo.setPNext(&asInfo);

        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
        return index;
    }

    int32 VulkanBindlessManager::WriteSampler(VulkanSampler* sampler)
    {
        int32 index = m_SamplerAllocator.Allocate();
        if (index == FreeList::INVALID) {
            TN_ERROR("Failed to allocate bindless sampler descriptor");
            return -1;
        }
    
        vk::DescriptorImageInfo imageInfo;
        imageInfo.setSampler(sampler->GetSampler());
    
        vk::WriteDescriptorSet writeInfo;
        writeInfo.setDstSet(m_Set);
        writeInfo.setDstBinding(1);
        writeInfo.setDstArrayElement(index);
        writeInfo.setDescriptorType(vk::DescriptorType::eSampler);
        writeInfo.setImageInfo(imageInfo);
        writeInfo.setDescriptorCount(1);
    
        m_ParentDevice->GetVulkanDevice().updateDescriptorSets(writeInfo, {});
        return index;
    }
}
