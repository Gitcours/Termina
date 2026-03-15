#include "MetalBLAS.hpp"
#include "MetalDevice.hpp"
#include "MetalBuffer.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {

    MetalBLAS::MetalBLAS(MetalDevice* device, const BLASDesc& desc)
        : m_ParentDevice(device)
    {
        m_Desc = desc;

        id<MTLDevice> mtlDevice = device->GetDevice();

        NSMutableArray<MTLAccelerationStructureTriangleGeometryDescriptor*>* geometries =
            [NSMutableArray array];

        id<MTLBuffer> vertexBuffer = static_cast<MetalBuffer*>(desc.VertexBuffer)->GetBuffer();
        id<MTLBuffer> indexBuffer  = static_cast<MetalBuffer*>(desc.IndexBuffer)->GetBuffer();

        // sizeof(ModelVertex) = float3+float3+float2+float4 = 12 floats = 48 bytes
        constexpr NSUInteger kVertexStride = 48;

        for (const BLASGeometry& geo : desc.Geometries)
        {
            MTLAccelerationStructureTriangleGeometryDescriptor* geoDesc =
                [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

            geoDesc.vertexBuffer       = vertexBuffer;
            geoDesc.vertexBufferOffset = geo.VertexOffset * kVertexStride;
            geoDesc.vertexStride       = kVertexStride;
            geoDesc.vertexFormat       = MTLAttributeFormatFloat3;

            geoDesc.indexBuffer        = indexBuffer;
            geoDesc.indexBufferOffset  = geo.IndexOffset * sizeof(uint32_t);
            geoDesc.indexType          = MTLIndexTypeUInt32;

            geoDesc.triangleCount      = geo.IndexCount / 3;
            geoDesc.opaque             = geo.Opaque ? YES : NO;

            [geometries addObject:geoDesc];
        }

        MTLPrimitiveAccelerationStructureDescriptor* asDesc =
            [MTLPrimitiveAccelerationStructureDescriptor descriptor];
        asDesc.geometryDescriptors = geometries;

        MTLAccelerationStructureSizes sizes = [mtlDevice accelerationStructureSizesWithDescriptor:asDesc];

        mAccelerationStructure = [mtlDevice newAccelerationStructureWithSize:sizes.accelerationStructureSize];
        if (!mAccelerationStructure) {
            TN_ERROR("Failed to allocate Metal BLAS");
            return;
        }

        id<MTLBuffer> scratch = [mtlDevice newBufferWithLength:sizes.buildScratchBufferSize
                                                       options:MTLResourceStorageModePrivate];

        id<MTLCommandBuffer> cmdBuf = [device->GetCommandQueue() commandBuffer];
        id<MTLAccelerationStructureCommandEncoder> encoder =
            [cmdBuf accelerationStructureCommandEncoder];

        [encoder buildAccelerationStructure:mAccelerationStructure
                                 descriptor:asDesc
                              scratchBuffer:scratch
                        scratchBufferOffset:0];
        [encoder endEncoding];
        [cmdBuf commit];
        [cmdBuf waitUntilCompleted];
    }

    MetalBLAS::~MetalBLAS()
    {
        mAccelerationStructure = nil;
    }

    uint64 MetalBLAS::GetGPUAddress() const
    {
        if (mAccelerationStructure)
            return mAccelerationStructure.gpuResourceID._impl;
        return 0;
    }

    int32 MetalBLAS::GetBindlessIndex() const
    {
        return -1;
    }

} // namespace Termina
