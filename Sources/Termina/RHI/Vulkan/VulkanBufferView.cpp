#include "VulkanBufferView.hpp"
#include "VulkanDevice.hpp"

namespace Termina {
    VulkanBufferView::VulkanBufferView(VulkanDevice* device, const BufferViewDesc& desc)
        : m_ParentDevice(device)
    {
        m_Desc = desc;
    
        switch (desc.Type) {
            case BufferViewType::CONSTANT: {
                m_BindlessIndex = m_ParentDevice->GetBindlessManager()->WriteBufferCBV(this);
                break;
            }
            case BufferViewType::SHADER_READ: {
                m_BindlessIndex = m_ParentDevice->GetBindlessManager()->WriteBufferSRV(this);
                break;
            }
            case BufferViewType::SHADER_WRITE: {
                m_BindlessIndex = m_ParentDevice->GetBindlessManager()->WriteBufferUAV(this);
                break;
            }
        }
    }
    
    VulkanBufferView::~VulkanBufferView()
    {
        m_ParentDevice->GetBindlessManager()->FreeResource(m_BindlessIndex);
    }
}
