#pragma once

#include <Termina/RHI/Texture.hpp>

#include <vulkan/vulkan.hpp>
#include <VMA/vk_mem_alloc.hpp>

namespace Termina {
    class VulkanTexture : public RendererTexture
    {
    public:
        VulkanTexture() = default;
        VulkanTexture(const TextureDesc& desc, vk::Device device, vma::Allocator allocator);
        ~VulkanTexture() override;
    
        void SetName(const std::string& name) override;
    
        vma::Allocation GetAllocation() const { return m_Allocation; }
        vk::Image GetImage() const { return m_Image; }
    private:
        friend class VulkanSurface;
    
        vma::Allocation m_Allocation;
        vma::AllocationInfo m_AllocationInfo;
        vk::Image m_Image;
    
        vk::Device m_ParentDevice;
        vma::Allocator m_ParentAllocator;
    
        bool m_ShouldDestroy = true;
    };
    
    inline vk::Format ConvertTextureFormatToVulkan(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::UNDEFINED:     return vk::Format::eUndefined;
        case TextureFormat::RGBA8_UNORM:   return vk::Format::eR8G8B8A8Unorm;
        case TextureFormat::RGBA8_SRGB:    return vk::Format::eR8G8B8A8Srgb;
        case TextureFormat::BGRA8_UNORM:   return vk::Format::eB8G8R8A8Unorm;
        case TextureFormat::ASTC6X6_SRGB:  return vk::Format::eAstc6x6SrgbBlock;
        case TextureFormat::ASTC6X6_UNORM: return vk::Format::eAstc6x6UnormBlock;
        case TextureFormat::BC7_SRGB:      return vk::Format::eBc7SrgbBlock;
        case TextureFormat::BC7_UNORM:     return vk::Format::eBc7UnormBlock;
        case TextureFormat::D32_FLOAT:     return vk::Format::eD32Sfloat;
        default:                           return vk::Format::eUndefined;
        }
    }
    
    inline vk::ImageUsageFlags ConvertTextureUsageToVulkan(TextureUsage usage)
    {
        vk::ImageUsageFlags flags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
        if (Any(usage, TextureUsage::RENDER_TARGET))
            flags |= vk::ImageUsageFlagBits::eColorAttachment;
        if (Any(usage, TextureUsage::DEPTH_TARGET))
            flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
        if (Any(usage, TextureUsage::SHADER_READ))
            flags |= vk::ImageUsageFlagBits::eSampled;
        if (Any(usage, TextureUsage::SHADER_WRITE))
            flags |= vk::ImageUsageFlagBits::eStorage;
        return flags;
    }
}
