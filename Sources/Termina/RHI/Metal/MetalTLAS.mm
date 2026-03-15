#include "MetalTLAS.hpp"
#include "MetalDevice.hpp"
#include "MetalBuffer.hpp"
#include "MetalBLAS.hpp"
#include "MetalBindlessManager.hpp"
#include "MetalRenderContext.hpp"

#include <Termina/Core/Logger.hpp>

#include <cstring>

namespace Termina {

    MetalTLAS::MetalTLAS(MetalDevice* device, uint32 maxInstances)
        : m_Device(device)
        , m_MaxInstances(maxInstances)
    {
        // Instance buffer: one MTLAccelerationStructureInstanceDescriptor per instance (shared memory)
        m_InstanceBuf = static_cast<MetalBuffer*>(device->CreateBuffer(BufferDesc()
            .SetSize(maxInstances * sizeof(MTLAccelerationStructureInstanceDescriptor))
            .SetStride(sizeof(MTLAccelerationStructureInstanceDescriptor))
            .SetUsage(BufferUsage::TRANSFER)));
        m_InstanceMapped = m_InstanceBuf->Map();

        // 8-byte handle buffer: holds the AS GPU resource ID so MSC can dereference it as a buffer
        m_HandleBuf = static_cast<MetalBuffer*>(device->CreateBuffer(BufferDesc()
            .SetSize(sizeof(uint64_t))
            .SetUsage(BufferUsage::TRANSFER | BufferUsage::SHADER_READ)));
        m_HandleMapped = m_HandleBuf->Map();

        // Register once in the descriptor heap; we'll update the buffer contents each frame
        m_BindlessIndex = static_cast<int32>(
            device->GetBindlessManager()->WriteAccelerationStructure(m_HandleBuf));
    }

    MetalTLAS::~MetalTLAS()
    {
        if (m_BindlessIndex >= 0)
            m_Device->GetBindlessManager()->Free(m_BindlessIndex);

        if (m_InstanceMapped) m_InstanceBuf->Unmap();
        if (m_HandleMapped)   m_HandleBuf->Unmap();

        delete m_InstanceBuf;
        delete m_HandleBuf;

        mAccelerationStructure = nil;
    }

    void MetalTLAS::Build(RenderContext* ctx,
                          const std::vector<TLASInstanceDesc>& instances,
                          RendererBuffer* /*scratch*/,
                          uint64 /*scratchOffset*/)
    {
        id<MTLDevice> mtlDevice = m_Device->GetDevice();

        const uint32 instanceCount = static_cast<uint32>(
            instances.size() > m_MaxInstances ? m_MaxInstances : instances.size());

        // Fill instance buffer
        MTLAccelerationStructureInstanceDescriptor* instanceData =
            static_cast<MTLAccelerationStructureInstanceDescriptor*>(m_InstanceMapped);

        for (uint32 i = 0; i < instanceCount; ++i)
        {
            const TLASInstanceDesc& desc = instances[i];
            MetalBLAS* mtlBlas = static_cast<MetalBLAS*>(desc.BLASObject);

            MTLAccelerationStructureInstanceDescriptor& inst = instanceData[i];
            memset(&inst, 0, sizeof(inst));

            // Pack 3x4 row-major transform from GLM column-major mat4
            const glm::mat4& m = desc.Transform;
            inst.transformationMatrix.columns[0] = { m[0][0], m[0][1], m[0][2] };
            inst.transformationMatrix.columns[1] = { m[1][0], m[1][1], m[1][2] };
            inst.transformationMatrix.columns[2] = { m[2][0], m[2][1], m[2][2] };
            inst.transformationMatrix.columns[3] = { m[3][0], m[3][1], m[3][2] };

            inst.options                  = desc.Opaque
                ? MTLAccelerationStructureInstanceOptionOpaque
                : MTLAccelerationStructureInstanceOptionNone;
            inst.mask                     = desc.Mask;
            inst.intersectionFunctionTableOffset = 0;
            inst.accelerationStructureIndex      = i; // index into instancedAccelerationStructures array below
        }

        // Gather the BLAS objects
        NSMutableArray<id<MTLAccelerationStructure>>* blasArray = [NSMutableArray array];
        for (uint32 i = 0; i < instanceCount; ++i)
        {
            MetalBLAS* mtlBlas = static_cast<MetalBLAS*>(instances[i].BLASObject);
            [blasArray addObject:mtlBlas->GetAccelerationStructure()];
        }

        MTLInstanceAccelerationStructureDescriptor* asDesc =
            [MTLInstanceAccelerationStructureDescriptor descriptor];
        asDesc.instancedAccelerationStructures = blasArray;
        asDesc.instanceCount                   = instanceCount;
        asDesc.instanceDescriptorBuffer        = m_InstanceBuf->GetBuffer();
        asDesc.instanceDescriptorBufferOffset  = 0;
        asDesc.instanceDescriptorStride        = sizeof(MTLAccelerationStructureInstanceDescriptor);

        MTLAccelerationStructureSizes sizes = [mtlDevice accelerationStructureSizesWithDescriptor:asDesc];

        if (!m_Built)
        {
            mAccelerationStructure = [mtlDevice newAccelerationStructureWithSize:sizes.accelerationStructureSize];
            if (!mAccelerationStructure) {
                TN_ERROR("Failed to allocate Metal TLAS");
                return;
            }
            m_Built = true;
        }

        id<MTLBuffer> scratch = [mtlDevice newBufferWithLength:sizes.buildScratchBufferSize
                                                       options:MTLResourceStorageModePrivate];

        // Use a dedicated command buffer so the build completes before the compute pass
        id<MTLCommandBuffer> cmdBuf = [m_Device->GetCommandQueue() commandBuffer];
        id<MTLAccelerationStructureCommandEncoder> encoder =
            [cmdBuf accelerationStructureCommandEncoder];

        [encoder buildAccelerationStructure:mAccelerationStructure
                                 descriptor:asDesc
                              scratchBuffer:scratch
                        scratchBufferOffset:0];
        [encoder endEncoding];
        [cmdBuf commit];
        [cmdBuf waitUntilCompleted];

        // Update the handle buffer with the new AS GPU resource ID
        uint64_t gpuID = mAccelerationStructure.gpuResourceID._impl;
        memcpy(m_HandleMapped, &gpuID, sizeof(uint64_t));
    }

} // namespace Termina
