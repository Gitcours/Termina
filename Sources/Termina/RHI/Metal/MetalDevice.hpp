#pragma once

#include <Termina/RHI/Device.hpp>

#import <Metal/Metal.h>

#include "MetalBindlessManager.hpp"

namespace Termina {

    class MetalDevice : public RendererDevice
    {
    public:
        MetalDevice();
        ~MetalDevice();

        void ExecuteRenderContext(RenderContext* context) override;
        void WaitIdle() override;

        RendererBackend GetBackend() const override { return RendererBackend::Metal; }
        RendererSurface* CreateSurface(Window* window) override;
        RenderContext* CreateRenderContext(bool singleTime) override;
        RendererTexture* CreateTexture(const TextureDesc& desc) override;
        TextureView* CreateTextureView(const TextureViewDesc& desc) override;
        RenderPipeline* CreateRenderPipeline(const RenderPipelineDesc& desc) override;
        RendererBuffer* CreateBuffer(const BufferDesc& desc) override;
        BufferView* CreateBufferView(const BufferViewDesc& desc) override;
        Sampler* CreateSampler(const SamplerDesc& desc) override;
        ComputePipeline* CreateComputePipeline(const ShaderModule& module, const std::string& name = "Compute Pipeline") override;
        BLAS* CreateBLAS(const BLASDesc& desc) override;
        TLAS* CreateTLAS(uint32 maxInstances) override;

        bool SupportsRaytracing() const override { return m_Device.supportsRaytracing; }
        bool SupportsMeshShaders() const override { return SupportsRaytracing(); }
        uint64 GetOptimalRowPitchAlignment() const override { return 4; }
        uint64 GetBufferImageGranularity() const override { return 1; }
        TextureFormat GetSurfaceFormat() const override { return TextureFormat::BGRA8_UNORM; }

        id<MTLDevice> GetDevice() { return m_Device; }
        id<MTLCommandQueue> GetCommandQueue() { return m_CommandQueue; }
        id<MTLResidencySet> GetResidencySet() { return m_ResidencySet; }
        MetalBindlessManager* GetBindlessManager() { return m_BindlessManager; }
    private:
        id<MTLDevice> m_Device;
        id<MTLCommandQueue> m_CommandQueue;
        id<MTLResidencySet> m_ResidencySet;

        MetalBindlessManager* m_BindlessManager;
    };
} // namespace Termina
