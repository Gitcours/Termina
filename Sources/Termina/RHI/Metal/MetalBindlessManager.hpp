#pragma once

#include <Termina/Core/FreeList.hpp>

#include <Metal/Metal.h>

#include "MetalBuffer.hpp"

namespace Termina {
    class MetalDevice;
    class MetalTextureView;
    class MetalSampler;
    class MetalBufferView;

    class MetalBindlessManager
    {
    public:
        MetalBindlessManager(MetalDevice* device);
        ~MetalBindlessManager();

        uint32 WriteTextureView(MetalTextureView* view);
        uint32 WriteBufferView(MetalBufferView* view);
        uint32 WriteSampler(MetalSampler* sampler);
        // handleBuffer must contain the AS GPU resource ID (uint64) at offset 0.
        // MSC translates RaytracingAccelerationStructure into a buffer pointer dereference.
        uint32 WriteAccelerationStructure(MetalBuffer* handleBuffer);
        void Free(uint32 index);
        void FreeSampler(uint32 index);

        id<MTLBuffer> GetHandle() const { return m_Handle->GetBuffer(); }
        id<MTLBuffer> GetSamplerHandle() const { return m_SamplerHandle->GetBuffer(); }
    private:
        MetalBuffer* m_Handle;
        void* m_MappedData;

        MetalBuffer* m_SamplerHandle;
        void* m_MappedSamplerData;

        FreeList m_FreeList;
        FreeList m_SamplerFreeList;
    };
}
