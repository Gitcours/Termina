#include "VulkanSampler.hpp"
#include "VulkanDevice.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    VulkanSampler::VulkanSampler(VulkanDevice* device, SamplerDesc desc)
        : m_ParentDevice(device)
    {
        m_Desc = desc;
    
        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.addressModeU = ConvertAddressMode(desc.Address);
        samplerInfo.addressModeV = ConvertAddressMode(desc.Address);
        samplerInfo.addressModeW = ConvertAddressMode(desc.Address);
        samplerInfo.magFilter = ConvertFilter(desc.Filter);
        samplerInfo.minFilter = ConvertFilter(desc.Filter);
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = desc.Filter == SamplerFilter::ANISOTROPIC ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = 16;
        samplerInfo.compareEnable = desc.ComparisonFunc != SamplerComparisonFunc::NEVER ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = ConvertComparisonFunc(desc.ComparisonFunc);
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = desc.UseMips ? 16.0f : 0.0f;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.flags = {};
    
        vk::Result result = m_ParentDevice->GetVulkanDevice().createSampler(&samplerInfo, nullptr, &m_Sampler);
        if (result != vk::Result::eSuccess) {
            TN_ERROR("Failed to create Vulkan sampler");
        }
    
        m_BindlessHandle = m_ParentDevice->GetBindlessManager()->WriteSampler(this);
    }
    
    VulkanSampler::~VulkanSampler()
    {
        if (m_Sampler) {
            m_ParentDevice->GetBindlessManager()->FreeSampler(m_BindlessHandle);
            m_ParentDevice->GetVulkanDevice().destroySampler(m_Sampler, nullptr);
            m_Sampler = nullptr;
        }
    }
}
