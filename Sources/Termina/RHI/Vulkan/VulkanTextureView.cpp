#include "VulkanTextureView.hpp"
#include "VulkanDevice.hpp"
#include "VulkanTexture.hpp"

namespace Termina {
    VulkanTextureView::VulkanTextureView(VulkanDevice* device, const TextureViewDesc& desc)
        : m_Device(device)
    {
        m_Desc = desc;
    
        TextureDesc textureDesc = desc.Texture->GetDesc();
    
        vk::ImageViewCreateInfo viewInfo;
        viewInfo.image = static_cast<VulkanTexture*>(desc.Texture)->GetImage();
        viewInfo.viewType = GetVulkanImageViewType(desc.Dimension);
        viewInfo.format = desc.Format != TextureFormat::UNDEFINED ? ConvertTextureFormatToVulkan(desc.Format) : ConvertTextureFormatToVulkan(desc.Texture->GetDesc().Format);
        viewInfo.subresourceRange.aspectMask = Any(textureDesc.Usage, TextureUsage::DEPTH_TARGET) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = static_cast<uint32>(desc.MipStart);
        viewInfo.subresourceRange.levelCount = desc.MipEnd == VIEW_ALL_MIP_LEVELS ? VK_REMAINING_MIP_LEVELS : static_cast<uint32>(desc.MipEnd - desc.MipStart);
        viewInfo.subresourceRange.baseArrayLayer = static_cast<uint32>(desc.ArrayLayerStart);
        viewInfo.subresourceRange.layerCount = desc.ArrayLayerEnd == VIEW_ALL_ARRAY_LAYERS ? VK_REMAINING_ARRAY_LAYERS : static_cast<uint32>(desc.ArrayLayerEnd - desc.ArrayLayerStart);
    
        m_ImageView = m_Device->GetVulkanDevice().createImageView(viewInfo);
    
        if (desc.Type == TextureViewType::SHADER_READ) {
            m_BindlessIndex = m_Device->GetBindlessManager()->WriteTextureSRV(this);
        } else if (desc.Type == TextureViewType::SHADER_WRITE) {
            m_BindlessIndex = m_Device->GetBindlessManager()->WriteTextureUAV(this);
        }
    }
    
    VulkanTextureView::~VulkanTextureView()
    {
        m_Device->GetVulkanDevice().destroyImageView(m_ImageView);
    
        if (m_BindlessIndex != -1) {
            m_Device->GetBindlessManager()->FreeResource(m_BindlessIndex);
        }
    }
}
