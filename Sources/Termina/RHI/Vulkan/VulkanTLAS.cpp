#include "VulkanTLAS.hpp"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanBLAS.hpp"
#include "VulkanBindlessManager.hpp"
#include "VulkanRenderContext.hpp"

#include <Termina/Core/Logger.hpp>

#include <cstring>

namespace Termina {

    VulkanTLAS::VulkanTLAS(VulkanDevice* device, uint32 maxInstances)
        : m_Device(device)
        , m_MaxInstances(maxInstances)
    {
        // Pre-allocate the instance buffer (host-visible, persistently mapped)
        m_InstanceBuf = device->CreateBuffer(BufferDesc()
            .SetSize(maxInstances * sizeof(VkAccelerationStructureInstanceKHR))
            .SetStride(sizeof(VkAccelerationStructureInstanceKHR))
            .SetUsage(BufferUsage::ACCELERATION_STRUCTURE | BufferUsage::TRANSFER));
        m_InstanceMapped = m_InstanceBuf->Map();
    }

    VulkanTLAS::~VulkanTLAS()
    {
        if (m_AS) {
            m_Device->GetVulkanDevice().destroyAccelerationStructureKHR(m_AS);
        }
        if (m_InstanceMapped) m_InstanceBuf->Unmap();
        delete m_InstanceBuf;
        delete m_Buffer;
    }

    void VulkanTLAS::Build(RenderContext* ctx,
                           const std::vector<TLASInstanceDesc>& instances,
                           RendererBuffer* scratch,
                           uint64 scratchOffset)
    {
        vk::Device vkDevice = m_Device->GetVulkanDevice();
        vk::CommandBuffer cmd = static_cast<VulkanRenderContext*>(ctx)->GetCommandBuffer();

        const uint32 instanceCount = static_cast<uint32>(
            instances.size() > m_MaxInstances ? m_MaxInstances : instances.size());

        // Fill instance buffer
        VkAccelerationStructureInstanceKHR* instanceData =
            static_cast<VkAccelerationStructureInstanceKHR*>(m_InstanceMapped);

        for (uint32 i = 0; i < instanceCount; ++i)
        {
            const TLASInstanceDesc& desc = instances[i];
            VulkanBLAS* vkBlas = static_cast<VulkanBLAS*>(desc.BLASObject);

            VkAccelerationStructureInstanceKHR& inst = instanceData[i];
            memset(&inst, 0, sizeof(inst));

            // Pack 3x4 row-major transform (GLM is column-major)
            const glm::mat4& m = desc.Transform;
            inst.transform.matrix[0][0] = m[0][0]; inst.transform.matrix[0][1] = m[1][0]; inst.transform.matrix[0][2] = m[2][0]; inst.transform.matrix[0][3] = m[3][0];
            inst.transform.matrix[1][0] = m[0][1]; inst.transform.matrix[1][1] = m[1][1]; inst.transform.matrix[1][2] = m[2][1]; inst.transform.matrix[1][3] = m[3][1];
            inst.transform.matrix[2][0] = m[0][2]; inst.transform.matrix[2][1] = m[1][2]; inst.transform.matrix[2][2] = m[2][2]; inst.transform.matrix[2][3] = m[3][2];

            inst.instanceCustomIndex                    = desc.InstanceID & 0xFFFFFF;
            inst.mask                                   = desc.Mask;
            inst.instanceShaderBindingTableRecordOffset = 0;
            inst.flags = desc.Opaque
                ? VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR
                : VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
            inst.accelerationStructureReference = vkBlas->GetGPUAddress();
        }

        // On first build, query sizes and allocate the TLAS buffer
        vk::AccelerationStructureGeometryInstancesDataKHR instancesData;
        instancesData.setArrayOfPointers(VK_FALSE);
        instancesData.setData(vk::DeviceOrHostAddressConstKHR(
            static_cast<VulkanBuffer*>(m_InstanceBuf)->GetGPUAddress()));

        vk::AccelerationStructureGeometryKHR geometry;
        geometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
        geometry.setGeometry(vk::AccelerationStructureGeometryDataKHR(instancesData));

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        buildInfo.setGeometries(geometry);

        if (!m_Built)
        {
            std::vector<uint32> maxPrimCounts = { m_MaxInstances };
            vk::AccelerationStructureBuildSizesInfoKHR sizeInfo =
                vkDevice.getAccelerationStructureBuildSizesKHR(
                    vk::AccelerationStructureBuildTypeKHR::eDevice,
                    buildInfo,
                    maxPrimCounts);

            m_Buffer = m_Device->CreateBuffer(BufferDesc()
                .SetSize(sizeInfo.accelerationStructureSize)
                .SetUsage(BufferUsage::ACCELERATION_STRUCTURE));

            vk::AccelerationStructureCreateInfoKHR asCreateInfo;
            asCreateInfo.setBuffer(static_cast<VulkanBuffer*>(m_Buffer)->GetVulkanBuffer());
            asCreateInfo.setSize(sizeInfo.accelerationStructureSize);
            asCreateInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);

            m_AS = vkDevice.createAccelerationStructureKHR(asCreateInfo);
            if (!m_AS) {
                TN_ERROR("Failed to create Vulkan TLAS");
                return;
            }

            m_Built = true;
        }

        buildInfo.setDstAccelerationStructure(m_AS);
        buildInfo.setScratchData(vk::DeviceOrHostAddressKHR(
            static_cast<VulkanBuffer*>(scratch)->GetGPUAddress() + scratchOffset));

        vk::AccelerationStructureBuildRangeInfoKHR range;
        range.setPrimitiveCount(instanceCount);
        range.setPrimitiveOffset(0);
        range.setFirstVertex(0);
        range.setTransformOffset(0);

        const vk::AccelerationStructureBuildRangeInfoKHR* pRange = &range;
        cmd.buildAccelerationStructuresKHR(1, &buildInfo, &pRange);

        // Write bindless descriptor (overwrites same slot each frame)
        if (m_BindlessIndex >= 0) {
            m_Device->GetBindlessManager()->FreeAccelerationStructure(m_BindlessIndex);
        }
        m_BindlessIndex = m_Device->GetBindlessManager()->WriteAccelerationStructure(m_AS);
    }

} // namespace Termina
