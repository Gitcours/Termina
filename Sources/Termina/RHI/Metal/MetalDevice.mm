#include "MetalDevice.hpp"
#include "MetalRenderContext.hpp"
#include "MetalTexture.hpp"
#include "MetalTextureView.hpp"
#include "MetalSurface.hpp"
#include "MetalRenderPipeline.hpp"
#include "MetalBuffer.hpp"
#include "MetalBufferView.hpp"
#include "MetalSampler.hpp"
#include "MetalComputePipeline.hpp"
#include "MetalBLAS.hpp"
#include "MetalTLAS.hpp"

#include <Termina/Core/Logger.hpp>

namespace Termina {
    MetalDevice::MetalDevice()
    {
        m_Device = MTLCreateSystemDefaultDevice();
        TN_INFO("Using Metal device: %s", [m_Device.name UTF8String]);

        m_CommandQueue = [m_Device newCommandQueue];

        MTLResidencySetDescriptor* residencySetDescriptor = [MTLResidencySetDescriptor new];
        residencySetDescriptor.initialCapacity = 1000;

        NSError* error = nil;
        m_ResidencySet = [m_Device newResidencySetWithDescriptor:residencySetDescriptor error:&error];
        if (error) {
            TN_ERROR("Failed to create Metal residency set");
        }

        [m_CommandQueue addResidencySet:m_ResidencySet];

        m_BindlessManager = new MetalBindlessManager(this);
    }

    MetalDevice::~MetalDevice()
    {
        delete m_BindlessManager;
    }

    void MetalDevice::ExecuteRenderContext(RenderContext* context)
    {
        [reinterpret_cast<MetalRenderContext*>(context)->GetCommandBuffer() commit];
    }

    void MetalDevice::WaitIdle()
    {
        dispatch_semaphore_t sema = dispatch_semaphore_create(0);
        id<MTLCommandBuffer> cmdBuf = [m_CommandQueue commandBuffer];
        [cmdBuf addCompletedHandler:^(id<MTLCommandBuffer>) {
            dispatch_semaphore_signal(sema);
        }];
        [cmdBuf commit];
        dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
    }

    RendererSurface* MetalDevice::CreateSurface(Window* window)
    {
        return reinterpret_cast<RendererSurface*>(new MetalSurface(this, window));
    }

    RenderContext* MetalDevice::CreateRenderContext(bool singleTime)
    {
        return reinterpret_cast<RenderContext*>(new MetalRenderContext(this, singleTime));
    }

    RendererTexture* MetalDevice::CreateTexture(const TextureDesc& desc)
    {
        return reinterpret_cast<RendererTexture*>(new MetalTexture(this, desc));
    }

    TextureView* MetalDevice::CreateTextureView(const TextureViewDesc& desc)
    {
        return reinterpret_cast<TextureView*>(new MetalTextureView(this, desc));
    }

    RenderPipeline* MetalDevice::CreateRenderPipeline(const RenderPipelineDesc& desc)
    {
        return reinterpret_cast<RenderPipeline*>(new MetalRenderPipeline(this, desc));
    }

    RendererBuffer* MetalDevice::CreateBuffer(const BufferDesc& desc)
    {
        return reinterpret_cast<RendererBuffer*>(new MetalBuffer(this, desc));
    }

    BufferView* MetalDevice::CreateBufferView(const BufferViewDesc& desc)
    {
        return reinterpret_cast<BufferView*>(new MetalBufferView(this, desc));
    }

    Sampler* MetalDevice::CreateSampler(const SamplerDesc& desc)
    {
        return reinterpret_cast<Sampler*>(new MetalSampler(this, desc));
    }

    ComputePipeline* MetalDevice::CreateComputePipeline(const ShaderModule& module, const std::string& name)
    {
        return reinterpret_cast<ComputePipeline*>(new MetalComputePipeline(this, module, name));
    }

    BLAS* MetalDevice::CreateBLAS(const BLASDesc& desc)
    {
        if (!m_Device.supportsRaytracing) return nullptr;
        return reinterpret_cast<BLAS*>(new MetalBLAS(this, desc));
    }

    TLAS* MetalDevice::CreateTLAS(uint32 maxInstances)
    {
        if (!m_Device.supportsRaytracing) return nullptr;
        return reinterpret_cast<TLAS*>(new MetalTLAS(this, maxInstances));
    }
} // namespace Termina
