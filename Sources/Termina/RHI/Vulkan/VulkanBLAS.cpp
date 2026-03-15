#include "VulkanBLAS.hpp"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {

    VulkanBLAS::VulkanBLAS(VulkanDevice* device, const BLASDesc& desc)
        : m_Device(device)
    {
        m_Desc = desc;

        vk::Device vkDevice = device->GetVulkanDevice();

        // Build one geometry entry per BLASGeometry
        std::vector<vk::AccelerationStructureGeometryKHR> geometries;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRanges;
        std::vector<uint32> maxPrimCounts;

        const uint64 vertexBufferAddress = static_cast<VulkanBuffer*>(desc.VertexBuffer)->GetGPUAddress();
        const uint64 indexBufferAddress  = static_cast<VulkanBuffer*>(desc.IndexBuffer)->GetGPUAddress();

        for (const BLASGeometry& geo : desc.Geometries)
        {
            vk::AccelerationStructureGeometryTrianglesDataKHR triangles;
            triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat); // Position is float3 at start of ModelVertex
            triangles.setVertexData(vk::DeviceOrHostAddressConstKHR(vertexBufferAddress
                + geo.VertexOffset * sizeof(float) * 12)); // stride of ModelVertex (pos+norm+uv+tan = 12 floats)
            triangles.setVertexStride(sizeof(float) * 12);  // ModelVertex stride: 3+3+2+4 = 12 floats = 48 bytes
            triangles.setMaxVertex(geo.VertexCount > 0 ? geo.VertexCount - 1 : 0);
            triangles.setIndexType(vk::IndexType::eUint32);
            triangles.setIndexData(vk::DeviceOrHostAddressConstKHR(indexBufferAddress
                + geo.IndexOffset * sizeof(uint32)));

            vk::AccelerationStructureGeometryKHR geometry;
            geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles);
            geometry.setGeometry(vk::AccelerationStructureGeometryDataKHR(triangles));
            geometry.setFlags(geo.Opaque ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagBitsKHR{});

            vk::AccelerationStructureBuildRangeInfoKHR range;
            range.setPrimitiveCount(geo.IndexCount / 3);
            range.setPrimitiveOffset(0); // offset already baked into IndexData address
            range.setFirstVertex(0);     // offset already baked into VertexData address
            range.setTransformOffset(0);

            geometries.push_back(geometry);
            buildRanges.push_back(range);
            maxPrimCounts.push_back(geo.IndexCount / 3);
        }

        // Query sizes
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo;
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        buildInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        buildInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        buildInfo.setGeometries(geometries);

        vk::AccelerationStructureBuildSizesInfoKHR sizeInfo =
            vkDevice.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                buildInfo,
                maxPrimCounts);

        // Allocate AS storage buffer
        m_Buffer = device->CreateBuffer(BufferDesc()
            .SetSize(sizeInfo.accelerationStructureSize)
            .SetUsage(BufferUsage::ACCELERATION_STRUCTURE));

        // Create the acceleration structure
        vk::AccelerationStructureCreateInfoKHR asCreateInfo;
        asCreateInfo.setBuffer(static_cast<VulkanBuffer*>(m_Buffer)->GetVulkanBuffer());
        asCreateInfo.setSize(sizeInfo.accelerationStructureSize);
        asCreateInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel);

        m_AS = vkDevice.createAccelerationStructureKHR(asCreateInfo);
        if (!m_AS) {
            TN_ERROR("Failed to create Vulkan BLAS");
            return;
        }

        // Allocate scratch buffer (temporary, destroyed after build)
        RendererBuffer* scratch = device->CreateBuffer(BufferDesc()
            .SetSize(sizeInfo.buildScratchSize)
            .SetUsage(BufferUsage::ACCELERATION_STRUCTURE | BufferUsage::SHADER_WRITE));

        // Record and submit build command
        vk::CommandBufferAllocateInfo cmdAllocInfo;
        cmdAllocInfo.setCommandPool(device->GetCommandPool());
        cmdAllocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
        cmdAllocInfo.setCommandBufferCount(1);

        vk::CommandBuffer cmd = vkDevice.allocateCommandBuffers(cmdAllocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(beginInfo);

        buildInfo.setDstAccelerationStructure(m_AS);
        buildInfo.setScratchData(vk::DeviceOrHostAddressKHR(
            static_cast<VulkanBuffer*>(scratch)->GetGPUAddress()));

        const vk::AccelerationStructureBuildRangeInfoKHR* pRanges = buildRanges.data();
        cmd.buildAccelerationStructuresKHR(1, &buildInfo, &pRanges);

        cmd.end();

        vk::SubmitInfo submitInfo;
        submitInfo.setCommandBuffers(cmd);
        device->GetMainQueue().submit(submitInfo, nullptr);
        device->GetMainQueue().waitIdle();

        vkDevice.freeCommandBuffers(device->GetCommandPool(), cmd);

        delete scratch;
    }

    VulkanBLAS::~VulkanBLAS()
    {
        if (m_AS) {
            m_Device->GetVulkanDevice().destroyAccelerationStructureKHR(m_AS);
        }
        delete m_Buffer;
    }

    uint64 VulkanBLAS::GetGPUAddress() const
    {
        vk::AccelerationStructureDeviceAddressInfoKHR addrInfo;
        addrInfo.setAccelerationStructure(m_AS);
        return m_Device->GetVulkanDevice().getAccelerationStructureAddressKHR(addrInfo);
    }

} // namespace Termina
