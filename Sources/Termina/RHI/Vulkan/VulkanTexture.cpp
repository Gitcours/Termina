#include "VulkanTexture.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    VulkanTexture::VulkanTexture(const TextureDesc& desc, vk::Device device, vma::Allocator allocator)
        : m_ParentDevice(device)
        , m_ParentAllocator(allocator)
        , m_ShouldDestroy(true)
    {
        m_Desc = desc;
    
        vk::ImageCreateInfo imageInfo;
        imageInfo.setArrayLayers(desc.ArrayLayers);
        imageInfo.setMipLevels(desc.MipLevels);
        imageInfo.setExtent({ desc.Width, desc.Height, desc.Depth });
        imageInfo.setFormat(ConvertTextureFormatToVulkan(desc.Format));
        imageInfo.setImageType(desc.Depth > 1 ? vk::ImageType::e3D : vk::ImageType::e2D);
        imageInfo.setInitialLayout(vk::ImageLayout::eUndefined);
        imageInfo.setSharingMode(vk::SharingMode::eExclusive);
        imageInfo.setTiling(vk::ImageTiling::eOptimal);
        imageInfo.setUsage(ConvertTextureUsageToVulkan(desc.Usage));
        imageInfo.setFlags(desc.IsCubeMap ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags{});
    
        vma::AllocationCreateInfo allocInfo = {};
        allocInfo.usage = vma::MemoryUsage::eGpuOnly;
    
        vk::Result result = allocator.createImage(&imageInfo, &allocInfo, &m_Image, &m_Allocation, &m_AllocationInfo);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to create Vulkan texture image!");
        }
    }
    
    VulkanTexture::~VulkanTexture()
    {
        if (m_ShouldDestroy) {
            m_ParentAllocator.destroyImage(m_Image, m_Allocation);
        }
    }
    
    void VulkanTexture::SetName(const std::string& name)
    {
        vk::DebugUtilsObjectNameInfoEXT nameInfo;
        nameInfo.setObjectType(vk::ObjectType::eImage);
        nameInfo.setObjectHandle(reinterpret_cast<uint64_t>(static_cast<VkImage>(m_Image)));
        nameInfo.setPObjectName(name.c_str());
    
        m_ParentDevice.setDebugUtilsObjectNameEXT(nameInfo);
    }
}
