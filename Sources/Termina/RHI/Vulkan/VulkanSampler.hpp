#pragma once

#include <Termina/RHI/Sampler.hpp>

#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanSampler : public Sampler
    {
    public:
        VulkanSampler(VulkanDevice* device, SamplerDesc desc);
        ~VulkanSampler();
    
        vk::Sampler GetSampler() { return m_Sampler; }
        int32 GetBindlessHandle() const override { return m_BindlessHandle; }
    private:
        VulkanDevice* m_ParentDevice;
    
        vk::Sampler m_Sampler;
        int32 m_BindlessHandle = -1;
    };
    
    inline vk::SamplerAddressMode ConvertAddressMode(SamplerAddressMode mode)
    {
        switch (mode) {
        case SamplerAddressMode::WRAP:
            return vk::SamplerAddressMode::eRepeat;
        case SamplerAddressMode::MIRROR:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case SamplerAddressMode::CLAMP:
            return vk::SamplerAddressMode::eClampToEdge;
        case SamplerAddressMode::BORDER:
            return vk::SamplerAddressMode::eClampToBorder;
        default:
            return vk::SamplerAddressMode::eRepeat;
        }
    }
    
    inline vk::Filter ConvertFilter(SamplerFilter filter)
    {
        switch (filter) {
        case SamplerFilter::POINT:
            return vk::Filter::eNearest;
        case SamplerFilter::LINEAR:
        case SamplerFilter::ANISOTROPIC:
            return vk::Filter::eLinear;
        default:
            return vk::Filter::eNearest;
        }
    }
    
    inline vk::CompareOp ConvertComparisonFunc(SamplerComparisonFunc func)
    {
        switch (func) {
        case SamplerComparisonFunc::NEVER:
            return vk::CompareOp::eNever;
        case SamplerComparisonFunc::LESS:
            return vk::CompareOp::eLess;
        case SamplerComparisonFunc::EQUAL:
            return vk::CompareOp::eEqual;
        case SamplerComparisonFunc::LESS_EQUAL:
            return vk::CompareOp::eLessOrEqual;
        case SamplerComparisonFunc::GREATER:
            return vk::CompareOp::eGreater;
        case SamplerComparisonFunc::NOT_EQUAL:
            return vk::CompareOp::eNotEqual;
        case SamplerComparisonFunc::GREATER_EQUAL:
            return vk::CompareOp::eGreaterOrEqual;
        case SamplerComparisonFunc::ALWAYS:
            return vk::CompareOp::eAlways;
        default:
            return vk::CompareOp::eAlways;
        }
    }
}
