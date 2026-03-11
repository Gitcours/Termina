#pragma once

#include <Termina/RHI/BufferView.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanBufferView : public BufferView
    {
    public:
        VulkanBufferView(VulkanDevice* parentDevice, const BufferViewDesc& desc);
        ~VulkanBufferView() override;
    
        int32 GetBindlessHandle() const override { return m_BindlessIndex; }
    private:
        VulkanDevice* m_ParentDevice = nullptr;
    
        int32 m_BindlessIndex = -1;
    };
}
