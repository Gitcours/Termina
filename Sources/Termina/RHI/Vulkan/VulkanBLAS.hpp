#pragma once

#include <Termina/RHI/BLAS.hpp>
#include <vulkan/vulkan.hpp>

namespace Termina {
    class VulkanDevice;

    class VulkanBLAS : public BLAS
    {
    public:
        VulkanBLAS(VulkanDevice* device, const BLASDesc& desc);
        ~VulkanBLAS() override;

        uint64 GetGPUAddress()    const override;
        int32  GetBindlessIndex() const override { return -1; } // BLAS is not bound individually

        vk::AccelerationStructureKHR GetVkAccelerationStructure() const { return m_AS; }

    private:
        VulkanDevice*                m_Device;
        RendererBuffer*              m_Buffer = nullptr; // AS storage buffer
        vk::AccelerationStructureKHR m_AS;
    };
}
