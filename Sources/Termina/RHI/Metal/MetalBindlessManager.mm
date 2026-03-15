#include "MetalBindlessManager.hpp"
#include "MetalDevice.hpp"
#include "MetalTextureView.hpp"
#include "MetalSampler.hpp"
#include "MetalBufferView.hpp"

#include <MetalShaderConverter/metal_irconverter_runtime.h>

namespace Termina {
    constexpr uint MTL_BINDLESS_MAX_RESOURCES = 1'000'000;
    constexpr uint MTL_BINDLESS_MAX_SAMPLERS = 2048;

    MetalBindlessManager::MetalBindlessManager(MetalDevice* device)
        : m_FreeList(MTL_BINDLESS_MAX_RESOURCES), m_SamplerFreeList(MTL_BINDLESS_MAX_SAMPLERS)
    {
        m_Handle = (MetalBuffer*)device->CreateBuffer(BufferDesc().SetSize(MTL_BINDLESS_MAX_RESOURCES * sizeof(IRDescriptorTableEntry))
                                                      .SetStride(sizeof(IRDescriptorTableEntry))
                                                      .SetUsage(BufferUsage::SHADER_READ));
        m_Handle->SetName("Resource Descriptor Heap");
        m_SamplerHandle = (MetalBuffer*)device->CreateBuffer(BufferDesc().SetSize(MTL_BINDLESS_MAX_SAMPLERS * sizeof(IRDescriptorTableEntry))
                                                             .SetStride(sizeof(IRDescriptorTableEntry))
                                                             .SetUsage(BufferUsage::SHADER_READ));
        m_SamplerHandle->SetName("Sampler Descriptor Heap");

        m_MappedData = m_Handle->Map();
        m_MappedSamplerData = m_SamplerHandle->Map();
    }

    MetalBindlessManager::~MetalBindlessManager()
    {
        delete m_SamplerHandle;
        delete m_Handle;
    }

    uint32 MetalBindlessManager::WriteTextureView(MetalTextureView* view)
    {
        IRDescriptorTableEntry entry;
        IRDescriptorTableSetTexture(&entry, view->GetView(), 0.0f, 0);

        uint index = m_FreeList.Allocate();
        memcpy((char*)m_MappedData + index * sizeof(IRDescriptorTableEntry), &entry, sizeof(IRDescriptorTableEntry));
        return index;
    }

    uint32 MetalBindlessManager::WriteBufferView(MetalBufferView* view)
    {
        IRDescriptorTableEntry entry;
        IRDescriptorTableSetBuffer(&entry, view->GetDesc().Buffer->GetGPUAddress(), 0);

        uint index = m_FreeList.Allocate();
        memcpy((char*)m_MappedData + index * sizeof(IRDescriptorTableEntry), &entry, sizeof(IRDescriptorTableEntry));
        return index;
    }

    uint32 MetalBindlessManager::WriteSampler(MetalSampler* sampler)
    {
        IRDescriptorTableEntry entry;
        IRDescriptorTableSetSampler(&entry, sampler->GetSampler(), 0);

        uint index = m_SamplerFreeList.Allocate();
        memcpy((char*)m_MappedSamplerData + index * sizeof(IRDescriptorTableEntry), &entry, sizeof(IRDescriptorTableEntry));
        return index;
    }

    // Metal Shader Converter requires RaytracingAccelerationStructure to be accessed as a buffer
    // containing the AS GPU resource ID — not as a direct resource heap entry.
    // The caller is responsible for creating and updating the handle buffer.
    uint32 MetalBindlessManager::WriteAccelerationStructure(MetalBuffer* handleBuffer)
    {
        IRDescriptorTableEntry entry;
        IRDescriptorTableSetBuffer(&entry, handleBuffer->GetGPUAddress(), 0);

        uint index = m_FreeList.Allocate();
        memcpy((char*)m_MappedData + index * sizeof(IRDescriptorTableEntry), &entry, sizeof(IRDescriptorTableEntry));
        return index;
    }

    void MetalBindlessManager::Free(uint index)
    {
        m_FreeList.Free(index);
    }

    void MetalBindlessManager::FreeSampler(uint index)
    {
        m_SamplerFreeList.Free(index);
    }
}
