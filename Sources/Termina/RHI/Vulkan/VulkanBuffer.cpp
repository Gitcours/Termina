#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    VulkanBuffer::VulkanBuffer(VulkanDevice* device, const BufferDesc& desc)
        : m_Device(device)
    {
        m_Desc = desc;
        
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = desc.Size;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;
        // eShaderDeviceAddress is always enabled (VMA allocator uses eBufferDeviceAddress)
        bufferInfo.usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst
                          | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        if (Any(desc.Usage, BufferUsage::VERTEX)) {
            bufferInfo.usage |= vk::BufferUsageFlagBits::eVertexBuffer
                              | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        }
        if (Any(desc.Usage, BufferUsage::INDEX)) {
            bufferInfo.usage |= vk::BufferUsageFlagBits::eIndexBuffer
                              | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        }
        if (Any(desc.Usage, BufferUsage::CONSTANT)) bufferInfo.usage |= vk::BufferUsageFlagBits::eUniformBuffer;
        if (Any(desc.Usage, BufferUsage::INDIRECT_COMMANDS)) bufferInfo.usage |= vk::BufferUsageFlagBits::eIndirectBuffer;
        if (Any(desc.Usage, BufferUsage::ACCELERATION_STRUCTURE)) {
            bufferInfo.usage |= vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        }
    
        vma::AllocationCreateInfo allocInfo;
        memset(&allocInfo, 0, sizeof(allocInfo)); // Work around for Windows because for some reason minAlignment isn't in the C++ version of VMA
        allocInfo.usage = vma::MemoryUsage::eGpuOnly;
        if (Any(desc.Usage, BufferUsage::TRANSFER)) allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
        if (Any(desc.Usage, BufferUsage::READBACK)) allocInfo.usage = vma::MemoryUsage::eCpuToGpu;
        if (Any(desc.Usage, BufferUsage::CONSTANT)) allocInfo.usage = vma::MemoryUsage::eCpuOnly;
    
        vk::Result result = m_Device->GetVulkanAllocator().createBuffer(&bufferInfo, &allocInfo, &m_Buffer, &m_Allocation, &m_AllocationInfo);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to create Vulkan buffer");
        }
    }
    
    VulkanBuffer::~VulkanBuffer()
    {
        m_Device->GetVulkanAllocator().destroyBuffer(m_Buffer, m_Allocation);
    }
    
    void VulkanBuffer::SetName(const std::string& name)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.objectType = vk::ObjectType::eBuffer;
        nameInfo.objectHandle = reinterpret_cast<uint64>(static_cast<VkBuffer>(m_Buffer));
        nameInfo.pObjectName = name.c_str();
    
        m_Device->GetVulkanDevice().setDebugUtilsObjectNameEXT(nameInfo);
    }
    
    void* VulkanBuffer::Map()
    {
        void* data = nullptr;
        vk::Result result = m_Device->GetVulkanAllocator().mapMemory(m_Allocation, &data);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to map Vulkan buffer memory");
            return nullptr;
        }
        return data;
    }
    
    void VulkanBuffer::Unmap()
    {
        m_Device->GetVulkanAllocator().unmapMemory(m_Allocation);
    }
    
    uint64 VulkanBuffer::GetGPUAddress() const
    {
        vk::BufferDeviceAddressInfo addressInfo{};
        addressInfo.buffer = m_Buffer;
    
        return m_Device->GetVulkanDevice().getBufferAddress(addressInfo);
    }
}
