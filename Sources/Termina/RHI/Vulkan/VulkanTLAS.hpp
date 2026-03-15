#pragma once

#include <Termina/RHI/TLAS.hpp>
#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanTLAS : public TLAS
    {
    public:
        VulkanTLAS(VulkanDevice* device, uint32 maxInstances);
        ~VulkanTLAS() override;

        void  Build(RenderContext* ctx,
                    const std::vector<TLASInstanceDesc>& instances,
                    RendererBuffer* scratch,
                    uint64 scratchOffset) override;

        int32 GetBindlessIndex() const override { return m_BindlessIndex; }

    private:
        VulkanDevice*                m_Device;
        uint32                       m_MaxInstances;

        RendererBuffer*              m_Buffer       = nullptr; // AS storage
        RendererBuffer*              m_InstanceBuf  = nullptr; // VkAccelerationStructureInstanceKHR array
        void*                        m_InstanceMapped = nullptr;

        vk::AccelerationStructureKHR m_AS;
        int32                        m_BindlessIndex = -1;
        bool                         m_Built = false; // first Build() allocates the AS
    };
}
