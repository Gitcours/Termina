#pragma once

#include <Termina/Core/Common.hpp>

namespace Termina {
    enum class SamplerAddressMode
    {
        WRAP,
        MIRROR,
        CLAMP,
        BORDER
    };

    enum class SamplerFilter
    {
        POINT,
        LINEAR,
        ANISOTROPIC
    };

    enum class SamplerComparisonFunc
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_EQUAL,
        ALWAYS
    };

    struct SamplerDesc
    {
        SamplerAddressMode Address = SamplerAddressMode::WRAP;
        SamplerFilter Filter = SamplerFilter::LINEAR;
        SamplerComparisonFunc ComparisonFunc = SamplerComparisonFunc::NEVER;
        bool UseMips = false;

        SamplerDesc& SetAddress(SamplerAddressMode address)
        {
            Address = address;
            return *this;
        }

        SamplerDesc& SetFilter(SamplerFilter filter)
        {
            Filter = filter;
            return *this;
        }

        SamplerDesc& SetComparisonFunc(SamplerComparisonFunc func)
        {
            ComparisonFunc = func;
            return *this;
        }

        SamplerDesc& EnableMips(bool enable = true)
        {
            UseMips = enable;
            return *this;
        }
    };

    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        virtual int32 GetBindlessHandle() const = 0;
        SamplerDesc GetDesc() const { return m_Desc; }
    protected:
        SamplerDesc m_Desc;
    };
}
